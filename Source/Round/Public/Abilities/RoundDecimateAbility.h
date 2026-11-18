#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundDecimateAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundDecimateAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
