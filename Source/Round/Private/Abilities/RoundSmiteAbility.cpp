#include "Abilities/RoundSmiteAbility.h"
#include "Actors/RoundCharacter.h"
#include "Components/RoundAttributeSetComponent.h"

FRoundActionResult ARoundSmiteAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 1)
    {
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    }

    FRoundActionResult Result = ExecuteDefaultAbility(TargetTile);
    if (Result.bSuccess && OwningCharacter && OwningCharacter->Attributes)
    {
        OwningCharacter->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnHit")))));
        OwningCharacter->Heal(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("SelfHealOnHit")))));
    }
    return Result;
}
