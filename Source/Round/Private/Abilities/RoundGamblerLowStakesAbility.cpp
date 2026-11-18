#include "Abilities/RoundGamblerLowStakesAbility.h"

#include "Actors/RoundCharacterGambler.h"
#include "Components/RoundAttributeSetComponent.h"

FRoundActionResult ARoundGamblerLowStakesAbility::ExecuteAbility(ARoundGridTile* TargetTile)
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

    const int32 ConfiguredMinimumKeptRank = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MinimumKeptRank")));
    const int32 MinimumKeptRank = FMath::Max(2, ConfiguredMinimumKeptRank > 0 ? ConfiguredMinimumKeptRank : 10);
    const int32 DiscardedCards = Gambler->DiscardHandCardsBelowRank(MinimumKeptRank);
    const int32 DrawnCards = Gambler->DrawCards(DiscardedCards);
    const int32 ExtraDrawThreshold = FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ExtraDrawThreshold"))));
    if (DiscardedCards >= ExtraDrawThreshold) Gambler->DrawCards(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ExtraDrawCount")))));
    if (Gambler->Attributes) Gambler->Attributes->GainInitiative(FMath::Min(25, DiscardedCards * FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativePerDiscard"))))));
    const int32 HealPerDiscard = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HealPerDiscard"))));
    if (HealPerDiscard > 0) Gambler->Heal(HealPerDiscard * DiscardedCards);
    ReduceCooldown(FMath::Min(100, DiscardedCards * FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionPerDiscard"))))));
    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Discarded %d low cards and drew %d new cards."), DiscardedCards, DrawnCards);
    return Result;
}
