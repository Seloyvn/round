#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundAbilityActor.h"
#include "RoundDragoonRepelAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundDragoonRepelAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual bool CanTargetTile(const class ARoundGridTile* TargetTile) const override;

protected:
    virtual FRoundActionResult ExecuteResolvedAbility(class ARoundGridTile* TargetTile) override;
};
