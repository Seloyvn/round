#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundStringSnareAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundStringSnareAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
