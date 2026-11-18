#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundDashAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundDashAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
