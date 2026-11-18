#include "Actors/RoundCharacterGambler.h"

#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Net/UnrealNetwork.h"

namespace
{
int32 SuitTieBreakScore(const ERoundCardSuit Suit)
{
    switch (Suit)
    {
    case ERoundCardSuit::Spades:
        return 4;
    case ERoundCardSuit::Hearts:
        return 3;
    case ERoundCardSuit::Clubs:
        return 2;
    case ERoundCardSuit::Diamonds:
    default:
        return 1;
    }
}
}

void ARoundCharacterGambler::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundCharacterGambler, DeckCards);
    DOREPLIFETIME(ARoundCharacterGambler, HandCards);
    DOREPLIFETIME(ARoundCharacterGambler, DiscardCards);
    DOREPLIFETIME(ARoundCharacterGambler, NextCardInstanceId);
}

void ARoundCharacterGambler::InitializeCharacter()
{
    Super::InitializeCharacter();

    if (!HasAuthority())
    {
        return;
    }

    if (DeckCards.Num() == 0 && HandCards.Num() == 0 && DiscardCards.Num() == 0)
    {
        BuildStartingDeck();
        DrawCards(GetStartingHandCards());
    }
}

void ARoundCharacterGambler::BeginTurn()
{
    Super::BeginTurn();
    bTookTurnActionThisTurn = false;

    if (HasAuthority())
    {
        DrawCards(GetTurnStartDrawCount());
    }
}

void ARoundCharacterGambler::EndTurn()
{
    if (HasAuthority() && !bTookTurnActionThisTurn)
    {
        DrawCards(GetIdleTurnDrawCount());
    }

    Super::EndTurn();
}

FRoundActionResult ARoundCharacterGambler::AttackField(ARoundGridTile* TargetTile)
{
    const FRoundActionResult Result = Super::AttackField(TargetTile);
    if (Result.bSuccess)
    {
        MarkTurnActionTaken();
    }
    return Result;
}

FRoundActionResult ARoundCharacterGambler::MoveAlongPath(const TArray<ARoundGridTile*>& Path)
{
    const FRoundActionResult Result = Super::MoveAlongPath(Path);
    if (Result.bSuccess)
    {
        MarkTurnActionTaken();
    }
    return Result;
}

FRoundActionResult ARoundCharacterGambler::UseAbilityIndex(int32 AbilityIndex, ARoundGridTile* TargetTile)
{
    const FRoundActionResult Result = Super::UseAbilityIndex(AbilityIndex, TargetTile);
    if (Result.bSuccess)
    {
        MarkTurnActionTaken();
        DrawCards(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("DrawAfterAbilityUse")))));
    }
    return Result;
}

int32 ARoundCharacterGambler::GetMaxHandCards() const
{
    const int32 BaseValue = FMath::RoundToInt(GetValueFromKey(TEXT("MaxHandCards")));
    return FMath::Max(1, (BaseValue > 0 ? BaseValue : 5) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MaxHandCards"))));
}

int32 ARoundCharacterGambler::GetStartingHandCards() const
{
    return FMath::Clamp(5 + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("StartingHandCardsAdd"))), 1, GetMaxHandCards());
}

int32 ARoundCharacterGambler::GetTurnStartDrawCount() const
{
    return FMath::Max(0, 1 + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("TurnStartDrawAdd"))));
}

int32 ARoundCharacterGambler::GetIdleTurnDrawCount() const
{
    return FMath::Max(0, 1 + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("IdleTurnDrawAdd"))));
}

int32 ARoundCharacterGambler::GetDeckCount() const
{
    return DeckCards.Num();
}

int32 ARoundCharacterGambler::GetHandCount() const
{
    return HandCards.Num();
}

int32 ARoundCharacterGambler::GetDiscardCount() const
{
    return DiscardCards.Num();
}

const TArray<FRoundPlayingCard>& ARoundCharacterGambler::GetHandCards() const
{
    return HandCards;
}

double ARoundCharacterGambler::GetRankMultiplier(int32 RankValue) const
{
    return static_cast<double>(FMath::Clamp(RankValue, 2, 14)) / 10.0
        + GetPassiveValueFromKey(TEXT("CardRankMultiplierAdd"));
}

bool ARoundCharacterGambler::HasHandCardIndex(int32 CardIndex) const
{
    return HandCards.IsValidIndex(CardIndex);
}

