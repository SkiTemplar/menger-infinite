#include "MengerViewExtension.h"

#include "GlobalShader.h"
#include "PixelShaderUtils.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "RenderGraphBuilder.h"
#include "RenderingThread.h"
#include "SceneView.h"
#include "ScreenPass.h"
#include "ShaderParameterStruct.h"

namespace
{
	TAutoConsoleVariable<int32> CVarMengerEnable(
		TEXT("r.Menger.Enable"), 1,
		TEXT("Dibuja la esponja de Menger tras el tonemapper (0: desactivado)."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarMengerMaxSteps(
		TEXT("r.Menger.MaxSteps"), 256,
		TEXT("Pasos máximos del raymarching por píxel."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<int32> CVarMengerPreFolds(
		TEXT("r.Menger.PreFolds"), 0,
		TEXT("Iteraciones del DE añadidas a las que pide el tamaño de píxel."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarMengerFogStart(
		TEXT("r.Menger.FogStart"), 2.0f,
		TEXT("Distancia en unidades de vista a la que empieza la niebla."),
		ECVF_RenderThreadSafe);

	TAutoConsoleVariable<float> CVarMengerFogLevels(
		TEXT("r.Menger.FogLevels"), 14.0f,
		TEXT("Niveles de escala (factores de 3 en distancia) que tarda la niebla en cubrirlo todo."),
		ECVF_RenderThreadSafe);

	/** Fracción del radio garantizado del bloque 3x3x3 (1 unidad de marco) que puede recorrer un rayo. */
	constexpr float BlockReachFraction = 0.95f;
}

class FMengerSpongePS final : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FMengerSpongePS);
	SHADER_USE_PARAMETER_STRUCT(FMengerSpongePS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		// xyz: cámara en el marco; w: unidades de marco por unidad de vista.
		SHADER_PARAMETER_ARRAY(FVector4f, FrameCamera, [FMengerFrameParams::MaxFrames])
		// x: máscara de ocupación; y: 1 si es el marco raíz repetido.
		SHADER_PARAMETER_ARRAY(FUintVector4, FrameInfo, [FMengerFrameParams::MaxFrames])
		SHADER_PARAMETER(int32, NumFrames)
		SHADER_PARAMETER(float, FrameReach)
		SHADER_PARAMETER(FVector3f, CamRight)
		SHADER_PARAMETER(FVector3f, CamUp)
		SHADER_PARAMETER(FVector3f, CamForward)
		SHADER_PARAMETER(FVector2f, TanHalfFov)
		SHADER_PARAMETER(FVector2f, ViewRectMin)
		SHADER_PARAMETER(FVector2f, ViewRectSize)
		SHADER_PARAMETER(int32, MaxSteps)
		SHADER_PARAMETER(int32, PreFolds)
		SHADER_PARAMETER(float, FogStart)
		SHADER_PARAMETER(float, FogLevels)
		SHADER_PARAMETER(float, MaxDist)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("MAX_FRAMES"), FMengerFrameParams::MaxFrames);
	}
};

IMPLEMENT_GLOBAL_SHADER(FMengerSpongePS, "/MengerShaders/MengerSponge.usf", "MainPS", SF_Pixel);

FMengerViewExtension::FMengerViewExtension(const FAutoRegister& AutoRegister, UWorld* InWorld)
	: FWorldSceneViewExtension(AutoRegister, InWorld)
{
}

void FMengerViewExtension::SetFrameParams(const FMengerFrameParams& Params)
{
	check(IsInGameThread());
	GameThreadParams = Params;
}

void FMengerViewExtension::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
	// La copia viaja en la cola de render, ordenada con el frame que la usa. El puntero compartido
	// mantiene viva la extensión hasta que se ejecute el comando.
	TSharedRef<FMengerViewExtension, ESPMode::ThreadSafe> Self = StaticCastSharedRef<FMengerViewExtension>(AsShared());
	ENQUEUE_RENDER_COMMAND(MengerSetFrameParams)(
		[Self, Params = GameThreadParams](FRHICommandListImmediate&)
		{
			Self->RenderThreadParams = Params;
		});
}

void FMengerViewExtension::SubscribeToPostProcessingPass(EPostProcessingPass Pass, const FSceneView& View,
	FPostProcessingPassDelegateArray& InOutPassCallbacks, bool bIsPassEnabled)
{
	if (Pass == EPostProcessingPass::Tonemap)
	{
		InOutPassCallbacks.Add(FPostProcessingPassDelegate::CreateRaw(this, &FMengerViewExtension::RenderSponge_RenderThread));
	}
}

FScreenPassTexture FMengerViewExtension::RenderSponge_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View,
	const FPostProcessMaterialInputs& Inputs)
{
	if (CVarMengerEnable.GetValueOnRenderThread() == 0 || !RenderThreadParams.IsSet() || RenderThreadParams->NumFrames == 0)
	{
		return Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
	}
	const FMengerFrameParams& Params = RenderThreadParams.GetValue();

	const FScreenPassTexture SceneColor = FScreenPassTexture::CopyFromSlice(
		GraphBuilder, Inputs.GetInput(EPostProcessMaterialInput::SceneColor));

	// Si hay OverrideOutput, este es el último pase del post-proceso y hay que escribir ahí.
	FScreenPassRenderTarget Output = Inputs.OverrideOutput;
	if (!Output.IsValid())
	{
		Output = FScreenPassRenderTarget::CreateFromInput(
			GraphBuilder, SceneColor, ERenderTargetLoadAction::ENoAction, TEXT("MengerSponge"));
	}

	const FViewMatrices& Matrices = View.ViewMatrices;
	const FMatrix& InvView = Matrices.GetInvViewMatrix();
	const FMatrix& Projection = Matrices.GetProjectionMatrix();
	const FIntRect& Rect = Output.ViewRect;

	FMengerSpongePS::FParameters* Parameters = GraphBuilder.AllocParameters<FMengerSpongePS::FParameters>();
	const int32 NumFrames = FMath::Clamp(Params.NumFrames, 1, FMengerFrameParams::MaxFrames);
	for (int32 Index = 0; Index < NumFrames; ++Index)
	{
		const FMengerFrame& Frame = Params.Frames[Index];
		Parameters->FrameCamera[Index] = FVector4f(Frame.CameraInFrame, Frame.ViewToFrame);
		Parameters->FrameInfo[Index] = FUintVector4(Frame.OccupancyMask, Frame.bIsTiled ? 1u : 0u, 0u, 0u);
	}
	Parameters->NumFrames = NumFrames;
	Parameters->FrameReach = BlockReachFraction;

	// La niebla es logarítmica: lo cubre todo a FogStart · 3^FogLevels. Si el último marco es un bloque,
	// el rayo tampoco puede salir de él.
	const float FogStart = FMath::Max(1e-3f, CVarMengerFogStart.GetValueOnRenderThread());
	const float FogLevels = FMath::Max(0.5f, CVarMengerFogLevels.GetValueOnRenderThread());
	float MaxDist = FogStart * FMath::Pow(3.0f, FogLevels);
	const FMengerFrame& Coarsest = Params.Frames[NumFrames - 1];
	if (!Coarsest.bIsTiled)
	{
		MaxDist = FMath::Min(MaxDist, BlockReachFraction / Coarsest.ViewToFrame);
	}

	// Espacio de vista de UE: X derecha, Y arriba, Z adelante.
	Parameters->CamRight = FVector3f(InvView.GetUnitAxis(EAxis::X));
	Parameters->CamUp = FVector3f(InvView.GetUnitAxis(EAxis::Y));
	Parameters->CamForward = FVector3f(InvView.GetUnitAxis(EAxis::Z));
	Parameters->TanHalfFov = FVector2f(1.0 / Projection.M[0][0], 1.0 / Projection.M[1][1]);
	Parameters->ViewRectMin = FVector2f(Rect.Min.X, Rect.Min.Y);
	Parameters->ViewRectSize = FVector2f(Rect.Width(), Rect.Height());
	Parameters->MaxSteps = FMath::Max(1, CVarMengerMaxSteps.GetValueOnRenderThread());
	Parameters->PreFolds = FMath::Max(0, CVarMengerPreFolds.GetValueOnRenderThread());
	Parameters->FogStart = FogStart;
	Parameters->FogLevels = FogLevels;
	Parameters->MaxDist = MaxDist;
	Parameters->RenderTargets[0] = Output.GetRenderTargetBinding();

	const FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
	const TShaderMapRef<FMengerSpongePS> PixelShader(ShaderMap);
	FPixelShaderUtils::AddFullscreenPass(GraphBuilder, ShaderMap, RDG_EVENT_NAME("MengerSponge"), PixelShader, Parameters, Rect);

	return MoveTemp(Output);
}
