#include "Abilities/RoundGamblerDoubleDrawAbility.h"

#include "Actors/RoundCharacterGambler.h"
#include "Components/RoundAttributeSetComponent.h"

FRoundActionResult ARoundGamblerDoubleDrawAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterGambler* Gambler = Cast<ARoundCharacterGambler>(OwningCharacter);
    if (!Gambler)
    {
        Result.Reason = TEXT("Only the Gambler can use this ability.");
        return Result;
    }

    if (!ConsumeAbilityActivation(TargetTile, Result))
    {
        return Result;
    }

    const int32 ConfiguredDrawCount = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DrawCount")));
    const int32 DrawCount = FMath::Max(1, ConfiguredDrawCount > 0 ? ConfiguredDrawCount : 2);
    const int32 DrawnCards = Gambler->DrawCards(DrawCount);
    const int32 InitiativePerCard = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativePerCardDrawn"))));
    if (Gambler->Attributes && InitiativePerCard > 0) Gambler->Attributes->GainInitiative(FMath::Min(25, InitiativePerCard * DrawnCards));
    const int32 HealPerCard = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HealPerCardDrawn"))));
    if (HealPerCard > 0) Gambler->Heal(HealPerCard * DrawnCards);
    if (DrawnCards >= DrawCount) ReduceCooldown(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionOnFullDraw")))));
    Result.bSuccess = DrawnCards > 0;
    Result.Reason = Result.bSuccess
        ? FString::Printf(TEXT("Drew %d cards."), DrawnCards)
        : TEXT("No cards could be drawn.");
    return Result;
}
