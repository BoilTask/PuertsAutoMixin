// Copyright DiDaPiPa 2024

using System.IO;
using UnrealBuildTool;

public class PuertsAutoMixinEditor : ModuleRules
{
	public PuertsAutoMixinEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		string EngineDir = Path.GetFullPath(Target.RelativeEnginePath);

		PrivateIncludePaths.AddRange(
			new[]
			{
				"Puerts/Private"
				, "PuertsAutoMixin/Public"
				, "PuertsAutoMixinEditor/Public"
				, Path.Combine(EngineDir, "Source/Editor/AnimationBlueprintEditor/Private")
				, Path.Combine(EngineDir, "Source/Runtime/Slate/Private"),
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"UMG",
				"Slate",
				"SlateCore",
				"BlueprintGraph",
				"ToolMenus",
				"ApplicationCore",
				"Projects",
				"Puerts",
				"PuertsAutoMixin"
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new[]
			{
				"Kismet",
				"MainFrame",
				"AnimationBlueprintEditor",
				"Slate",
				"SlateCore",
				"Persona",
			}
		);
		DynamicallyLoadedModuleNames.AddRange(
			new[]
			{
				"Kismet",
				"MainFrame",
				"AnimationBlueprintEditor",
			}
		);
	}
}
