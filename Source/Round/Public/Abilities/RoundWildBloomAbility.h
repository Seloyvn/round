#pragma once

#include "Actors/RoundAbilityActor.h"
#include "RoundWildBloomAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundWildBloomAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual bool CanTargetTile(const class ARoundGridTile* TargetTile) const override;
    virtual void GetPreviewTiles(const class ARoundGridTile* TargetTile, TArray<class ARoundGridTile*>& OutTiles) const override;
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
};
