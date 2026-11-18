#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundDelayAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundDelayAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
