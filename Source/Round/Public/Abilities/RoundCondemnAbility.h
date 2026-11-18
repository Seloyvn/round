#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundCondemnAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundCondemnAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual bool CanTargetTile(const class ARoundGridTile* TargetTile) const override;
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
