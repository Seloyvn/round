#include "Abilities/RoundBorrowedTimeAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Effects/RoundDeferredDamageEffect.h"

FRoundActionResult ARoundBorrowedTimeAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!OwningCharacter || !TargetCharacter || !GetWorld())
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundDeferredDamageEffect* DeferredDamageEffect = GetWorld()->SpawnActor<ARoundDeferredDamageEffect>(ARoundDeferredDamageEffect::StaticClass());
    if (!DeferredDamageEffect)
    {
        Result.Reason = TEXT("Failed to create effect.");
        return Result;
    }

    DeferredDamageEffect->EffectId = TEXT("BorrowedTime");
    DeferredDamageEffect->DisplayName = TEXT("Borrowed Time");
    DeferredDamageEffect->Description = TEXT("Schiebt einen Teil des Schadens bis spaeter auf.");
    DeferredDamageEffect->CauserCharacter = OwningCharacter;
    const double DeferredDamageFraction = GetAbilityValueFromKey(TEXT("DeferredDamageFraction"));
    const int32 DurationInitiative = GetAbilityDurationInitiativeFromKey(TEXT("DurationInitiative"));
    DeferredDamageEffect->DeferredDamageFraction = FMath::Clamp(DeferredDamageFraction > 0.0 ? DeferredDamageFraction : 0.5, 0.0, 1.0);
    DeferredDamageEffect->DeferredDamageForgivenFraction = FMath::Clamp(GetAbilityValueFromKey(TEXT("DeferredDamageForgivenFraction")), 0.0, 1.0);
    DeferredDamageEffect->HealOnResolve = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HealOnResolve"))));
    DeferredDamageEffect->InitiativeGainOnResolve = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnResolve"))));
    DeferredDamageEffect->SetDurationInitiative(FMath::Max(1, DurationInitiative > 0 ? DurationInitiative : 60), false);
    TargetCharacter->AddEffect(DeferredDamageEffect);
    NotifyAllyAffectedBySpell(TargetCharacter);

    Result.bSuccess = true;
    Result.Reason = TEXT("Borrowed Time applied.");
    return Result;
}
