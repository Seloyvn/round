#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundConcentrateAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundConcentrateAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
