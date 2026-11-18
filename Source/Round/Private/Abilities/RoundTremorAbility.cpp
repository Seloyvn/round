#include "Abilities/RoundTremorAbility.h"

#include "Actors/RoundCharacterGeomancer.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundTerrainHelpers.h"

bool ARoundTremorAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    return OwningCharacter && OwningCharacter->CurrentTile && GetAbilityVisibility() == ERoundButtonState::Enabled;
}

void ARoundTremorAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    if (!OwningCharacter || !OwningCharacter->CurrentTile)
    {
        return;
    }

    OutTiles.Append(OwningCharacter->CurrentTile->GetDistanceFields(FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AOERange"))))));
}

FRoundActionResult ARoundTremorAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterGeomancer* Geomancer = Cast<ARoundCharacterGeomancer>(OwningCharacter);
    if (!Geomancer || !Geomancer->CurrentTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    const int32 Radius = FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AOERange"))));
    const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    int32 HitCount = 0;

    for (ARoundGridTile* Tile : Geomancer->CurrentTile->GetDistanceFields(Radius))
    {
        ARoundCharacter* TargetCharacter = Tile ? Tile->OccupyingCharacter : nullptr;
        if (!TargetCharacter || !Geomancer->IsEnemyFor(TargetCharacter))
        {
            continue;
        }

        bool bDidDie = false;
        int32 Damage = GetAbilityDamage();
        if (Geomancer->HasGeomancerMarkOn(TargetCharacter))
        {
            Damage += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusDamageVsMarked"))));
        }
        const int32 AppliedDamage = ApplyAbilityDamageToTarget(TargetCharacter, Damage, DefenseIgnore, bDidDie);
        if (AppliedDamage > 0)
        {
            if (!bDidDie)
            {
                Geomancer->ApplyGeomancerMark(TargetCharacter);
                if (TargetCharacter->Attributes) TargetCharacter->Attributes->PassInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit")))));
            }
            ++HitCount;
        }
    }

    const int32 InitiativePerTarget = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativePerTargetHit"))));
    if (Geomancer->Attributes && InitiativePerTarget > 0) Geomancer->Attributes->GainInitiative(FMath::Min(30, HitCount * InitiativePerTarget));

    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Tremor hit %d target(s)."), HitCount);
    return Result;
}
