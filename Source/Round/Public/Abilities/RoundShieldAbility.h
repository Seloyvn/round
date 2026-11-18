#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundShieldAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundShieldAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
