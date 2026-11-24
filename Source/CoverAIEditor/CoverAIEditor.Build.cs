using UnrealBuildTool;

public class CoverAIEditor : ModuleRules
{
	public CoverAIEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"AssetRegistry",
			"NavigationSystem",
			"PropertyBindingUtils",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"StateTreeEditorModule",
			"CoverAI"
		});
	}
}
