#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundRootsnareAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundRootsnareAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteResolvedAbility(class ARoundGridTile* TargetTile) override;
};
