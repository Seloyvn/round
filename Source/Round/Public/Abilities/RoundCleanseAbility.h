#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundCleanseAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundCleanseAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
