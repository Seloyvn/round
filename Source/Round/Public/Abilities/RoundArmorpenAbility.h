#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundArmorpenAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundArmorpenAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
