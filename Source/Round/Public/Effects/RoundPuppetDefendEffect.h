#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundPuppetDefendEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundPuppetDefendEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    double NextPuppetHitDamageMultiplier = 1.0;
};
