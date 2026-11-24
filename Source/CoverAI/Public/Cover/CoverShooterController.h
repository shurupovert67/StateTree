#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CoverShooterController.generated.h"

class UStateTree;
class UStateTreeAIComponent;

/**
 * Bot brain.
 *
 * It only owns the StateTree component: all the behavior is in the StateTree asset.
 * The asset is soft referenced so the C++ module does not break when the asset
 * has not been created yet.
 */
UCLASS()
class COVERAI_API ACoverShooterController : public AAIController
{
	GENERATED_BODY()

public:
	ACoverShooterController();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CoverAI|AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;

	/** Behavior asset. Defaults to /Game/AI/StateTree/ST_Bot_CoverShooter. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "CoverAI|AI")
	TSoftObjectPtr<UStateTree> StateTreeAsset;

protected:
	virtual void PostInitProperties() override;
	virtual void PostInitializeComponents() override;

	void LoadAndAssignStateTree();
};
