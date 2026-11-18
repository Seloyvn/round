#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundAbilityActor.h"
#include "RoundDragoonOvershootAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundDragoonOvershootAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual bool CanTargetTile(const class ARoundGridTile* TargetTile) const override;
    virtual void GetPreviewTiles(const class ARoundGridTile* TargetTile, TArray<class ARoundGridTile*>& OutTiles) const override;

protected:
    virtual FRoundActionResult ExecuteResolvedAbility(class ARoundGridTile* TargetTile) override;
};
