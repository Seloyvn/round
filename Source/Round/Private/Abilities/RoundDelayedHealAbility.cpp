#include "Abilities/RoundDelayedHealAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"

FRoundActionResult ARoundDelayedHealAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    return ExecuteDefaultAbility(TargetTile);
}

FRoundActionResult ARoundDelayedHealAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!TargetCharacter)
    {
        Result.Reason = TEXT("No target.");
        return Result;
    }

    const int32 HealAmount = FMath::Max(
        0,
        FMath::RoundToInt(
            GetAbilityValueFromKey(TEXT("BaseHeal")) > 0.0
                ? GetAbilityValueFromKey(TEXT("BaseHeal"))
                : GetAbilityValueFromKey(TEXT("Heal"))));
    int32 FinalHeal = HealAmount;
    if (TargetCharacter->Attributes && TargetCharacter->Attributes->CurrentHealth * 2 <= TargetCharacter->Attributes->BaseStats.MaxHealth)
    {
        FinalHeal += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusHealBelowHalf"))));
    }
    if (FinalHeal > 0)
    {
        TargetCharacter->Heal(FinalHeal);
    }
    if (TargetCharacter->Attributes) TargetCharacter->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("TargetInitiativeGain")))));

    NotifyAllyAffectedBySpell(TargetCharacter);
    Result.bSuccess = true;
    Result.Reason = TEXT("Delayed heal resolved.");
    return Result;
}
