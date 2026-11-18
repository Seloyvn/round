#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundRockblastAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundRockblastAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
