#include "Cover/CoverPoint.h"

#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The engine cube is 100 cm wide, so its half extent is 50. */
	constexpr float BasicShapeHalfSize = 50.f;
}

ACoverPoint::ACoverPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	BlockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockMesh"));
	BlockMesh->SetupAttachment(Root);
	BlockMesh->SetMobility(EComponentMobility::Movable);
	BlockMesh->SetCollisionProfileName(TEXT("BlockAll"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BlockMesh->SetStaticMesh(CubeMesh.Object);
	}

	FrontArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("FrontArrow"));
	FrontArrow->SetupAttachment(Root);
	FrontArrow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FrontArrow->SetHiddenInGame(true);
	FrontArrow->ArrowSize = 1.25f;
	FrontArrow->ArrowColor = FColor::Orange;

	UpdateMeshFromExtents();
}

void ACoverPoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateMeshFromExtents();
}

#if WITH_EDITOR
void ACoverPoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateMeshFromExtents();
}
#endif

void ACoverPoint::UpdateMeshFromExtents()
{
	if (BlockMesh)
	{
		BlockMesh->SetRelativeLocation(FVector(0.f, 0.f, BlockHalfExtents.Z));
		BlockMesh->SetRelativeScale3D(FVector(
			FMath::Max(0.01f, BlockHalfExtents.X / BasicShapeHalfSize),
			FMath::Max(0.01f, BlockHalfExtents.Y / BasicShapeHalfSize),
			FMath::Max(0.01f, BlockHalfExtents.Z / BasicShapeHalfSize)));
	}

	if (FrontArrow)
	{
		FrontArrow->SetRelativeLocation(FVector(0.f, 0.f, BlockHalfExtents.Z));
	}
}

FVector ACoverPoint::GetTopCenter() const
{
	return GetActorLocation() + FVector(0.f, 0.f, BlockHalfExtents.Z * 2.f);
}

bool ACoverPoint::IsPlayerInFront(const FVector& PlayerLocation) const
{
	FVector ToPlayer = PlayerLocation - GetActorLocation();
	ToPlayer.Z = 0.f;

	if (ToPlayer.IsNearlyZero())
	{
		return true;
	}

	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	return FVector::DotProduct(ToPlayer.GetSafeNormal(), Forward) > 0.1f;
}

FVector ACoverPoint::GetCoverSpot(const FVector& PlayerLocation) const
{
	const FVector CoverLocation = GetActorLocation();

	// Direction from the block towards the bot: exactly opposite of the player.
	FVector AwayFromPlayer = CoverLocation - PlayerLocation;
	AwayFromPlayer.Z = 0.f;
	AwayFromPlayer = AwayFromPlayer.GetSafeNormal();
	if (AwayFromPlayer.IsNearlyZero())
	{
		// Player is standing on top of the block center: fall back to "behind" the block.
		AwayFromPlayer = -GetActorForwardVector().GetSafeNormal2D();
	}

	// Distance from the block center to its border along AwayFromPlayer (box support function).
	const float HalfX = FMath::Max(1.f, BlockHalfExtents.X);
	const float HalfY = FMath::Max(1.f, BlockHalfExtents.Y);
	const float Denominator = FMath::Abs(AwayFromPlayer.X) / HalfX + FMath::Abs(AwayFromPlayer.Y) / HalfY;
	const float BorderDistance = Denominator > KINDA_SMALL_NUMBER ? 1.f / Denominator : 0.f;

	FVector Spot = CoverLocation + AwayFromPlayer * (BorderDistance + StandBackDistance);
	Spot.Z = CoverLocation.Z;
	return Spot;
}
