#include "Abilities/RoundDetonateRocksAbility.h"

#include "Actors/RoundCharacterGeomancer.h"
#include "Actors/RoundGridTile.h"
#include "Data/RoundTerrainHelpers.h"
#include "Components/RoundAttributeSetComponent.h"

bool ARoundDetonateRocksAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    return OwningCharacter && OwningCharacter->CurrentTile && TargetTile && IsTargetInRange(TargetTile);
}

void ARoundDetonateRocksAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    if (!TargetTile)
    {
        return;
    }

    OutTiles.Add(const_cast<ARoundGridTile*>(TargetTile));
    OutTiles.Append(TargetTile->GetDistanceFields(FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AOERange"))))));
}

FRoundActionResult ARoundDetonateRocksAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterGeomancer* Geomancer = Cast<ARoundCharacterGeomancer>(OwningCharacter);
    if (!Geomancer || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    const int32 DetonationRange = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AOERange"))));
    const int32 ExplosionRadius = FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ExplosionRadius"))));
    const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    int32 TriggeredRocks = 0;
    const bool bPreserveRocks = GetAbilityValueFromKey(TEXT("PreserveRocks")) > 0.0;

    for (ARoundGridTile* Candidate : TargetTile->GetDistanceFields(DetonationRange))
    {
        if (!Candidate || Candidate->TerrainType != ERoundTerrainType::Rocks)
        {
            continue;
        }

        for (ARoundGridTile* ExplosionTile : Candidate->GetDistanceFields(ExplosionRadius))
        {
            ARoundCharacter* TargetCharacter = ExplosionTile ? ExplosionTile->OccupyingCharacter : nullptr;
            if (!TargetCharacter || !Geomancer->IsEnemyFor(TargetCharacter))
            {
                continue;
            }

            bool bDidDie = false;
            const int32 AppliedDamage = ApplyAbilityDamageToTarget(TargetCharacter, GetAbilityDamage(), DefenseIgnore, bDidDie);
            if (AppliedDamage > 0 && !bDidDie)
            {
                Geomancer->ApplyGeomancerMark(TargetCharacter);
                if (TargetCharacter->Attributes) TargetCharacter->Attributes->PassInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit")))));
            }
        }

        if (!bPreserveRocks) Candidate->SetTerrainType(ERoundTerrainType::Rubble);
        ++TriggeredRocks;
    }

    const int32 ReductionPerRock = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionPerRock"))));
    if (ReductionPerRock > 0) ReduceCooldown(FMath::Min(120, TriggeredRocks * ReductionPerRock));

    Result.bSuccess = TriggeredRocks > 0;
    Result.Reason = FString::Printf(TEXT("Detonate triggered %d rock(s)."), TriggeredRocks);
    return Result;
}
