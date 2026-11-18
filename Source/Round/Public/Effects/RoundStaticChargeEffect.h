#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundStaticChargeEffect.generated.h"

UCLASS()
class ROUND_API ARoundStaticChargeEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual bool IsDebuff() const override { return true; }
    virtual void OnOwnerMoved(class ARoundCharacter* TargetCharacter, class ARoundGridTile* PreviousTile, class ARoundGridTile* NewTile) override;
};
