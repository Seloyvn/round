#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEffectActor.h"
#include "RoundObjectiveStatRewardEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundObjectiveStatRewardEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void ApplyToCharacter(class ARoundCharacter* TargetCharacter) override;
    virtual void RemoveEffect() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    int32 AttackAdd = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    int32 DefenseAdd = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    int32 MaxHealthAdd = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    int32 MaxInitiativeAdd = 0;

private:
    UPROPERTY(Transient)
    bool bApplied = false;
};
