#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundIcebeamAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundIcebeamAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;

private:
    void ApplyIcebeamSlow(class ARoundCharacter* TargetCharacter, double SlowAmount, int32 DurationInitiative, bool bPrimaryTarget);
};
