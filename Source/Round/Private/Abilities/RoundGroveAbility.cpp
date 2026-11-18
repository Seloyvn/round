#include "Abilities/RoundGroveAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundEffectActor.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"

FRoundActionResult ARoundGroveAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!OwningCharacter || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    TArray<ARoundGridTile*> AffectedTiles = {TargetTile};
    if (GetAbilityValueFromKey(TEXT("SpreadToAdjacentFields")) > 0.0)
    {
        if (ARoundGameState* GameState = OwningCharacter->GetRoundGameState())
        {
            for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(TargetTile)) if (Neighbor) AffectedTiles.AddUnique(Neighbor);
        }
    }
    for (ARoundGridTile* Tile : AffectedTiles)
    {
        if (!Tile) continue;
        Tile->SetTerrainType(ERoundTerrainType::Grove);
        ARoundCharacter* Occupant = Tile->GetResolvedOccupyingCharacter();
        if (!Occupant || OwningCharacter->IsEnemyFor(Occupant)) continue;
        Occupant->Heal(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HealOnPlacement")))));
        if (Occupant->Attributes) Occupant->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnPlacement")))));
        if (GetAbilityValueFromKey(TEXT("CleanseOneDebuff")) > 0.0)
        {
            for (ARoundEffectActor* Effect : Occupant->ActiveEffects)
            {
                if (Effect && Effect->IsDebuff() && Effect->IsRemovable()) { Effect->RemoveEffect(); break; }
            }
        }
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Grove created.");
    return Result;
}
