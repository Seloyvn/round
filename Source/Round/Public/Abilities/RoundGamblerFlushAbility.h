#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundAbilityActor.h"
#include "RoundGamblerFlushAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundGamblerFlushAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual bool CanTargetTile(const class ARoundGridTile* TargetTile) const override;
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