int32 ARoundCharacterGambler::DrawCards(int32 Count)
{
    if (!HasAuthority() || Count <= 0)
    {
        return 0;
    }

    int32 Drawn = 0;
    for (int32 DrawIndex = 0; DrawIndex < Count; ++DrawIndex)
    {
        FRoundPlayingCard DrawnCard;
        if (!DrawSingleCard(DrawnCard))
        {
            break;
        }

        while (HandCards.Num() >= GetMaxHandCards() && HandCards.Num() > 0)
        {
            DiscardCard(HandCards[0]);
            HandCards.RemoveAt(0);
        }

        HandCards.Add(DrawnCard);
        ++Drawn;
    }

    if (Drawn > 0)
    {
        NotifyRoundStateChanged();
        ForceNetUpdate();
    }

    return Drawn;
}

bool ARoundCharacterGambler::ConsumeOldestHandCard(FRoundPlayingCard& OutCard)
{
    if (!HasAuthority() || HandCards.Num() <= 0)
    {
        return false;
    }

    OutCard = HandCards[0];
    HandCards.RemoveAt(0);
    DiscardCard(OutCard);
    NotifyRoundStateChanged();
    ForceNetUpdate();
    return true;
}

int32 ARoundCharacterGambler::DiscardHandCardsBelowRank(int32 MinimumKeptRank)
{
    if (!HasAuthority())
    {
        return 0;
    }

    TArray<int32> IndicesToDiscard;
    for (int32 CardIndex = 0; CardIndex < HandCards.Num(); ++CardIndex)
    {
        if (HandCards[CardIndex].RankValue < MinimumKeptRank)
        {
            IndicesToDiscard.Add(CardIndex);
        }
    }

    return DiscardSelectedHandCards(IndicesToDiscard);
}

int32 ARoundCharacterGambler::DiscardSelectedHandCards(const TArray<int32>& CardIndices)
{
    if (!HasAuthority() || CardIndices.Num() <= 0)
    {
        return 0;
    }

    TArray<int32> SortedIndices = CardIndices;
    SortedIndices.Sort(TGreater<int32>());
    for (int32 Index = SortedIndices.Num() - 1; Index > 0; --Index)
    {
        if (SortedIndices[Index] == SortedIndices[Index - 1])
        {
            SortedIndices.RemoveAt(Index);
        }
    }

    int32 DiscardedCount = 0;
    for (int32 CardIndex : SortedIndices)
    {
        if (!HandCards.IsValidIndex(CardIndex))
        {
            continue;
        }

        DiscardCard(HandCards[CardIndex]);
        HandCards.RemoveAt(CardIndex);
        ++DiscardedCount;
    }

    if (DiscardedCount > 0)
    {
        const int32 InitiativePerCard = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InitiativePerDiscardedCard"))));
        if (Attributes && InitiativePerCard > 0) Attributes->GainInitiative(FMath::Min(30, InitiativePerCard * DiscardedCount));
        NotifyRoundStateChanged();
        ForceNetUpdate();
    }

    return DiscardedCount;
}

bool ARoundCharacterGambler::ConsumeMostCommonSuitCards(TArray<FRoundPlayingCard>& OutConsumedCards, ERoundCardSuit& OutSuit)
{
    OutConsumedCards.Reset();
    if (!HasAuthority() || HandCards.Num() <= 0 || !GetMostCommonSuit(OutSuit))
    {
        return false;
    }

    for (int32 CardIndex = HandCards.Num() - 1; CardIndex >= 0; --CardIndex)
    {
        if (HandCards[CardIndex].Suit != OutSuit)
        {
            continue;
        }

        OutConsumedCards.Insert(HandCards[CardIndex], 0);
        DiscardCard(HandCards[CardIndex]);
        HandCards.RemoveAt(CardIndex);
    }

    if (OutConsumedCards.Num() > 0)
    {
        NotifyRoundStateChanged();
        ForceNetUpdate();
        return true;
    }

    return false;
}

bool ARoundCharacterGambler::TransformHandCardFromDeckProfile(int32 CardIndex, FRoundPlayingCard& OutTransformedCard)
{
    if (!HasAuthority() || !HandCards.IsValidIndex(CardIndex))
    {
        return false;
    }

    EnsureDeckAvailable();
    if (DeckCards.Num() <= 0)
    {
        return false;
    }

    const FRoundPlayingCard& SampledDeckCard = DeckCards[FMath::RandRange(0, DeckCards.Num() - 1)];
    HandCards[CardIndex].Suit = SampledDeckCard.Suit;
    HandCards[CardIndex].RankValue = SampledDeckCard.RankValue;
    OutTransformedCard = HandCards[CardIndex];
    NotifyRoundStateChanged();
    ForceNetUpdate();
    return true;
}

