#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundArcaneShiftAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundArcaneShiftAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual bool CanTargetTile(const class ARoundGridTile* TargetTile) const override;
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
