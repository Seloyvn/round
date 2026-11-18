#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundPiercingShotAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundPiercingShotAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual void GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const override;
    virtual FRoundActionResult ExecuteAbility(ARoundGridTile* TargetTile) override;
};
