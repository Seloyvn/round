#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundAbilityActor.h"
#include "RoundAccelerateAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundAccelerateAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteResolvedAbility(class ARoundGridTile* TargetTile) override;
};
