#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundSummonBladeAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundSummonBladeAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual bool CanTargetTile(const class ARoundGridTile* TargetTile) const override;
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
