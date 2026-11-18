#include "Abilities/RoundLandsplitterAbility.h"

#include "Actors/RoundCharacterGeomancer.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"

namespace
{
ARoundGridTile* FindPushBackTile(ARoundGameState* GameState, ARoundGridTile* SourceTile, ARoundGridTile* TargetTile)
{
    if (!GameState || !SourceTile || !TargetTile)
    {
        return nullptr;
    }

    const FVector2D Direction = TargetTile->GridPosition - SourceTile->GridPosition;
    ARoundGridTile* BestTile = nullptr;
    double BestDot = -DBL_MAX;
    for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(TargetTile))
    {
        if (!Neighbor || Neighbor->OccupyingCharacter || Neighbor->GetResolvedOccupyingFieldObject() || !Neighbor->IsMoveable())
        {
            continue;
        }

        const FVector2D NeighborDirection = Neighbor->GridPosition - TargetTile->GridPosition;
        const double Dot = FVector2D::DotProduct(Direction, NeighborDirection);
        if (Dot > BestDot)
        {
            BestDot = Dot;
            BestTile = Neighbor;
        }
    }

    return BestTile;
}

TArray<ARoundGridTile*> BuildCentralLine(const ARoundCharacter* Character, ARoundGridTile* TargetTile, const int32 Range)
{
    TArray<ARoundGridTile*> Result;
    if (!Character || !Character->CurrentTile || !TargetTile)
    {
        return Result;
    }

    const ARoundGameState* GameState = Character->GetRoundGameState();
    if (!GameState)
    {
        return Result;
    }

    Result = GameState->GetFieldsBetween(Character->CurrentTile, TargetTile, true);
    if (Result.Num() > Range + 1)
    {
        Result.SetNum(Range + 1);
    }

    if (Result.Num() > 0 && Result[0] == Character->CurrentTile)
    {
        Result.RemoveAt(0);
    }

    return Result;
}
}

bool ARoundLandsplitterAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    return OwningCharacter && OwningCharacter->CurrentTile && TargetTile && TargetTile != OwningCharacter->CurrentTile && IsTargetInRange(TargetTile);
}

void ARoundLandsplitterAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    const ARoundGameState* GameState = OwningCharacter ? OwningCharacter->GetRoundGameState() : nullptr;
    if (!GameState || !TargetTile)
    {
        return;
    }

    const TArray<ARoundGridTile*> CenterLine = BuildCentralLine(OwningCharacter, const_cast<ARoundGridTile*>(TargetTile), GetAbilityRange());
    for (ARoundGridTile* Tile : CenterLine)
    {
        if (!Tile)
        {
            continue;
        }

        OutTiles.AddUnique(Tile);
        for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(Tile))
        {
            if (Neighbor)
            {
                OutTiles.AddUnique(Neighbor);
            }
        }
    }
}

FRoundActionResult ARoundLandsplitterAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterGeomancer* Geomancer = Cast<ARoundCharacterGeomancer>(OwningCharacter);
    ARoundGameState* GameState = OwningCharacter ? OwningCharacter->GetRoundGameState() : nullptr;
    if (!Geomancer || !GameState || !TargetTile || !Geomancer->CurrentTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    TArray<ARoundGridTile*> AffectedTiles;
    GetPreviewTiles(TargetTile, AffectedTiles);
    const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    TArray<ARoundCharacter*> HitTargets;

    for (ARoundGridTile* Tile : AffectedTiles)
    {
        ARoundCharacter* TargetCharacter = Tile ? Tile->OccupyingCharacter : nullptr;
        if (!TargetCharacter || !Geomancer->IsEnemyFor(TargetCharacter) || HitTargets.Contains(TargetCharacter))
        {
            continue;
        }

        bool bDidDie = false;
        int32 Damage = GetAbilityDamage();
        if (Geomancer->HasGeomancerMarkOn(TargetCharacter)) Damage += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusDamageVsMarked"))));
        const int32 AppliedDamage = ApplyAbilityDamageToTarget(TargetCharacter, Damage, DefenseIgnore, bDidDie);
        if (AppliedDamage <= 0)
        {
            continue;
        }

        if (!bDidDie)
        {
            Geomancer->ApplyGeomancerMark(TargetCharacter);
            if (TargetCharacter->Attributes) TargetCharacter->Attributes->PassInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit")))));
            HitTargets.Add(TargetCharacter);
        }
    }

    for (ARoundCharacter* TargetCharacter : HitTargets)
    {
        if (!TargetCharacter || !TargetCharacter->CurrentTile)
        {
            continue;
        }

        const int32 PushDistance = FMath::Max(1, 1 + FMath::RoundToInt(GetAbilityValueFromKey(TEXT("PushDistance"))));
        for (int32 Step = 0; Step < PushDistance; ++Step)
        {
            ARoundGridTile* PushTile = FindPushBackTile(GameState, Geomancer->CurrentTile, TargetCharacter->CurrentTile);
            if (!PushTile || !TargetCharacter->TryForcedSetField(PushTile, OwningCharacter, true)) break;
        }
    }

    Result.bSuccess = HitTargets.Num() > 0;
    Result.Reason = FString::Printf(TEXT("Landsplitter hit %d target(s)."), HitTargets.Num());
    return Result;
}
