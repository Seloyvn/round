#include "Abilities/RoundRescueAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"
#include "Actors/RoundEffectActor.h"

FRoundActionResult ARoundRescueAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes || !OwningCharacter->CurrentTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState();
    if (!TargetCharacter || !RoundGameState)
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

    if (ARoundGridTile* RescueTile = RoundGameState->GetCloseFreeFieldFrom(OwningCharacter->CurrentTile, TargetCharacter->CurrentTile))
    {
        TargetCharacter->SetField(RescueTile, true);
    }

    const int32 HealAmount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("Heal"))));
    if (HealAmount > 0) TargetCharacter->Heal(HealAmount);
    if (TargetCharacter->Attributes) TargetCharacter->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("TargetInitiativeGain")))));
    if (GetAbilityValueFromKey(TEXT("RemoveDebuff")) > 0.0)
    {
        for (ARoundEffectActor* Effect : TargetCharacter->ActiveEffects)
        {
            if (Effect && Effect->IsDebuff() && Effect->IsRemovable()) { Effect->RemoveEffect(); break; }
        }
    }

    NotifyAllyAffectedBySpell(TargetCharacter);
    Result.bSuccess = true;
    Result.Reason = TEXT("Rescue executed.");
    return Result;
}
