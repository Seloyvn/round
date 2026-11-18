#include "Abilities/RoundSmokeStepAbility.h"

#include "Abilities/RoundArcaneShiftAbility.h"
#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"

FRoundActionResult ARoundSmokeStepAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes || !TargetTile || TargetTile->OccupyingCharacter)
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
    OwningCharacter->SetField(TargetTile, true);

    Result.bSuccess = true;
    Result.Reason = TEXT("Smoke Step executed.");
    return Result;
}
