#include "Abilities/RoundDragoonHoldTheLineAbility.h"

#include "Actors/RoundCharacter.h"
#include "Effects/RoundDragoonThreatEffect.h"

bool ARoundDragoonHoldTheLineAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    return OwningCharacter
        && OwningCharacter->CurrentTile
        && GetAbilityVisibility() == ERoundButtonState::Enabled;
}

FRoundActionResult ARoundDragoonHoldTheLineAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!OwningCharacter || !OwningCharacter->GetWorld())
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundDragoonThreatEffect* ThreatEffect = OwningCharacter->GetWorld()->SpawnActor<ARoundDragoonThreatEffect>(ARoundDragoonThreatEffect::StaticClass());
    if (!ThreatEffect)
    {
        Result.Reason = TEXT("Failed to create threat effect.");
        return Result;
    }

    ThreatEffect->EffectId = TEXT("DragoonThreat");
    ThreatEffect->DisplayName = TEXT("Hold the Line");
    ThreatEffect->Description = TEXT("Trifft Gegner, die die AttackRange verlassen.");
    ThreatEffect->CauserCharacter = OwningCharacter;
    ThreatEffect->TriggerDamage = GetAbilityDamage();
    ThreatEffect->TriggerDamageType = GetAbilityDamageType();
    ThreatEffect->TriggerDefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    ThreatEffect->ThreatRangeAdd = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ThreatRangeAdd"))));
    ThreatEffect->InitiativeLossOnTrigger = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnTrigger"))));
    const double IncomingMultiplierAdd = GetAbilityValueFromKey(TEXT("IncomingDamageMultiplierAdd"));
    ThreatEffect->IncomingDamageMultiplier = FMath::Max(0.0, 1.0 + IncomingMultiplierAdd);
    ThreatEffect->MaxTriggers = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MaxTriggers"))));
    ThreatEffect->bAutoExpireOnTurnEnd = false;
    OwningCharacter->AddEffect(ThreatEffect);

    OwningCharacter->EndTurn();
    Result.bSuccess = true;
    Result.Reason = TEXT("Hold the Line executed.");
    return Result;
}
