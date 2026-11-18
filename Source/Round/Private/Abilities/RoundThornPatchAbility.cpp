#include "Abilities/RoundThornPatchAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacterDruid.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundTerrainHelpers.h"
#include "Game/RoundGameState.h"

FRoundActionResult ARoundThornPatchAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
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

    bool bConnectedToNature = false;
    if (ARoundGameState* GameState = OwningCharacter->GetRoundGameState())
    {
        for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(TargetTile)) if (Neighbor && RoundTerrain::IsNatureTerrain(Neighbor->TerrainType)) bConnectedToNature = true;
    }
    for (ARoundGridTile* Tile : AffectedTiles)
    {
        if (!Tile) continue;
        ARoundCharacter* Occupant = Tile->GetResolvedOccupyingCharacter();
        if (Occupant && OwningCharacter->IsEnemyFor(Occupant) && GetAbilityDamage() > 0)
        {
            bool bDidDie = false;
            ApplyAbilityDamageToTarget(Occupant, GetAbilityDamage(), FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore")))), bDidDie);
            if (!bDidDie && Occupant->Attributes) Occupant->Attributes->PassInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit")))));
        }
        Tile->SetTerrainType(ERoundTerrainType::Thorns);
    }
    if (bConnectedToNature && OwningCharacter->Attributes)
    {
        OwningCharacter->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeRefundIfConnected")))));
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Thorn patch created.");
    return Result;
}
