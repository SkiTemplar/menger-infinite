using UnrealBuildTool;

public class MengerInfiniteRender : ModuleRules
{
	public MengerInfiniteRender(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new string[] { "RenderCore", "Renderer", "RHI", "Projects" });
	}
}
