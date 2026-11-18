#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundGroveAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundGroveAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteResolvedAbility(class ARoundGridTile* TargetTile) override;
};
