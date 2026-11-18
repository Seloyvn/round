#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundAimAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundAimAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual void GetPreviewTiles(const class ARoundGridTile* TargetTile, TArray<class ARoundGridTile*>& OutTiles) const override;
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
