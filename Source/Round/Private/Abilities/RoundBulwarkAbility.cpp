#include "Abilities/RoundBulwarkAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundModifierEffect.h"

FRoundActionResult ARoundBulwarkAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 3)
    {
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    }

    FRoundActionResult Result;
    if (!OwningCharacter || !OwningCharacter->Attributes)
    {
        Result.Reason = TEXT("No owning character.");
        return Result;
    }

    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    SetCooldown(GetAbilityCooldown());

    ARoundModifierEffect* Effect = OwningCharacter->GetWorld() ? OwningCharacter->GetWorld()->SpawnActor<ARoundModifierEffect>(ARoundModifierEffect::StaticClass()) : nullptr;
    if (!Effect)
    {
        Result.Reason = TEXT("Failed to create bulwark effect.");
        return Result;
    }

    Effect->EffectId = TEXT("Bulwark");
    Effect->DisplayName = TEXT("Bulwark");
    Effect->Description = TEXT("Erleidet weniger Schaden und verursacht mehr Schaden.");
    Effect->IncomingDamageMultiplier = GetAbilityValueFromKey(TEXT("SelfIncomingDamageMultiplier")) > 0.0 ? GetAbilityValueFromKey(TEXT("SelfIncomingDamageMultiplier")) : 1.0;
    Effect->OutgoingDamageMultiplier = GetAbilityValueFromKey(TEXT("OutgoingDamageMultiplier")) > 0.0 ? GetAbilityValueFromKey(TEXT("OutgoingDamageMultiplier")) : 1.0;
    Effect->DefenseIgnoreAdd = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    Effect->MoveCostAdd = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MoveCostAdd")));
    Effect->HealFractionFromDamageDealt = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("HealFractionFromDamageDealt")));
    Effect->SetDurationInitiative(FMath::Max(1, GetAbilityDurationInitiativeFromKey(TEXT("DurationInitiative"))), false);
    Effect->CauserCharacter = OwningCharacter;
    OwningCharacter->AddEffect(Effect);

    Result.bSuccess = true;
    Result.Reason = TEXT("Bulwark applied.");
    return Result;
}
