#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundAbilityActor.h"
#include "RoundGamblerLowStakesAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundGamblerLowStakesAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
