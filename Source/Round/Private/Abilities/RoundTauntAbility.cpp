#include "Abilities/RoundTauntAbility.h"

FRoundActionResult ARoundTauntAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 10)
    {
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    }
    return ExecuteDefaultAbility(TargetTile);
}
