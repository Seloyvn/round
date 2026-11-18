#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundExecuteAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundExecuteAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual bool CanTargetTile(const class ARoundGridTile* TargetTile) const override;
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
