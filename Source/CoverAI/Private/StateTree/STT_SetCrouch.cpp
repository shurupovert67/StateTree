#include "StateTree/STT_SetCrouch.h"

#include "Cover/CoverShooterCharacter.h"
#include "CoverAI.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FSTT_SetCrouch::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	ACoverShooterCharacter* Bot = Cast<ACoverShooterCharacter>(Data.Pawn);
	if (!Bot)
	{
		UE_LOG(LogCoverAI, Warning, TEXT("SetCrouch: the context pawn is not a CoverShooterCharacter."));
		return EStateTreeRunStatus::Failed;
	}

	UE_LOG(LogCoverAI, Verbose, TEXT("EnterState: SetCrouch(%s), hold %.2fs"), Data.bCrouch ? TEXT("true") : TEXT("false"), Data.HoldTime);
	Bot->SetCoverCrouch(Data.bCrouch);

	Data.RemainingTime = Data.HoldTime;
	return Data.RemainingTime > 0.f ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FSTT_SetCrouch::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	Data.RemainingTime -= DeltaTime;
	return Data.RemainingTime <= 0.f ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}
