#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundCharacterChronomancer.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundCharacterChronomancer : public ARoundCharacter
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginTurn() override;

    UFUNCTION(BlueprintPure, Category = "Round|Chronomancer")
    int32 GetFragments() const;

    UFUNCTION(BlueprintPure, Category = "Round|Chronomancer")
    int32 GetMaxFragments() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Chronomancer")
    int32 GainFragments(int32 Amount);

    UFUNCTION(BlueprintPure, Category = "Round|Chronomancer")
    bool HasEnoughFragments(int32 Cost) const;

    UFUNCTION(BlueprintCallable, Category = "Round|Chronomancer")
    bool SpendFragments(int32 Cost);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Chronomancer")
    int32 Fragments = 0;

    bool bUsedFragmentDiscountThisTurn = false;
    bool bUsedFirstSpendRefundThisTurn = false;
};
