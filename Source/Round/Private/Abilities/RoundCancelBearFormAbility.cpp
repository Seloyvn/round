#include "Abilities/RoundCancelBearFormAbility.h"

#include "Actors/RoundCharacterDruid.h"

bool ARoundCancelBearFormAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    const ARoundCharacterDruid* Druid = Cast<ARoundCharacterDruid>(OwningCharacter);
    return Druid && Druid->IsBearFormActive();
}

FRoundActionResult ARoundCancelBearFormAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterDruid* Druid = Cast<ARoundCharacterDruid>(OwningCharacter);
    if (!Druid || !Druid->IsBearFormActive())
    {
        Result.Reason = TEXT("Bear form is not active.");
        return Result;
    }

    Result.bSuccess = Druid->ExitBearForm();
    Result.Reason = Result.bSuccess ? TEXT("Bear form canceled.") : TEXT("Failed to cancel bear form.");
    return Result;
}
