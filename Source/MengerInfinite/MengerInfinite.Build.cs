using UnrealBuildTool;

public class MengerInfinite : ModuleRules
{
	public MengerInfinite(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "RHI", "MengerInfiniteRender"
		});
	}
}
