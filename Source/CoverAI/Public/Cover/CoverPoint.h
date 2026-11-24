#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CoverPoint.generated.h"

class UArrowComponent;
class UStaticMeshComponent;

/**
 * Waist-high cover block.
 *
 * The actor origin is on the ground under the block, the arrow shows the "front" side
 * (the side the block is supposed to protect from).
 *
 * The bot does not have a hardcoded "stand here" marker: the stand spot is computed
 * from the player position, so a single block works for a player standing anywhere.
 */
UCLASS(Blueprintable)
class COVERAI_API ACoverPoint : public AActor
{
	GENERATED_BODY()

public:
	ACoverPoint();

	/** Half size of the block footprint (X/Y) and half height (Z), in centimeters. 65 => 130 cm tall. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	FVector BlockHalfExtents = FVector(100.f, 100.f, 65.f);

	/** Extra gap between the block and the bot capsule. Keep it bigger than the capsule radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	float StandBackDistance = 55.f;

	/** Returns the point where the bot should stand to be hidden behind this block. */
	UFUNCTION(BlueprintPure, Category = "Cover")
	FVector GetCoverSpot(const FVector& PlayerLocation) const;

	/** True when the player is on the "front" side of the block, i.e. this block can actually protect the bot. */
	UFUNCTION(BlueprintPure, Category = "Cover")
	bool IsPlayerInFront(const FVector& PlayerLocation) const;

	/** Center of the top face of the block (useful for debug drawing). */
	UFUNCTION(BlueprintPure, Category = "Cover")
	FVector GetTopCenter() const;

	UFUNCTION(BlueprintPure, Category = "Cover")
	float GetBlockHeight() const { return BlockHalfExtents.Z * 2.f; }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	void UpdateMeshFromExtents();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UStaticMeshComponent> BlockMesh;

	/** Points to the front side of the block. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cover")
	TObjectPtr<UArrowComponent> FrontArrow;
};
