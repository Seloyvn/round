#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundRescueAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundRescueAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
