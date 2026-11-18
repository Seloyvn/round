#include "Actors/RoundFieldTriggerActor.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Net/UnrealNetwork.h"

ARoundFieldTriggerActor::ARoundFieldTriggerActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);
}

void ARoundFieldTriggerActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundFieldTriggerActor, CauserCharacter);
    DOREPLIFETIME(ARoundFieldTriggerActor, TriggerTiles);
}

void ARoundFieldTriggerActor::InitializeTrigger(ARoundCharacter* InCauserCharacter, const TArray<ARoundGridTile*>& InTriggerTiles)
{
    CauserCharacter = InCauserCharacter;
    TriggerTiles.Reset();

    for (ARoundGridTile* TriggerTile : InTriggerTiles)
    {
        if (!TriggerTile)
        {
            continue;
        }

        TriggerTiles.AddUnique(TriggerTile);
        TriggerTile->AddFieldTrigger(this);
    }
}

void ARoundFieldTriggerActor::NotifyCharacterEntered(ARoundCharacter* EnteringCharacter, ARoundGridTile* EnteredTile, ARoundGridTile* PreviousTile)
{
    if (!HasAuthority())
    {
        return;
    }

    if (!CanTriggerCharacter(EnteringCharacter, EnteredTile, PreviousTile))
    {
        return;
    }

    ExecuteTrigger(EnteringCharacter, EnteredTile, PreviousTile);
}

void ARoundFieldTriggerActor::RemoveTrigger()
{
    const TArray<TObjectPtr<ARoundGridTile>> TilesSnapshot = TriggerTiles;
    TriggerTiles.Reset();

    for (ARoundGridTile* TriggerTile : TilesSnapshot)
    {
        if (TriggerTile)
        {
            TriggerTile->RemoveFieldTrigger(this);
        }
    }

    Destroy();
}

bool ARoundFieldTriggerActor::CanTriggerCharacter_Implementation(ARoundCharacter* EnteringCharacter, ARoundGridTile* EnteredTile, ARoundGridTile* PreviousTile) const
{
    return EnteringCharacter != nullptr && EnteredTile != nullptr && PreviousTile != EnteredTile;
}

void ARoundFieldTriggerActor::ExecuteTrigger_Implementation(ARoundCharacter* EnteringCharacter, ARoundGridTile* EnteredTile, ARoundGridTile* PreviousTile)
{
}
