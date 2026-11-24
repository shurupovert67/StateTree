#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CoverAIEditorTools.generated.h"

/**
 * Editor-only helpers for the cover AI demo.
 *
 * Callable from Python (or from the editor console via a Python one-liner):
 *   unreal.CoverAIEditorTools.build_cover_state_tree()
 */
UCLASS()
class COVERAIEDITOR_API UCoverAIEditorTools : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Creates (or rebuilds) /Game/AI/StateTree/ST_Bot_CoverShooter with the full cover-shooter
	 * behavior: FindBestCover -> MoveToCover -> InCover -> PeekAndShoot -> InCover ...
	 * The asset is compiled and saved, so it can be used right away.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoverAI")
	static bool BuildCoverStateTree();

	/**
	 * Creates /Game/AI/Lvl_CoverShooting from the Third Person template map:
	 * five cover blocks around the player start, a navmesh bounds volume and one bot.
	 */
	UFUNCTION(BlueprintCallable, Category = "CoverAI")
	static bool BuildDemoLevel();
};
