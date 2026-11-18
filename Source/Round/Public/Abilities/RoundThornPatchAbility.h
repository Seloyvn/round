#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundThornPatchAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundThornPatchAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteResolvedAbility(class ARoundGridTile* TargetTile) override;
};
