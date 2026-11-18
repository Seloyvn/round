#include "Abilities/RoundBearFormAbility.h"

#include "Actors/RoundCharacterDruid.h"

FRoundActionResult ARoundBearFormAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!OwningCharacter)
    {
        Result.Reason = TEXT("No owning character.");
        return Result;
    }

    Result = ExecuteDefaultAbility(OwningCharacter->CurrentTile);
    if (!Result.bSuccess)
    {
        return Result;
    }

    ARoundCharacterDruid* Druid = Cast<ARoundCharacterDruid>(OwningCharacter);
    if (!Druid)
    {
        Result.bSuccess = false;
        Result.Reason = TEXT("Only druids can use bear form.");
        return Result;
    }

    Result.bSuccess = Druid->EnterBearForm(this);
    Result.Reason = Result.bSuccess ? TEXT("Bear form activated.") : TEXT("Bear form is already active.");
    return Result;
}
