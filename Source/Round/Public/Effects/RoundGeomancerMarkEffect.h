#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEffectActor.h"
#include "RoundGeomancerMarkEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundGeomancerMarkEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual bool IsDebuff() const override;
    virtual bool IsRemovable() const override;
};
