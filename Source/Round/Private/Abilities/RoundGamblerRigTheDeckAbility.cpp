#include "Abilities/RoundGamblerRigTheDeckAbility.h"

#include "Actors/RoundCharacterGambler.h"

bool ARoundGamblerRigTheDeckAbility::RequiresHandCardSelection() const
{
    return true;
}

int32 ARoundGamblerRigTheDeckAbility::GetMinHandCardSelections() const
{
    return 1;
}

int32 ARoundGamblerRigTheDeckAbility::GetMaxHandCardSelections() const
{
    return 1;
}

bool ARoundGamblerRigTheDeckAbility::IsValidHandCardSelectionIndex(int32 CardIndex) const
{
    const ARoundCharacterGambler* Gambler = Cast<ARoundCharacterGambler>(OwningCharacter);
    return Gambler && Gambler->HasHandCardIndex(CardIndex);
}

FRoundActionResult ARoundGamblerRigTheDeckAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterGambler* Gambler = Cast<ARoundCharacterGambler>(OwningCharacter);
    if (!Gambler)
    {
        Result.Reason = TEXT("Only the Gambler can use this ability.");
        return Result;
    }

    if (PendingHandSelectionIndices.Num() != 1 || !Gambler->HasHandCardIndex(PendingHandSelectionIndices[0]))
    {
        Result.Reason = TEXT("Rig the Deck requires one selected hand card.");
        return Result;
    }

    if (!ConsumeAbilityActivation(TargetTile, Result))
    {
        return Result;
    }

    FRoundPlayingCard TransformedCard;
    if (!Gambler->TransformHandCardFromDeckProfile(PendingHandSelectionIndices[0], TransformedCard))
    {
        Result.Reason = TEXT("The selected card could not be transformed.");
        return Result;
    }

    Gambler->ImproveHandCard(PendingHandSelectionIndices[0], FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RankAdd"))), FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MinimumResultRank"))));
    Gambler->DrawCards(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DrawAfterRig")))));
    ReduceCooldown(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionOnRig")))));

    ClearPendingHandSelectionIndices();
    Result.bSuccess = true;
    Result.Reason = TEXT("Rig the Deck transformed the selected card.");
    return Result;
}
