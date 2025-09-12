// Copyright DiDaPiPa 2024

using UnrealBuildTool;

public class PuertsAutoMixin : ModuleRules
{
	public PuertsAutoMixin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
			new[]
			{
				"PuertsAutoMixin/Public"
			}
		);

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core", "JsEnv"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UMG",
				"Slate",
				"SlateCore",
				"Json",
				"JsonUtilities",
				"Puerts",
			}
		);

		if (Target.bBuildEditor)
		{
			OptimizeCode = CodeOptimization.Never;
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
