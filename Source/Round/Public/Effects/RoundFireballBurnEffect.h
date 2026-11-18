#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundFireballBurnEffect.generated.h"

UCLASS()
class ROUND_API ARoundFireballBurnEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual bool IsDebuff() const override { return true; }
    virtual void OnTurnStarted(class ARoundCharacter* TargetCharacter) override;
};
