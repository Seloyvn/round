#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundShieldOathAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundShieldOathAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
