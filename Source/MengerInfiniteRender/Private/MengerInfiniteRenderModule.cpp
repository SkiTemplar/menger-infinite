#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ShaderCore.h"

/** Registra el directorio virtual /MengerShaders; por eso el módulo carga en PostConfigInit. */
class FMengerInfiniteRenderModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		const FString ShaderDir = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders")));
		AddShaderSourceDirectoryMapping(TEXT("/MengerShaders"), ShaderDir);
	}
};

IMPLEMENT_MODULE(FMengerInfiniteRenderModule, MengerInfiniteRender);
