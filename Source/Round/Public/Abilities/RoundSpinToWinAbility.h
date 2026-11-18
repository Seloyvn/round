#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundSpinToWinAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundSpinToWinAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual void GetPreviewTiles(const class ARoundGridTile* TargetTile, TArray<class ARoundGridTile*>& OutTiles) const override;
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
