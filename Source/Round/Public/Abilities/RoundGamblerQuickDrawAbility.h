#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundAbilityActor.h"
#include "RoundGamblerQuickDrawAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundGamblerQuickDrawAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
