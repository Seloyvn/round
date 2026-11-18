#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundRendAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundRendAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
