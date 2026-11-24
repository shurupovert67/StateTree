using UnrealBuildTool;

public class CoverAI : ModuleRules
{
	public CoverAI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"AIModule",
			"GameplayTasks",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
		});
	}
}
