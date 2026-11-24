#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STT_SetCrouch.generated.h"

class APawn;

USTRUCT()
struct COVERAI_API FSTT_SetCrouchInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;

	/** true = hide behind the cover, false = stand up. */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bCrouch = true;

	/** How long to stay in this pose before the state completes. */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = "0.0"))
	float HoldTime = 0.6f;

	UPROPERTY(Transient)
	float RemainingTime = 0.f;
};

/**
 * Crouches down or stands up and (optionally) waits a bit.
 * The delay is built in, so a single task per state is enough.
 */
USTRUCT(meta = (DisplayName = "Cover: Set Crouch", Category = "AI|Cover"))
struct COVERAI_API FSTT_SetCrouch : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTT_SetCrouchInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
