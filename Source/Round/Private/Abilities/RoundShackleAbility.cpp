#include "Abilities/RoundShackleAbility.h"

FRoundActionResult ARoundShackleAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 8)
    {
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    }
    return ExecuteDefaultAbility(TargetTile);
}
