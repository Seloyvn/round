#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundInvisibilityEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundInvisibilityEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual bool IsBuff() const override { return true; }
    virtual bool SetHasVision_Implementation(class ARoundCharacter* TargetCharacter, bool bCurrentVision) const override;
};
