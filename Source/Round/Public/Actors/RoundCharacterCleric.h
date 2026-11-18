#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundCharacterCleric.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundCharacterCleric : public ARoundCharacter
{
    GENERATED_BODY()

public:
    ARoundCharacterCleric();

    UFUNCTION(BlueprintPure, Category = "Round|Cleric")
    int32 GetBlessingDurationInitiative() const;

    UFUNCTION(BlueprintPure, Category = "Round|Cleric")
    int32 GetBlessingMaxStacks() const;

    UFUNCTION(BlueprintPure, Category = "Round|Cleric")
    int32 GetBlessingHealAmount() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Cleric")
    class ARoundBlessingEffect* BlessAlly(class ARoundCharacter* Ally, int32 AddedStacks = 1);

protected:
    ARoundBlessingEffect* FindBlessingEffectOn(class ARoundCharacter* Ally) const;
};
