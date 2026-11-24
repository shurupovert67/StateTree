#include "CoverAIEditorTools.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Builders/CubeBuilder.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "AI/Navigation/NavigationTypes.h"
#include "Cover/CoverPoint.h"
#include "Cover/CoverProjectile.h"
#include "Cover/CoverShooterCharacter.h"
#include "Cover/CoverShooterController.h"
#include "CoverAI.h"
#include "Editor.h"
#include "Model.h"
#include "Components/BrushComponent.h"
#include "Engine/Polys.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "Misc/PackageName.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "StateTree.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditingSubsystem.h"
#include "StateTreeEditorData.h"
#include "StateTreeEditorNode.h"
#include "StateTreeFactory.h"
#include "StateTreeState.h"
#include "StateTree/STC_NeedsNewCover.h"
#include "StateTree/STT_FindBestCover.h"
#include "StateTree/STT_MoveToCover.h"
#include "StateTree/STT_PeekAndShoot.h"
#include "StateTree/STT_SetCrouch.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
	const TCHAR* StateTreePackageName = TEXT("/Game/AI/StateTree/ST_Bot_CoverShooter");
	const TCHAR* StateTreeAssetName = TEXT("ST_Bot_CoverShooter");

	/** The engine "Delay Task". Looked up by path, because its header is private to StateTreeModule. */
	UScriptStruct* FindDelayTaskStruct()
	{
		return FindObject<UScriptStruct>(nullptr, TEXT("/Script/StateTreeModule.StateTreeDelayTask"));
	}

	UStateTreeState* AddState(UStateTreeState& Parent, const TCHAR* StateName, const TCHAR* Description)
	{
		UStateTreeState& State = Parent.AddChildState(FName(StateName));
		State.Description = Description;
		return &State;
	}
}

