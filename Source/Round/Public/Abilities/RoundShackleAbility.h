#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundShackleAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundShackleAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
