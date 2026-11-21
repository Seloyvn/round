#include "Abilities/RoundSanctifyGroundAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundSanctifiedGroundAuraEffect.h"

FRoundActionResult ARoundSanctifyGroundAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 5)
    {
        return Super::ExecuteAbility(TargetTile);
    }
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    SetCooldown(GetAbilityCooldown());

    TArray<ARoundEffectActor*> EffectsToRemove;
    for (ARoundEffectActor* Effect : OwningCharacter->ActiveEffects)
    {
        if (ARoundAuraEffect* AuraEffect = Cast<ARoundAuraEffect>(Effect))
        {
            if (AuraEffect->EffectId == FName(TEXT("SanctifyGround")))
            {
                EffectsToRemove.Add(AuraEffect);
            }
        }
    }

    for (ARoundEffectActor* Effect : EffectsToRemove)
    {
        if (Effect)
        {
            Effect->RemoveEffect();
        }
    }

    ARoundSanctifiedGroundAuraEffect* AuraEffect = OwningCharacter->GetWorld() ? OwningCharacter->GetWorld()->SpawnActor<ARoundSanctifiedGroundAuraEffect>(ARoundSanctifiedGroundAuraEffect::StaticClass()) : nullptr;
    if (!AuraEffect)
    {
        Result.Reason = TEXT("Failed to create sanctify ground.");
        return Result;
    }

    AuraEffect->EffectId = TEXT("SanctifyGround");
    AuraEffect->DisplayName = TEXT("Sanctify Ground");
    AuraEffect->Description = TEXT("Fuegt Gegnern in der Zone heiligen Schaden zu.");
    AuraEffect->Radius = FMath::Max(0, GetAbilityRange() > 0 ? FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AuraRadius"))) : FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AuraRadius"))));
    AuraEffect->bAffectEnemies = true;
    AuraEffect->bAffectAllies = false;
    AuraEffect->bAffectSelf = false;
    AuraEffect->bAuraUsesFixedPosition = GetAbilityValueFromKey(TEXT("FollowCaster")) <= 0.0;
    AuraEffect->AuraFixedPosition = TargetTile->GridPosition;
    AuraEffect->AuraTriggerDamage = GetAbilityDamage();
    AuraEffect->AuraTriggerDamageType = GetAbilityDamageType();
    AuraEffect->InitiativeLossPerPulse = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossPerPulse"))));
    AuraEffect->SetDurationInitiative(FMath::Max(1, GetAbilityDurationInitiativeFromKey(TEXT("DurationInitiative"))), false);
    AuraEffect->CauserCharacter = OwningCharacter;
    OwningCharacter->AddEffect(AuraEffect);

    Result.bSuccess = true;
    Result.Reason = TEXT("Sanctify Ground created.");
    return Result;
}
