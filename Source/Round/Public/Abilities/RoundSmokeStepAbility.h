#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundSmokeStepAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundSmokeStepAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
