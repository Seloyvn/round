#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundSuppressingFireAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundSuppressingFireAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
