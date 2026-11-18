#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundBulwarkAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundBulwarkAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
