#include "Abilities/RoundDeathMarkAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundEffectActor.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundDeathMarkEffect.h"
#include "Effects/RoundInvisibilityEffect.h"

FRoundActionResult ARoundDeathMarkAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 20)
    {
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    }
    FRoundActionResult Result;
    ARoundCharacter* Target = ResolveTargetCharacter(TargetTile);
    if (!Target) return Result;

    const bool bWasDebuffed = Target->ActiveEffects.ContainsByPredicate([](const ARoundEffectActor* Effect)
    {
        return Effect && Effect->IsDebuff();
    });
    if (!ConsumeAbilityActivation(TargetTile, Result)) return Result;

    if (bWasDebuffed && OwningCharacter && OwningCharacter->Attributes)
    {
        OwningCharacter->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeRefundVsDebuffed")))));
    }

    const TArray<TObjectPtr<ARoundEffectActor>> ExistingEffects = Target->ActiveEffects;
    for (ARoundEffectActor* Existing : ExistingEffects)
    {
        if (Existing && Existing->EffectId == FName(TEXT("DeathMark")) && Existing->CauserCharacter == OwningCharacter)
        {
            Existing->RemoveEffect();
        }
    }

    if (ARoundDeathMarkEffect* Mark = GetWorld()->SpawnActor<ARoundDeathMarkEffect>())
    {
        Mark->EffectId = TEXT("DeathMark");
        Mark->DisplayName = TEXT("Death Mark");
        Mark->Description = TEXT("Erhoeht waehrend der gesamten Dauer den Schaden von Character 3 gegen dieses Ziel.");
        Mark->CauserCharacter = OwningCharacter;
        Mark->ComboGainOnTrigger = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ComboGainOnTrigger"))));
        Mark->DamageMultiplierFromCauser = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("DamageMultiplierFromCauser")));
        Mark->DefenseIgnoreFromCauser = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnoreFromCauser"))));
        Mark->MoveDurationExtension = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MoveDurationExtension"))));
        Mark->ExecuteCooldownReductionOnMaxCombo = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ExecuteCooldownReductionOnMaxCombo"))));
        Mark->DeathMarkCooldownReductionOnExpiry = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DeathMarkCooldownReductionOnExpiry"))));
        Mark->TransferRangeOnKill = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("TransferRangeOnKill"))));
        Mark->LowHealthThreshold = FMath::Clamp(GetAbilityValueFromKey(TEXT("LowHealthThreshold")), 0.0, 1.0);
        Mark->LowHealthDamageMultiplierAdd = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("LowHealthDamageMultiplierAdd")));
        Mark->bPreventInvisibility = GetAbilityValueFromKey(TEXT("PreventInvisibility")) > 0.0;
        Mark->bGrantComboEachCauserTurn = GetAbilityValueFromKey(TEXT("GrantComboEachCauserTurn")) > 0.0;
        Mark->SetDurationInitiative(FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DurationInitiative")))), false);

        if (Mark->bPreventInvisibility)
        {
            const TArray<TObjectPtr<ARoundEffectActor>> EffectsBeforeMark = Target->ActiveEffects;
            for (ARoundEffectActor* Existing : EffectsBeforeMark)
            {
                if (Existing && Existing->IsA<ARoundInvisibilityEffect>()) Existing->RemoveEffect();
            }
        }
        Target->AddEffect(Mark);
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Death Mark applied.");
    return Result;
}
