using UnrealBuildTool;
using System.Collections.Generic;

public class MengerInfiniteTarget : TargetRules
{
	public MengerInfiniteTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.AddRange(new string[] { "MengerInfinite", "MengerInfiniteRender" });
	}
}
