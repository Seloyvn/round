#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundFireballAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundFireballAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual void GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const override;
    virtual FRoundActionResult ExecuteAbility(ARoundGridTile* TargetTile) override;
    void ResolveAfterburn(ARoundGridTile* TargetTile, double DamageMultiplier);

protected:
    virtual FRoundActionResult ExecuteResolvedAbility(ARoundGridTile* TargetTile) override;

private:
    int32 ResolveExplosion(ARoundGridTile* TargetTile, double DamageMultiplier, bool bApplySecondaryEffects);
};
