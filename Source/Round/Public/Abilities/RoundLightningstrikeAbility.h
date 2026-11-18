#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundLightningstrikeAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundLightningstrikeAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual void GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const override;
    virtual FRoundActionResult ExecuteAbility(ARoundGridTile* TargetTile) override;
};
