#include "StateTree/STT_FindBestCover.h"

#include "Cover/CoverShooterCharacter.h"
#include "CoverAI.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FSTT_FindBestCover::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	ACoverShooterCharacter* Bot = Cast<ACoverShooterCharacter>(Data.Pawn);
	if (!Bot)
	{
		UE_LOG(LogCoverAI, Warning, TEXT("FindBestCover: the context pawn is not a CoverShooterCharacter."));
		return EStateTreeRunStatus::Failed;
	}

	UE_LOG(LogCoverAI, Verbose, TEXT("EnterState: FindBestCover"));
	const bool bFound = Bot->SelectBestCover();
	return bFound ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
}
