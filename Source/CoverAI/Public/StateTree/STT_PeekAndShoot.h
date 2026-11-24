#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STT_PeekAndShoot.generated.h"

class APawn;

USTRUCT()
struct COVERAI_API FSTT_PeekAndShootInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	/** How many projectiles are fired per peek. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "1"))
	int32 ShotsPerBurst = 4;

	/** Delay between two shots. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.05"))
	float ShotInterval = 0.3f;

	/** Delay between standing up and the first shot (time to aim). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float StandUpDelay = 0.35f;

	UPROPERTY(Transient)
	float Timer = 0.f;

	UPROPERTY(Transient)
	int32 ShotsLeft = 0;

	UPROPERTY(Transient)
	bool bBurstStarted = false;
};

/**
 * The "peek" half of the cycle: stand up, aim at the player, fire a short burst,
 * then finish so the tree can put the bot back into cover.
 */
USTRUCT(meta = (DisplayName = "Cover: Peek And Shoot", Category = "AI|Cover"))
struct COVERAI_API FSTT_PeekAndShoot : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTT_PeekAndShootInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