bool UCoverAIEditorTools::BuildCoverStateTree()
{
	const FString PackageName(StateTreePackageName);
	const FName AssetName(StateTreeAssetName);

	// ---------------------------------------------------------------- asset
	UStateTree* StateTree = nullptr;
	UPackage* Package = nullptr;

	if (FPackageName::DoesPackageExist(PackageName))
	{
		StateTree = LoadObject<UStateTree>(nullptr, *PackageName);
		if (StateTree)
		{
			Package = StateTree->GetOutermost();
			UE_LOG(LogCoverAI, Log, TEXT("Rebuilding existing StateTree asset '%s'."), *PackageName);
		}
	}

	if (!StateTree)
	{
		Package = CreatePackage(*PackageName);
		UStateTreeFactory* Factory = NewObject<UStateTreeFactory>(GetTransientPackage());
		Factory->SetSchemaClass(UStateTreeAIComponentSchema::StaticClass());

		StateTree = Cast<UStateTree>(Factory->FactoryCreateNew(
			UStateTree::StaticClass(),
			Package,
			AssetName,
			RF_Public | RF_Standalone | RF_Transactional,
			nullptr,
			GWarn));

		if (!StateTree)
		{
			UE_LOG(LogCoverAI, Error, TEXT("Failed to create StateTree asset '%s'."), *PackageName);
			return false;
		}

		FAssetRegistryModule::AssetCreated(StateTree);
		UE_LOG(LogCoverAI, Log, TEXT("Created StateTree asset '%s'."), *PackageName);
	}

	UStateTreeEditorData* EditorData = Cast<UStateTreeEditorData>(StateTree->EditorData);
	if (!EditorData)
	{
		UE_LOG(LogCoverAI, Error, TEXT("'%s' has no StateTree editor data."), *PackageName);
		return false;
	}

	// The component on the bot uses the "StateTree AI Component" schema (pawns + AIController as context).
	if (!EditorData->Schema || !EditorData->Schema->IsA<UStateTreeAIComponentSchema>())
	{
		EditorData->Schema = NewObject<UStateTreeAIComponentSchema>(EditorData);
		UE_LOG(LogCoverAI, Warning, TEXT("Schema was missing or wrong, reset it to 'StateTree AI Component'."));
	}

	EditorData->SubTrees.Reset();

	// --------------------------------------------------------------- states
	UStateTreeState& Root = EditorData->AddRootState();
	Root.Name = TEXT("Root");
	Root.Description = TEXT("Cover shooter: pick cover, hide, peek, shoot, repeat.");

	UStateTreeState* FindState = AddState(Root, TEXT("FindBestCover"),
		TEXT("Score all cover blocks for the current player position and remember the best one."));

	UStateTreeState* MoveState = AddState(Root, TEXT("MoveToCover"),
		TEXT("Walk to the chosen cover spot."));

	UStateTreeState* InCoverState = AddState(Root, TEXT("InCover"),
		TEXT("Crouch behind the block (this is also 'return to cover' after shooting)."));

	UStateTreeState* PeekState = AddState(Root, TEXT("PeekAndShoot"),
		TEXT("Stand up, aim at the player, fire a short burst."));

	UStateTreeState* WaitState = AddState(Root, TEXT("Wait"),
		TEXT("Short delay before retrying when no cover was found."));

	// ---------------------------------------------------------------- tasks
	FindState->AddTask<FSTT_FindBestCover>();
	MoveState->AddTask<FSTT_MoveToCover>();
	InCoverState->AddTask<FSTT_SetCrouch>();
	PeekState->AddTask<FSTT_PeekAndShoot>();

	const bool bHasDelayTask = FindDelayTaskStruct() != nullptr;
	if (bHasDelayTask)
	{
		WaitState->Tasks.AddDefaulted_GetRef().InitializeAs(WaitState, FindDelayTaskStruct());
	}

	UStateTreeState* RetryState = bHasDelayTask ? WaitState : FindState;

	// ----------------------------------------------------------- transitions
	FindState->AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, MoveState);
	FindState->AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, RetryState);

	MoveState->AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::GotoState, InCoverState);
	MoveState->AddTransition(EStateTreeTransitionTrigger::OnStateFailed, EStateTreeTransitionType::GotoState, RetryState);

	InCoverState->AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, PeekState);

	// While hiding, re-check the cover: if the player walked around it or the line of sight opened, pick a new one.
	FStateTreeTransition& RecheckTransition = InCoverState->AddTransition(
		EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, FindState);
	RecheckTransition.Conditions.AddDefaulted_GetRef().InitializeAs<FSTC_NeedsNewCover>(InCoverState);

	PeekState->AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, InCoverState);

	if (bHasDelayTask)
	{
		WaitState->AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, FindState);
	}
	else
	{
		UE_LOG(LogCoverAI, Warning, TEXT("'StateTreeDelayTask' was not found, the retry state will fall back to FindBestCover without a delay."));
	}

	// --------------------------------------------------------------- compile
	FStateTreeCompilerLog CompilerLog;
	const bool bCompiled = UStateTreeEditingSubsystem::CompileStateTree(StateTree, CompilerLog);
	CompilerLog.DumpToLog(StateTree, LogCoverAI);

	if (!bCompiled)
	{
		UE_LOG(LogCoverAI, Error, TEXT("StateTree compilation failed, see messages above."));
		return false;
	}

	// ------------------------------------------------------------------ save
	Package->MarkPackageDirty();

	const FString FileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const bool bSaved = UPackage::SavePackage(Package, StateTree, *FileName, SaveArgs);

	if (!bSaved)
	{
		UE_LOG(LogCoverAI, Error, TEXT("Failed to save '%s' to '%s'."), *PackageName, *FileName);
		return false;
	}

	UE_LOG(LogCoverAI, Log, TEXT("StateTree '%s' compiled and saved."), *PackageName);
	return true;
}

