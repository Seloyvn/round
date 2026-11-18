#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundSanctifyGroundAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundSanctifyGroundAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
