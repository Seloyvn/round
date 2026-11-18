#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundSilenceEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundSilenceEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual bool IsDebuff() const override { return true; }
    virtual bool PreventsAbilityUse(const class ARoundAbilityActor* Ability) const override { return true; }
};
