#include "StateTree/STT_MoveToCover.h"

#include "AIController.h"
#include "Cover/CoverShooterCharacter.h"
#include "CoverAI.h"
#include "Navigation/PathFollowingComponent.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FSTT_MoveToCover::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);

	ACoverShooterCharacter* Bot = Cast<ACoverShooterCharacter>(Data.Pawn);
	AAIController* AI = Bot ? Cast<AAIController>(Bot->GetController()) : nullptr;
	if (!Bot || !AI)
	{
		UE_LOG(LogCoverAI, Warning, TEXT("MoveToCover: no pawn / AI controller in the context."));
		return EStateTreeRunStatus::Failed;
	}

	if (Bot->CoverSpot.IsNearlyZero())
	{
		UE_LOG(LogCoverAI, Warning, TEXT("MoveToCover: no cover spot was selected (FindBestCover did not run?)."));
		return EStateTreeRunStatus::Failed;
	}

	UE_LOG(LogCoverAI, Verbose, TEXT("EnterState: MoveToCover -> %s"), *Bot->CoverSpot.ToCompactString());
	Bot->SetAimingMode(false);
	Data.TargetLocation = Bot->CoverSpot;

	const EPathFollowingRequestResult::Type Result = AI->MoveToLocation(
		Data.TargetLocation,
		Data.AcceptanceRadius,
		/*bStopOnOverlap*/ false,
		/*bUsePathfinding*/ true,
		/*bProjectDestinationToNavigation*/ false,
		/*bCanStrafe*/ false,
		/*FilterClass*/ nullptr,
		/*bAllowPartialPath*/ false);

	if (Result == EPathFollowingRequestResult::Failed)
	{
		UE_LOG(LogCoverAI, Warning, TEXT("MoveToCover: path request failed (no navmesh / unreachable spot)."));
		return EStateTreeRunStatus::Failed;
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FSTT_MoveToCover::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	APawn* Pawn = Data.Pawn;
	AAIController* AI = Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
	if (!Pawn || !AI)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Still walking (or still computing the path).
	const EPathFollowingStatus::Type MoveStatus = AI->GetMoveStatus();
	if (MoveStatus == EPathFollowingStatus::Moving || MoveStatus == EPathFollowingStatus::Waiting)
	{
		return EStateTreeRunStatus::Running;
	}

	const float DistanceToSpot = FVector::Dist2D(Pawn->GetActorLocation(), Data.TargetLocation);
	// The AI controller adds the agent radius to the reach test, so a few extra centimeters are normal.
	const float AllowedDistance = Data.AcceptanceRadius + 2.f * Pawn->GetSimpleCollisionRadius() + 30.f;
	const bool bArrived = DistanceToSpot <= AllowedDistance;
	if (!bArrived)
	{
		UE_LOG(LogCoverAI, Warning, TEXT("MoveToCover: movement stopped %.0f cm away from the cover spot (allowed %.0f)."), DistanceToSpot, AllowedDistance);
	}
	else
	{
		UE_LOG(LogCoverAI, Verbose, TEXT("MoveToCover: arrived, %.0f cm from the spot."), DistanceToSpot);
	}

	return bArrived ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Failed;
}

void FSTT_MoveToCover::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);

	if (APawn* Pawn = Data.Pawn)
	{
		if (AAIController* AI = Cast<AAIController>(Pawn->GetController()))
		{
			AI->StopMovement();
		}
	}
}
