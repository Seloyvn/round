#include "Abilities/RoundGamblerFlushAbility.h"

#include "Actors/RoundCharacterGambler.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"

bool ARoundGamblerFlushAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    const ARoundCharacterGambler* Gambler = Cast<ARoundCharacterGambler>(OwningCharacter);
    if (!Gambler || Gambler->GetHandCount() <= 0 || !TargetTile)
    {
        return false;
    }

    ERoundCardSuit DominantSuit = ERoundCardSuit::Hearts;
    if (!Gambler->GetMostCommonSuit(DominantSuit))
    {
        return false;
    }

    ARoundCharacter* Occupant = TargetTile->OccupyingCharacter;
    switch (DominantSuit)
    {
    case ERoundCardSuit::Hearts:
    case ERoundCardSuit::Diamonds:
        return Occupant && Occupant->Team == OwningCharacter->Team;
    case ERoundCardSuit::Spades:
    case ERoundCardSuit::Clubs:
    default:
        return Occupant && OwningCharacter->IsEnemyFor(Occupant) && IsTargetInRange(TargetTile);
    }
}

FRoundActionResult ARoundGamblerFlushAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterGambler* Gambler = Cast<ARoundCharacterGambler>(OwningCharacter);
    if (!Gambler)
    {
        Result.Reason = TEXT("Only the Gambler can use this ability.");
        return Result;
    }

    TArray<FRoundPlayingCard> ConsumedCards;
    ERoundCardSuit ConsumedSuit = ERoundCardSuit::Hearts;
    if (Gambler->GetHandCount() <= 0 || !CanTargetTile(TargetTile))
    {
        Result.Reason = TEXT("Flush target is not legal.");
        return Result;
    }

    if (!ConsumeAbilityActivation(TargetTile, Result))
    {
        return Result;
    }

    if (!Gambler->ConsumeMostCommonSuitCards(ConsumedCards, ConsumedSuit))
    {
        Result.Reason = TEXT("No suit could be consumed.");
        return Result;
    }

    double MultiplierSum = 0.0;
    for (const FRoundPlayingCard& Card : ConsumedCards)
    {
        MultiplierSum += Gambler->GetRankMultiplier(Card.RankValue);
    }

    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!TargetCharacter)
    {
        Result.Reason = TEXT("Flush requires a character target.");
        return Result;
    }

    switch (ConsumedSuit)
    {
    case ERoundCardSuit::Hearts:
    {
        const int32 HealAmount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HeartsHeal")) * MultiplierSum));
        TargetCharacter->Heal(HealAmount);
        Result.Reason = FString::Printf(TEXT("Flush healed for %d."), HealAmount);
        break;
    }
    case ERoundCardSuit::Spades:
    {
        const int32 Damage = FMath::Max(0, FMath::RoundToInt(GetAbilityDamage() * MultiplierSum));
        bool bDidDie = false;
        int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
        const int32 AppliedDamage = ApplyAbilityDamageToTarget(TargetCharacter, Damage, DefenseIgnore, bDidDie);
        Result.Reason = FString::Printf(TEXT("Flush dealt %d damage."), AppliedDamage);
        break;
    }
    case ERoundCardSuit::Clubs:
    {
        const int32 Damage = FMath::Max(0, FMath::RoundToInt(GetAbilityDamage() * MultiplierSum));
        const int32 InitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ClubsInitiativeLoss")) * MultiplierSum));
        bool bDidDie = false;
        int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
        const int32 AppliedDamage = ApplyAbilityDamageToTarget(TargetCharacter, Damage, DefenseIgnore, bDidDie);
        if (InitiativeLoss > 0)
        {
            TargetCharacter->ApplyDelay(InitiativeLoss);
        }
        Result.Reason = FString::Printf(TEXT("Flush dealt %d damage and delayed the target."), AppliedDamage);
        break;
    }
    case ERoundCardSuit::Diamonds:
    default:
    {
        const int32 InitiativeGain = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DiamondsInitiativeGain")) * MultiplierSum));
        if (TargetCharacter->Attributes && InitiativeGain > 0)
        {
            TargetCharacter->Attributes->GainInitiative(InitiativeGain);
        }
        Result.Reason = FString::Printf(TEXT("Flush granted %d initiative."), InitiativeGain);
        break;
    }
    }

    const int32 CardsAfter = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DrawAfterFlush"))));
    if (CardsAfter > 0) Gambler->DrawCards(CardsAfter);
    ReduceCooldown(FMath::Min(120, ConsumedCards.Num() * FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionPerConsumedCard"))))));

    Result.bSuccess = true;
    return Result;
}
