#include "Cover/CoverShooterCharacter.h"

#include "AI/NavigationSystemBase.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Cover/CoverPoint.h"
#include "Cover/CoverProjectile.h"
#include "Cover/CoverShooterController.h"
#include "CoverAI.h"
#include "DrawDebugHelpers.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** Height of the shot spawn point above the capsule center. Capsule center is Z = 88, so the muzzle is at ~143 cm:
	 *  it clears a 130 cm cover block, which is exactly what the bot needs when it peeks. */
	constexpr float MuzzleHeightAboveCapsuleCenter = 55.f;

	/** How far in front of the bot the muzzle is. */
	constexpr float MuzzleForwardOffset = 45.f;

	/** Offset of the mannequin pivot below the capsule center so that the feet touch the ground. */
	constexpr float MeshFeetOffsetZ = -88.f;
}

ACoverShooterCharacter::ACoverShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// The bot aims with its body (AimAtTarget), movement does not rotate it while peeking.
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 480.f, 0.f);
	Movement->MaxWalkSpeed = 420.f;
	Movement->MaxWalkSpeedCrouched = 200.f;
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
	Movement->SetCrouchedHalfHeight(45.f);
	Movement->bUseControllerDesiredRotation = false;

	// Mannequin from the project template.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannequinMesh(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MannequinMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannequinMesh.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> MannequinAnimBP(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (MannequinAnimBP.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(MannequinAnimBP.Class);
	}

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, MeshFeetOffsetZ));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(GetCapsuleComponent());
	MuzzlePoint->SetRelativeLocation(FVector(MuzzleForwardOffset, 0.f, MuzzleHeightAboveCapsuleCenter));

	// Spawn the AI controller automatically when the bot is placed in the level.
	AIControllerClass = ACoverShooterController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	ProjectileClass = ACoverProjectile::StaticClass();
}

void ACoverShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		TargetPlayer = PlayerController->GetPawn();
	}

	if (!TargetPlayer)
	{
		UE_LOG(LogCoverAI, Warning, TEXT("[%s] No player pawn found on BeginPlay, will retry when searching for cover."), *GetName());
	}
}

FVector ACoverShooterCharacter::GetMuzzleLocation() const
{
	return MuzzlePoint ? MuzzlePoint->GetComponentLocation() : GetActorLocation() + FVector(0.f, 0.f, 130.f);
}

void ACoverShooterCharacter::SetAimingMode(bool bAiming)
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = !bAiming;
	}
}

void ACoverShooterCharacter::SetCoverCrouch(bool bShouldCrouch)
{
	if (bShouldCrouch)
	{
		if (!bIsCrouched)
		{
			Crouch();
		}
	}
	else if (bIsCrouched)
	{
		UnCrouch();
	}

	UpdateCrouchVisual(bShouldCrouch);
}

void ACoverShooterCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	UpdateCrouchVisual(true);
}

void ACoverShooterCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	UpdateCrouchVisual(false);
}

void ACoverShooterCharacter::UpdateCrouchVisual(bool bCrouched)
{
	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent)
	{
		return;
	}

	// The crouched capsule is shorter and its center is lower, so the mesh has to be moved up again,
	// otherwise the mannequin would sink into the floor.
	const float NewOffsetZ = bCrouched
		? -GetCharacterMovement()->GetCrouchedHalfHeight()
		: MeshFeetOffsetZ;

	MeshComponent->SetRelativeLocation(FVector(0.f, 0.f, NewOffsetZ));

	// Placeholder "crouch pose" until real crouch animations are added.
	const float NewScaleZ = (bCrouched && bUsePlaceholderCrouchVisual) ? CrouchVisualScaleZ : 1.f;
	MeshComponent->SetRelativeScale3D(FVector(1.f, 1.f, NewScaleZ));
}

void ACoverShooterCharacter::AimAtTarget(float DeltaSeconds)
{
	if (!IsValid(TargetPlayer))
	{
		return;
	}

	FVector ToTarget = TargetPlayer->GetActorLocation() - GetActorLocation();
	ToTarget.Z = 0.f;
	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const FRotator DesiredRotation(0.f, ToTarget.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaSeconds, AimInterpSpeed));
}

void ACoverShooterCharacter::FireOnce()
{
	if (!ProjectileClass || !IsValid(TargetPlayer) || !GetWorld())
	{
		return;
	}

	// Shooting from behind the block would just hit the block, so only shoot while standing.
	if (bIsCrouched)
	{
		UE_LOG(LogCoverAI, Verbose, TEXT("[%s] FireOnce skipped: bot is crouched."), *GetName());
		return;
	}

	const FVector Start = GetMuzzleLocation();
	const FVector Target = TargetPlayer->GetActorLocation() + FVector(0.f, 0.f, AimTargetHeight);

	FRotator ShotRotation = (Target - Start).Rotation();
	ShotRotation.Yaw += FMath::FRandRange(-AimSpreadDegrees, AimSpreadDegrees);
	ShotRotation.Pitch += FMath::FRandRange(-AimSpreadDegrees * 0.5f, AimSpreadDegrees * 0.5f);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	UE_LOG(LogCoverAI, Verbose, TEXT("[%s] Shot fired."), *GetName());
	GetWorld()->SpawnActor<ACoverProjectile>(ProjectileClass, Start, ShotRotation, SpawnParameters);
}

