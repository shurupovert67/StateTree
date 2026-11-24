#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CoverShooterCharacter.generated.h"

class ACoverPoint;
class ACoverProjectile;

/**
 * AI bot that fights from behind waist-high cover.
 *
 * This class only contains "mechanics": crouch, aim, shoot, cover scoring.
 * The behavior itself (when to search, when to hide, when to peek) is authored
 * in the StateTree asset and drives these functions through StateTree tasks.
 */
UCLASS()
class COVERAI_API ACoverShooterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACoverShooterCharacter();

	/** Projectiles are spawned here. It sits above the cover block, so the bot can shoot while standing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CoverAI|Components")
	TObjectPtr<USceneComponent> MuzzlePoint;

	// ------------------------------------------------------------------ Cover
	/** Covers that put the player closer than this are ignored (too easy to push). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoverAI|Cover")
	float MinFiringDistance = 600.f;

	/** Covers that put the player further than this are ignored. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoverAI|Cover")
	float MaxFiringDistance = 3500.f;

	/** Comfortable shooting distance, used as a score bonus. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoverAI|Cover")
	float IdealFiringDistance = 1200.f;

	/** Weight of "distance to the player" in the cover score. 0 = only "closest cover to the bot". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoverAI|Cover")
	float FiringDistanceWeight = 0.5f;

	/** Draws the chosen cover spot while playing (demo/debug helper). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoverAI|Cover")
	bool bDrawCoverDebug = true;

	/** The template has no crouch animations, so the crouched mesh is squashed and kept on the ground.
	 *  Turn this off as soon as you add a real crouch pose / animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoverAI|Cover")
	bool bUsePlaceholderCrouchVisual = true;

	/** How much the mesh is squashed while crouched (1 = no squash). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoverAI|Cover", meta = (EditCondition = "bUsePlaceholderCrouchVisual", ClampMin = "0.3", ClampMax = "1.0"))
	float CrouchVisualScaleZ = 0.62f;

	// ----------------------------------------------------------------- Combat
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoverAI|Combat")
	TSubclassOf<ACoverProjectile> ProjectileClass;

	/** How fast the bot turns towards the player while peeking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoverAI|Combat")
	float AimInterpSpeed = 8.f;

	/** Random cone (in degrees) applied to every shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoverAI|Combat")
	float AimSpreadDegrees = 1.5f;

	/** Aim point height above the player origin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CoverAI|Combat")
	float AimTargetHeight = 110.f;

	// ------------------------------------------------------------ Runtime state
	UPROPERTY(BlueprintReadOnly, Transient, Category = "CoverAI|Runtime")
	TObjectPtr<APawn> TargetPlayer;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "CoverAI|Runtime")
	TObjectPtr<ACoverPoint> CurrentCover;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "CoverAI|Runtime")
	FVector CoverSpot = FVector::ZeroVector;

	// --------------------------------------------------------- StateTree API
	/** Picks the best cover for the current player position. Returns false if nothing suitable was found. */
	UFUNCTION(BlueprintCallable, Category = "CoverAI")
	bool SelectBestCover();

	/** True when the current cover / cover spot is not usable anymore (player moved, line of sight opened, ...). */
	UFUNCTION(BlueprintCallable, Category = "CoverAI")
	bool NeedsNewCover() const;

	/** Smoothly turns the bot towards the player (yaw only). Call every tick while peeking. */
	UFUNCTION(BlueprintCallable, Category = "CoverAI")
	void AimAtTarget(float DeltaSeconds);

	/** Spawns a single projectile towards the player. */
	UFUNCTION(BlueprintCallable, Category = "CoverAI")
	void FireOnce();

	/** While aiming the bot faces the target instead of following its movement direction. */
	UFUNCTION(BlueprintCallable, Category = "CoverAI")
	void SetAimingMode(bool bAiming);

	/** Crouch / stand helper that also keeps the movement mode sane. */
	UFUNCTION(BlueprintCallable, Category = "CoverAI")
	void SetCoverCrouch(bool bShouldCrouch);

	UFUNCTION(BlueprintPure, Category = "CoverAI")
	FVector GetMuzzleLocation() const;

protected:
	virtual void BeginPlay() override;

	// Keeps the placeholder crouch visual in sync when the movement component actually applies the crouch.
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	/** True if something (usually the cover block) blocks the view from Observer to Spot. */
	bool IsSpotHiddenFrom(const AActor* Observer, const FVector& Spot) const;

	/** Snaps a location to the navmesh. Returns false when there is no navmesh below/around it. */
	bool ProjectToNavMesh(const FVector& InLocation, FVector& OutLocation) const;

	/** Keeps the mesh feet on the ground and applies the placeholder crouch squash. */
	void UpdateCrouchVisual(bool bCrouched);
};
