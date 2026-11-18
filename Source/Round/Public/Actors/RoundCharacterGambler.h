#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundCharacterGambler.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundCharacterGambler : public ARoundCharacter
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void InitializeCharacter() override;
    virtual void BeginTurn() override;
    virtual void EndTurn() override;
    virtual FRoundActionResult AttackField(class ARoundGridTile* TargetTile) override;
    virtual FRoundActionResult MoveAlongPath(const TArray<class ARoundGridTile*>& Path) override;
    virtual FRoundActionResult UseAbilityIndex(int32 AbilityIndex, class ARoundGridTile* TargetTile) override;

    UFUNCTION(BlueprintPure, Category = "Round|Gambler")
    int32 GetMaxHandCards() const;

    UFUNCTION(BlueprintPure, Category = "Round|Gambler")
    int32 GetStartingHandCards() const;

    UFUNCTION(BlueprintPure, Category = "Round|Gambler")
    int32 GetTurnStartDrawCount() const;

    UFUNCTION(BlueprintPure, Category = "Round|Gambler")
    int32 GetIdleTurnDrawCount() const;

    UFUNCTION(BlueprintPure, Category = "Round|Gambler")
    int32 GetDeckCount() const;

    UFUNCTION(BlueprintPure, Category = "Round|Gambler")
    int32 GetHandCount() const;

    UFUNCTION(BlueprintPure, Category = "Round|Gambler")
    int32 GetDiscardCount() const;

    const TArray<FRoundPlayingCard>& GetHandCards() const;

    UFUNCTION(BlueprintPure, Category = "Round|Gambler")
    double GetRankMultiplier(int32 RankValue) const;

    UFUNCTION(BlueprintPure, Category = "Round|Gambler")
    bool HasHandCardIndex(int32 CardIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Round|Gambler")
    int32 DrawCards(int32 Count);

    UFUNCTION(BlueprintCallable, Category = "Round|Gambler")
    bool ConsumeOldestHandCard(FRoundPlayingCard& OutCard);

    UFUNCTION(BlueprintCallable, Category = "Round|Gambler")
    int32 DiscardHandCardsBelowRank(int32 MinimumKeptRank);

    UFUNCTION(BlueprintCallable, Category = "Round|Gambler")
    int32 DiscardSelectedHandCards(const TArray<int32>& CardIndices);

    UFUNCTION(BlueprintCallable, Category = "Round|Gambler")
    bool ConsumeMostCommonSuitCards(TArray<FRoundPlayingCard>& OutConsumedCards, ERoundCardSuit& OutSuit);

    UFUNCTION(BlueprintCallable, Category = "Round|Gambler")
    bool TransformHandCardFromDeckProfile(int32 CardIndex, FRoundPlayingCard& OutTransformedCard);

    UFUNCTION(BlueprintCallable, Category = "Round|Gambler")
    bool ImproveHandCard(int32 CardIndex, int32 RankAdd, int32 MinimumRank);

    UFUNCTION(BlueprintPure, Category = "Round|Gambler")
    bool GetMostCommonSuit(ERoundCardSuit& OutSuit) const;

protected:
    void BuildStartingDeck();
    bool DrawSingleCard(FRoundPlayingCard& OutCard);
    void EnsureDeckAvailable();
    void DiscardCard(const FRoundPlayingCard& Card);
    int32 SumRankValues(const TArray<FRoundPlayingCard>& Cards) const;
    int32 GetMinimumDeckRank() const;
    void MarkTurnActionTaken();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Gambler")
    TArray<FRoundPlayingCard> DeckCards;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Gambler")
    TArray<FRoundPlayingCard> HandCards;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Gambler")
    TArray<FRoundPlayingCard> DiscardCards;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Gambler")
    int32 NextCardInstanceId = 1;

    UPROPERTY(Transient)
    bool bTookTurnActionThisTurn = false;
};
