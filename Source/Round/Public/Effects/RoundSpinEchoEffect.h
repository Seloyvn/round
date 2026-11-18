#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundSpinEchoEffect.generated.h"

UCLASS()
class ROUND_API ARoundSpinEchoEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    int32 Damage = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    int32 Radius = 1;

    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount) override;
};
