#include "StateTree/STT_PeekAndShoot.h"

#include "Cover/CoverShooterCharacter.h"
#include "CoverAI.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FSTT_PeekAndShoot::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	ACoverShooterCharacter* Bot = Cast<ACoverShooterCharacter>(Data.Pawn);
	if (!Bot)
	{
		UE_LOG(LogCoverAI, Warning, TEXT("PeekAndShoot: the context pawn is not a CoverShooterCharacter."));
		return EStateTreeRunStatus::Failed;
	}

	UE_LOG(LogCoverAI, Verbose, TEXT("EnterState: PeekAndShoot (%d shots)"), Data.ShotsPerBurst);
	Bot->SetAimingMode(true);
	Bot->SetCoverCrouch(false);

	Data.Timer = 0.f;
	Data.ShotsLeft = FMath::Max(1, Data.ShotsPerBurst);
	Data.bBurstStarted = false;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTT_PeekAndShoot::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	ACoverShooterCharacter* Bot = Cast<ACoverShooterCharacter>(Data.Pawn);
	if (!Bot)
	{
		return EStateTreeRunStatus::Failed;
	}

	Bot->AimAtTarget(DeltaTime);
	Data.Timer += DeltaTime;

	if (!Data.bBurstStarted)
	{
		// Still standing up / aiming.
		if (Data.Timer < Data.StandUpDelay)
		{
			return EStateTreeRunStatus::Running;
		}

		Data.bBurstStarted = true;
		Data.Timer = 0.f;
	}
	else if (Data.Timer < Data.ShotInterval)
	{
		return EStateTreeRunStatus::Running;
	}
	else
	{
		Data.Timer = 0.f;
	}

	Bot->FireOnce();
	--Data.ShotsLeft;

	return Data.ShotsLeft <= 0 ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

void FSTT_PeekAndShoot::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (ACoverShooterCharacter* Bot = Cast<ACoverShooterCharacter>(Data.Pawn))
	{
		Bot->SetAimingMode(false);
	}
}
