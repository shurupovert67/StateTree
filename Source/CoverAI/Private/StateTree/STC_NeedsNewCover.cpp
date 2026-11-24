#include "StateTree/STC_NeedsNewCover.h"

#include "Cover/CoverShooterCharacter.h"
#include "StateTreeExecutionContext.h"

bool FSTC_NeedsNewCover::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	const ACoverShooterCharacter* Bot = Cast<ACoverShooterCharacter>(Data.Pawn);
	if (!Bot)
	{
		return true;
	}

	return Bot->NeedsNewCover();
}
