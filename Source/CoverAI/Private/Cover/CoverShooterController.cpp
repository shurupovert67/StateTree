#include "Cover/CoverShooterController.h"

#include "Components/StateTreeAIComponent.h"
#include "CoverAI.h"
#include "StateTree.h"
#include "UObject/SoftObjectPath.h"

ACoverShooterController::ACoverShooterController()
{
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));

	StateTreeAsset = TSoftObjectPtr<UStateTree>(
		FSoftObjectPath(TEXT("/Game/AI/StateTree/ST_Bot_CoverShooter.ST_Bot_CoverShooter")));
}

void ACoverShooterController::PostInitProperties()
{
	Super::PostInitProperties();

	// Assign the tree before the component's InitializeComponent() validates it,
	// otherwise the log gets a scary "state tree asset is not set" error.
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		LoadAndAssignStateTree();
	}
}

void ACoverShooterController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// Must happen before the component starts its logic (BeginPlay), otherwise the tree is empty.
	LoadAndAssignStateTree();
}

void ACoverShooterController::LoadAndAssignStateTree()
{
	if (!StateTreeComponent)
	{
		UE_LOG(LogCoverAI, Error, TEXT("[%s] StateTreeAIComponent is missing."), *GetName());
		return;
	}

	if (StateTreeAsset.IsNull())
	{
		UE_LOG(LogCoverAI, Error, TEXT("[%s] No StateTree asset assigned (StateTreeAsset is null)."), *GetName());
		return;
	}

	UStateTree* Tree = StateTreeAsset.LoadSynchronous();
	if (!Tree)
	{
		UE_LOG(LogCoverAI, Error,
			TEXT("[%s] StateTree asset '%s' was not found. Create it in the editor (schema = StateTree AI Component) or assign another asset."),
			*GetName(), *StateTreeAsset.ToString());
		return;
	}

	StateTreeComponent->SetStateTree(Tree);
	UE_LOG(LogCoverAI, Log, TEXT("[%s] StateTree set to '%s'."), *GetName(), *Tree->GetName());
}
