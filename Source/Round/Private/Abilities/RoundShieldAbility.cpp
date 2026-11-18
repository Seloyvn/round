#include "Abilities/RoundShieldAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundShieldEffect.h"

FRoundActionResult ARoundShieldAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!TargetCharacter)
    {
        Result.Reason = TEXT("No target.");
        return Result;
    }

    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    SetCooldown(GetAbilityCooldown());

    ARoundShieldEffect* Effect = GetWorld() ? GetWorld()->SpawnActor<ARoundShieldEffect>(ARoundShieldEffect::StaticClass()) : nullptr;
    if (!Effect)
    {
        Result.Reason = TEXT("Failed to create shield effect.");
        return Result;
    }

    Effect->EffectId = TEXT("Shield");
    Effect->DisplayName = TEXT("Shield");
    Effect->Description = TEXT("Verhindert eingehenden Schaden.");
    Effect->ShieldAmountRemaining = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ShieldAmount"))));
    Effect->CauserCharacter = OwningCharacter;
    Effect->SetDurationInitiative(FMath::Max(1, GetAbilityDurationInitiativeFromKey(TEXT("DurationInitiative"))), false);
    TargetCharacter->AddEffect(Effect);

    const int32 HealAmount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("Heal"))));
    if (HealAmount > 0) TargetCharacter->Heal(HealAmount);
    if (TargetCharacter->Attributes) TargetCharacter->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("TargetInitiativeGain")))));

    NotifyAllyAffectedBySpell(TargetCharacter);
    Result.bSuccess = true;
    Result.Reason = TEXT("Shield applied.");
    return Result;
}