bool ACoverShooterCharacter::SelectBestCover()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// The player pawn can appear after BeginPlay (respawn, level streaming, ...).
	if (!IsValid(TargetPlayer))
	{
		if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			TargetPlayer = PlayerController->GetPawn();
		}
	}

	if (!IsValid(TargetPlayer))
	{
		UE_LOG(LogCoverAI, Warning, TEXT("[%s] SelectBestCover failed: no player pawn."), *GetName());
		return false;
	}

	const FVector PlayerLocation = TargetPlayer->GetActorLocation();

	ACoverPoint* BestCover = nullptr;
	FVector BestSpot = FVector::ZeroVector;
	float BestScore = TNumericLimits<float>::Max();

	for (TActorIterator<ACoverPoint> It(World); It; ++It)
	{
		ACoverPoint* Cover = *It;
		if (!IsValid(Cover))
		{
			continue;
		}

		if (!Cover->IsPlayerInFront(PlayerLocation))
		{
			UE_LOG(LogCoverAI, Verbose, TEXT("  [%s] skipped: player is not in front of it."), *Cover->GetName());
			continue;
		}

		FVector Spot;
		if (!ProjectToNavMesh(Cover->GetCoverSpot(PlayerLocation), Spot))
		{
			UE_LOG(LogCoverAI, Verbose, TEXT("  [%s] skipped: cover spot is off the navmesh."), *Cover->GetName());
			continue;
		}

		const float DistanceToPlayer = FVector::Dist2D(Spot, PlayerLocation);
		if (DistanceToPlayer < MinFiringDistance || DistanceToPlayer > MaxFiringDistance)
		{
			UE_LOG(LogCoverAI, Verbose, TEXT("  [%s] skipped: player is %.0f cm away (allowed %.0f..%.0f)."),
				*Cover->GetName(), DistanceToPlayer, MinFiringDistance, MaxFiringDistance);
			continue;
		}

		if (!IsSpotHiddenFrom(TargetPlayer, Spot))
		{
			UE_LOG(LogCoverAI, Verbose, TEXT("  [%s] skipped: the cover spot is visible to the player."), *Cover->GetName());
			continue;
		}

		const float DistanceToBot = FVector::Dist2D(GetActorLocation(), Spot);
		const float Score = DistanceToBot + FiringDistanceWeight * FMath::Abs(DistanceToPlayer - IdealFiringDistance);

		UE_LOG(LogCoverAI, Verbose, TEXT("  [%s] candidate, score %.1f (bot %.0f cm, player %.0f cm)."),
			*Cover->GetName(), Score, DistanceToBot, DistanceToPlayer);

		if (Score < BestScore)
		{
			BestScore = Score;
			BestCover = Cover;
			BestSpot = Spot;
		}
	}

	if (!BestCover)
	{
		UE_LOG(LogCoverAI, Warning, TEXT("[%s] No suitable cover found (player at %s)."), *GetName(), *PlayerLocation.ToCompactString());
		return false;
	}

	CurrentCover = BestCover;
	CoverSpot = BestSpot;

	UE_LOG(LogCoverAI, Log, TEXT("[%s] Cover chosen: %s (score %.1f), spot %s."),
		*GetName(), *BestCover->GetName(), BestScore, *BestSpot.ToCompactString());

	if (bDrawCoverDebug && World)
	{
		DrawDebugSphere(World, BestSpot + FVector(0.f, 0.f, 10.f), 30.f, 12, FColor::Green, false, 2.f);
		DrawDebugLine(World, GetActorLocation(), BestSpot, FColor::Green, false, 2.f, 0, 2.f);
		DrawDebugSphere(World, BestCover->GetTopCenter(), 25.f, 8, FColor::Yellow, false, 2.f);
	}

	return true;
}

bool ACoverShooterCharacter::NeedsNewCover() const
{
	if (!IsValid(TargetPlayer) || !IsValid(CurrentCover))
	{
		return true;
	}

	const FVector PlayerLocation = TargetPlayer->GetActorLocation();

	if (!CurrentCover->IsPlayerInFront(PlayerLocation))
	{
		return true;
	}

	const float DistanceToPlayer = FVector::Dist2D(CoverSpot, PlayerLocation);
	if (DistanceToPlayer < MinFiringDistance || DistanceToPlayer > MaxFiringDistance)
	{
		return true;
	}

	// The whole point of cover: the player must not see the spot.
	return !IsSpotHiddenFrom(TargetPlayer, CoverSpot);
}

bool ACoverShooterCharacter::IsSpotHiddenFrom(const AActor* Observer, const FVector& Spot) const
{
	const UWorld* World = GetWorld();
	if (!World || !IsValid(Observer))
	{
		return false;
	}

	const APawn* ObserverPawn = Cast<APawn>(Observer);
	const FVector Start = ObserverPawn
		? ObserverPawn->GetPawnViewLocation()
		: Observer->GetActorLocation() + FVector(0.f, 0.f, 60.f);

	// Aim slightly above the ground so the trace really has to pass the top of the block.
	const FVector End = Spot + FVector(0.f, 0.f, 65.f);

	FCollisionQueryParams QueryParams(FName(TEXT("CoverLineOfSight")), false, this);
	QueryParams.AddIgnoredActor(Observer);

	FHitResult Hit;
	const bool bBlocked = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams);
	return bBlocked;
}

bool ACoverShooterCharacter::ProjectToNavMesh(const FVector& InLocation, FVector& OutLocation) const
{
	if (UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation NavLocation;
		// Deliberately small vertical extent: the top of the cover block also generates navmesh,
		// so a big extent could snap the spot onto the block instead of the floor next to it.
		if (NavigationSystem->ProjectPointToNavigation(InLocation, NavLocation, FVector(80.f, 80.f, 90.f)))
		{
			OutLocation = NavLocation.Location;
			return true;
		}
	}

	OutLocation = InLocation;
	return false;
}