bool ARoundCharacterGambler::ImproveHandCard(int32 CardIndex, int32 RankAdd, int32 MinimumRank)
{
    if (!HasAuthority() || !HandCards.IsValidIndex(CardIndex)) return false;
    HandCards[CardIndex].RankValue = FMath::Clamp(FMath::Max(HandCards[CardIndex].RankValue + RankAdd, MinimumRank), 2, 14);
    NotifyRoundStateChanged(); ForceNetUpdate(); return true;
}

bool ARoundCharacterGambler::GetMostCommonSuit(ERoundCardSuit& OutSuit) const
{
    if (HandCards.Num() <= 0)
    {
        return false;
    }

    struct FSuitStats
    {
        int32 Count = 0;
        int32 RankTotal = 0;
        int32 FirstIndex = MAX_int32;
    };

    TMap<ERoundCardSuit, FSuitStats> StatsBySuit;
    for (int32 CardIndex = 0; CardIndex < HandCards.Num(); ++CardIndex)
    {
        const FRoundPlayingCard& Card = HandCards[CardIndex];
        FSuitStats& Stats = StatsBySuit.FindOrAdd(Card.Suit);
        ++Stats.Count;
        Stats.RankTotal += Card.RankValue;
        Stats.FirstIndex = FMath::Min(Stats.FirstIndex, CardIndex);
    }

    bool bFound = false;
    FSuitStats BestStats;
    ERoundCardSuit BestSuit = ERoundCardSuit::Hearts;

    for (const TPair<ERoundCardSuit, FSuitStats>& Pair : StatsBySuit)
    {
        const FSuitStats& Stats = Pair.Value;
        const bool bBetterSuit = !bFound
            || Stats.Count > BestStats.Count
            || (Stats.Count == BestStats.Count && Stats.RankTotal > BestStats.RankTotal)
            || (Stats.Count == BestStats.Count && Stats.RankTotal == BestStats.RankTotal && Stats.FirstIndex < BestStats.FirstIndex)
            || (Stats.Count == BestStats.Count && Stats.RankTotal == BestStats.RankTotal && Stats.FirstIndex == BestStats.FirstIndex && SuitTieBreakScore(Pair.Key) > SuitTieBreakScore(BestSuit));
        if (bBetterSuit)
        {
            bFound = true;
            BestStats = Stats;
            BestSuit = Pair.Key;
        }
    }

    if (!bFound)
    {
        return false;
    }

    OutSuit = BestSuit;
    return true;
}

void ARoundCharacterGambler::BuildStartingDeck()
{
    DeckCards.Reset();
    HandCards.Reset();
    DiscardCards.Reset();
    NextCardInstanceId = 1;

    const TArray<ERoundCardSuit> Suits = {
        ERoundCardSuit::Hearts,
        ERoundCardSuit::Spades,
        ERoundCardSuit::Clubs,
        ERoundCardSuit::Diamonds
    };

    for (ERoundCardSuit Suit : Suits)
    {
        for (int32 RankValue = GetMinimumDeckRank(); RankValue <= 14; ++RankValue)
        {
            FRoundPlayingCard Card;
            Card.CardInstanceId = NextCardInstanceId++;
            Card.Suit = Suit;
            Card.RankValue = RankValue;
            DeckCards.Add(Card);
        }
    }
}

int32 ARoundCharacterGambler::GetMinimumDeckRank() const
{
    return FMath::Clamp(2 + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MinimumDeckRankAdd"))), 2, 10);
}

bool ARoundCharacterGambler::DrawSingleCard(FRoundPlayingCard& OutCard)
{
    EnsureDeckAvailable();
    if (DeckCards.Num() <= 0)
    {
        return false;
    }

    const int32 DrawIndex = FMath::RandRange(0, DeckCards.Num() - 1);
    OutCard = DeckCards[DrawIndex];
    DeckCards.RemoveAt(DrawIndex);
    return true;
}

void ARoundCharacterGambler::EnsureDeckAvailable()
{
    if (DeckCards.Num() > 0 || DiscardCards.Num() <= 0)
    {
        return;
    }

    DeckCards = DiscardCards;
    DiscardCards.Reset();
    const int32 HealAmount = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("HealOnDeckReshuffle"))));
    if (HealAmount > 0) Heal(HealAmount);
}

void ARoundCharacterGambler::DiscardCard(const FRoundPlayingCard& Card)
{
    DiscardCards.Add(Card);
}

int32 ARoundCharacterGambler::SumRankValues(const TArray<FRoundPlayingCard>& Cards) const
{
    int32 Total = 0;
    for (const FRoundPlayingCard& Card : Cards)
    {
        Total += Card.RankValue;
    }
    return Total;
}

void ARoundCharacterGambler::MarkTurnActionTaken()
{
    bTookTurnActionThisTurn = true;
}
