#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundTauntAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundTauntAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
