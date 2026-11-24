#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STT_MoveToCover.generated.h"

class APawn;

USTRUCT()
struct COVERAI_API FSTT_MoveToCoverInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	/** How close the bot has to get to the cover spot. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 70.f;

	UPROPERTY(Transient)
	FVector TargetLocation = FVector::ZeroVector;
};

/**
 * Moves the bot to the cover spot picked by FindBestCover.
 * Succeeds when the spot is reached, fails when the path is impossible.
 */
USTRUCT(meta = (DisplayName = "Cover: Move To Cover", Category = "AI|Cover"))
struct COVERAI_API FSTT_MoveToCover : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTT_MoveToCoverInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
