#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundAbilityActor.h"
#include "RoundGamblerDoubleDrawAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundGamblerDoubleDrawAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
