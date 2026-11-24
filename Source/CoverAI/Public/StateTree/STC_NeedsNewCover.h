#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "STC_NeedsNewCover.generated.h"

class APawn;

USTRUCT()
struct COVERAI_API FSTC_NeedsNewCoverInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<APawn> Pawn = nullptr;
};

/**
 * Tick transition condition: true when the current cover stopped working
 * (the player walked around it, opened the line of sight or is out of range).
 */
USTRUCT(meta = (DisplayName = "Cover: Needs New Cover", Category = "AI|Cover"))
struct COVERAI_API FSTC_NeedsNewCover : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTC_NeedsNewCoverInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
