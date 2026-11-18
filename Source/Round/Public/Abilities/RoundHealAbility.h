#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundHealAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundHealAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
