#include "Abilities/RoundSummonBladeAbility.h"

#include "Actors/RoundCharacterBladecaller.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"

bool ARoundSummonBladeAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    const ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    return Bladecaller
        && TargetTile
        && IsTargetInRange(TargetTile)
        && GetAbilityVisibility() == ERoundButtonState::Enabled
        && !TargetTile->OccupyingFieldObject;
}

FRoundActionResult ARoundSummonBladeAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    if (!Bladecaller || !Bladecaller->Attributes || !CanTargetTile(TargetTile))
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    if (!Bladecaller->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    SetCooldown(GetAbilityCooldown());

    if (!Bladecaller->SummonBladeAtTile(TargetTile))
    {
        Result.Reason = TEXT("Failed to summon blade.");
        return Result;
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Blade summoned.");
    return Result;
}
