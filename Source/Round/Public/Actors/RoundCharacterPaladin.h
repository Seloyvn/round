#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundCharacterPaladin.generated.h"

UCLASS()
class ROUND_API ARoundCharacterPaladin : public ARoundCharacter
{
    GENERATED_BODY()

public:
    ARoundCharacterPaladin();

    virtual void InitializeCharacter() override;


    UFUNCTION(BlueprintPure, Category = "Round")
    double GetAuraDamageReduction() const;

    UFUNCTION(BlueprintPure, Category = "Round")
    int32 GetAuraRadius() const;

protected:
    void EnsurePassiveAura();

    UPROPERTY(Transient)
    TObjectPtr<class ARoundPaladinAuraEffect> PassiveAuraEffect = nullptr;
};
