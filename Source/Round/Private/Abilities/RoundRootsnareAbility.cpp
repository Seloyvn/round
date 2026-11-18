#include "Abilities/RoundRootsnareAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Effects/RoundRootEffect.h"
#include "Data/RoundTerrainHelpers.h"

FRoundActionResult ARoundRootsnareAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!OwningCharacter || !TargetTile || !TargetCharacter || !OwningCharacter->IsEnemyFor(TargetCharacter))
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    if (GetAbilityDamage() > 0)
    {
        bool bDidDie = false;
        ApplyAbilityDamageToTarget(TargetCharacter, GetAbilityDamage(), FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore")))), bDidDie);
    }

    if (ARoundRootEffect* RootEffect = GetWorld()->SpawnActor<ARoundRootEffect>(ARoundRootEffect::StaticClass()))
    {
        RootEffect->EffectId = TEXT("Rootsnare");
        RootEffect->DisplayName = TEXT("Rooted");
        RootEffect->Description = TEXT("Kann sich nicht bewegen.");
        int32 Duration = FMath::Max(1, GetAbilityDurationInitiativeFromKey(TEXT("DurationInitiative")));
        if (TargetTile && RoundTerrain::IsNatureTerrain(TargetTile->TerrainType))
        {
            Duration += FMath::Max(0, GetAbilityDurationInitiativeFromKey(TEXT("BonusDurationOnNatureInitiative")));
            Duration += FMath::Max(0, GetAbilityDurationInitiativeFromKey(TEXT("BonusDurationInitiative")));
        }
        RootEffect->CauserCharacter = OwningCharacter;
        const double DamageMultiplierAdd = GetAbilityValueFromKey(TEXT("IncomingDamageMultiplierFromCauserAdd"));
        RootEffect->IncomingDamageMultiplierFromCauser = 1.0 + FMath::Max(0.0, DamageMultiplierAdd);
        RootEffect->InitiativeLossOnExpire = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnExpire"))));
        RootEffect->SetDurationInitiative(Duration > 0 ? Duration : 40, false);
        TargetCharacter->AddEffect(RootEffect);
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Target rooted.");
    return Result;
}
