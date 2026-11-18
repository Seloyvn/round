#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundBleedEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundBleedEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual bool IsDebuff() const override { return true; }
    virtual void OnTurnStarted(class ARoundCharacter* TargetCharacter) override;
};