bool UCoverAIEditorTools::BuildDemoLevel()
{
	const TCHAR* SourceMapName = TEXT("/Game/ThirdPerson/Lvl_ThirdPerson");
	const TCHAR* TargetMapName = TEXT("/Game/AI/Lvl_CoverShooting");

	UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(SourceMapName);
	if (!World)
	{
		UE_LOG(LogCoverAI, Error, TEXT("Could not load '%s'."), SourceMapName);
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Remove leftovers of earlier experiments from the copied map, so the demo is predictable.
	int32 RemovedActors = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor->IsA<ACoverPoint>()
			|| Actor->IsA<ACoverShooterCharacter>()
			|| Actor->IsA<ACoverShooterController>()
			|| Actor->IsA<ACoverProjectile>())
		{
			World->EditorDestroyActor(Actor, true);
			++RemovedActors;
		}
	}
	UE_LOG(LogCoverAI, Log, TEXT("Removed %d old cover-AI actors from the copied map."), RemovedActors);

	// Arena centre = player start.
	FVector Center = FVector::ZeroVector;
	APlayerStart* PlayerStart = nullptr;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		PlayerStart = *It;
		Center = PlayerStart->GetActorLocation();
		break;
	}

	if (!PlayerStart)
	{
		UE_LOG(LogCoverAI, Error, TEXT("'%s' has no PlayerStart, the map was probably not loaded."), SourceMapName);
		return false;
	}

	// Ground height under the centre (arena floor).
	float FloorZ = 0.f;
	{
		FHitResult Hit;
		FCollisionQueryParams QueryParams(FName(TEXT("CoverDemoFloor")), false);
		QueryParams.AddIgnoredActor(PlayerStart);

		const FVector Start = Center + FVector(0.f, 0.f, 300.f);
		const FVector End = Center - FVector(0.f, 0.f, 3000.f);
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams) && Hit.bBlockingHit)
		{
			FloorZ = Hit.ImpactPoint.Z;
		}
	}

	UE_LOG(LogCoverAI, Log, TEXT("Arena centre %s, floor Z %.1f."), *Center.ToCompactString(), FloorZ);

	// Cover blocks in a ring around the centre, the front arrow points at the centre (towards the player).
	struct FPlacement
	{
		float AngleDegrees;
		float Radius;
	};
	static const FPlacement Placements[] =
	{
		{   0.f,  900.f },
		{  65.f, 1150.f },
		{ 140.f,  950.f },
		{ 215.f, 1250.f },
		{ 295.f, 1000.f },
	};

	int32 SpawnedCovers = 0;
	for (const FPlacement& Placement : Placements)
	{
		const float Radians = FMath::DegreesToRadians(Placement.AngleDegrees);
		const FVector Offset(FMath::Cos(Radians) * Placement.Radius, FMath::Sin(Radians) * Placement.Radius, 0.f);

		FVector Location = Center + Offset;
		Location.Z = FloorZ;

		const FRotator Rotation(0.f, (Center - Location).Rotation().Yaw, 0.f);
		if (World->SpawnActor<ACoverPoint>(ACoverPoint::StaticClass(), Location, Rotation, SpawnParameters))
		{
			++SpawnedCovers;
		}
	}

	// The bot starts off to the side, so it has to walk to a cover.
	FVector BotSpawnLocation = Center + FVector(-700.f, -700.f, 0.f);
	BotSpawnLocation.Z = FloorZ + 90.f; // character origin is the capsule center
	ACoverShooterCharacter* Bot = World->SpawnActor<ACoverShooterCharacter>(
		ACoverShooterCharacter::StaticClass(),
		BotSpawnLocation,
		FRotator(0.f, (Center - BotSpawnLocation).Rotation().Yaw, 0.f),
		SpawnParameters);

	// NavMesh bounds volume covering the arena (kept centred on the floor, not on the player start).
	ANavMeshBoundsVolume* NavBounds = World->SpawnActor<ANavMeshBoundsVolume>(
		ANavMeshBoundsVolume::StaticClass(),
		FVector(Center.X, Center.Y, FloorZ + 400.f),
		FRotator::ZeroRotator,
		SpawnParameters);

	if (NavBounds)
	{
		// A volume spawned from code has no brush model (the editor's placement tools create it),
		// so create it here, otherwise UCubeBuilder silently does nothing.
		if (!NavBounds->Brush)
		{
			NavBounds->Brush = NewObject<UModel>(NavBounds, NAME_None, RF_Transactional);
			NavBounds->Brush->Initialize(NavBounds, true);
		}
		if (UBrushComponent* BrushComponent = NavBounds->GetBrushComponent())
		{
			BrushComponent->Brush = NavBounds->Brush;
		}

		UCubeBuilder* CubeBuilder = NewObject<UCubeBuilder>(NavBounds);
		CubeBuilder->X = 6000.f;
		CubeBuilder->Y = 6000.f;
		CubeBuilder->Z = 1200.f;
		const bool bCubeBuilt = CubeBuilder->Build(World, NavBounds);

		const int32 PolyCount = (NavBounds->Brush && NavBounds->Brush->Polys) ? NavBounds->Brush->Polys->Element.Num() : -1;
		const FBox VolumeBounds = NavBounds->GetComponentsBoundingBox(true);
		UE_LOG(LogCoverAI, Log, TEXT("NavMeshBoundsVolume: build=%s, polys=%d, bounds=%s."),
			bCubeBuilt ? TEXT("ok") : TEXT("failed"), PolyCount, *VolumeBounds.ToString());
	}

	// Build the navmesh now, so the map also works in a standalone/`-game` run (not only in PIE).
	if (UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		NavigationSystem->Build();
	}
	else
	{
		UE_LOG(LogCoverAI, Warning, TEXT("No navigation system in the map, the bot will not be able to walk."));
	}

	const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(World, TargetMapName);
	if (bSaved)
	{
		UE_LOG(LogCoverAI, Log, TEXT("Demo level '%s': saved, covers=%d, bot=%s, navmesh=%s."),
			TargetMapName, SpawnedCovers,
			Bot ? TEXT("yes") : TEXT("no"),
			NavBounds ? TEXT("yes") : TEXT("no"));
	}
	else
	{
		UE_LOG(LogCoverAI, Error, TEXT("Failed to save demo level '%s'."), TargetMapName);
	}

	return bSaved;
}