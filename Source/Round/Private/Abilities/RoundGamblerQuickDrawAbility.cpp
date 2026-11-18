#include "Abilities/RoundGamblerQuickDrawAbility.h"

#include "Actors/RoundCharacterGambler.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"

FRoundActionResult ARoundGamblerQuickDrawAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterGambler* Gambler = Cast<ARoundCharacterGambler>(OwningCharacter);
    if (!Gambler)
    {
        Result.Reason = TEXT("Only the Gambler can use this ability.");
        return Result;
    }

    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!TargetCharacter || !OwningCharacter || !OwningCharacter->IsEnemyFor(TargetCharacter))
    {
        Result.Reason = TEXT("Quick Draw requires an enemy target.");
        return Result;
    }

    FRoundPlayingCard PlayedCard;
    if (Gambler->GetHandCount() <= 0)
    {
        Result.Reason = TEXT("No cards in hand.");
        return Result;
    }

    if (!ConsumeAbilityActivation(TargetTile, Result))
    {
        return Result;
    }

    if (!Gambler->ConsumeOldestHandCard(PlayedCard))
    {
        Result.Reason = TEXT("No card could be consumed.");
        return Result;
    }

    const double RankMultiplier = Gambler->GetRankMultiplier(PlayedCard.RankValue);
    int32 Damage = FMath::Max(0, FMath::RoundToInt(GetAbilityDamage() * RankMultiplier));
    const int32 HeartsHeal = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HeartsHeal")) * RankMultiplier));
    const int32 ClubsInitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ClubsInitiativeLoss")) * RankMultiplier));
    const int32 DiamondsInitiativeGain = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DiamondsInitiativeGain")) * RankMultiplier));
    Damage += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("FlatDamageBonus"))));

    switch (PlayedCard.Suit)
    {
    case ERoundCardSuit::Spades:
        Damage += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("SpadesBonusDamage")) * RankMultiplier));
        break;
    case ERoundCardSuit::Hearts:
    case ERoundCardSuit::Clubs:
    case ERoundCardSuit::Diamonds:
    default:
        break;
    }

    bool bDidDie = false;
    int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    const int32 AppliedDamage = ApplyAbilityDamageToTarget(TargetCharacter, Damage, DefenseIgnore, bDidDie);

    if (PlayedCard.Suit == ERoundCardSuit::Hearts && HeartsHeal > 0)
    {
        Gambler->Heal(HeartsHeal);
    }
    else if (PlayedCard.Suit == ERoundCardSuit::Clubs && ClubsInitiativeLoss > 0)
    {
        TargetCharacter->ApplyDelay(ClubsInitiativeLoss);
    }
    else if (PlayedCard.Suit == ERoundCardSuit::Diamonds && DiamondsInitiativeGain > 0 && Gambler->Attributes)
    {
        Gambler->Attributes->GainInitiative(DiamondsInitiativeGain);
    }

    const int32 DrawCount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(bDidDie ? TEXT("DrawOnKill") : TEXT("DrawOnHit"))));
    if (DrawCount > 0) Gambler->DrawCards(DrawCount);
    if (PlayedCard.RankValue >= 11) ReduceCooldown(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionOnFaceCard")))));

    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Quick Draw dealt %d damage."), AppliedDamage);
    return Result;
}
