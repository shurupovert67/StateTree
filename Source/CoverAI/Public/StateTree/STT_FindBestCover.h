#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "STT_FindBestCover.generated.h"

class APawn;

USTRUCT()
struct COVERAI_API FSTT_FindBestCoverInstanceData
{
	GENERATED_BODY()

	/** Filled automatically from the StateTree context (the controlled pawn). */
	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;
};

/**
 * Asks the bot to score every cover block and remember the best one.
 * Succeeds when a cover was found, fails otherwise.
 */
USTRUCT(meta = (DisplayName = "Cover: Find Best Cover", Category = "AI|Cover"))
struct COVERAI_API FSTT_FindBestCover : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTT_FindBestCoverInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
