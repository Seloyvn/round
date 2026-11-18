#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundBearFormAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundBearFormAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
