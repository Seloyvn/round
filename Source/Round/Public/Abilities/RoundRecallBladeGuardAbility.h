#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundAbilityActor.h"
#include "RoundRecallBladeGuardAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundRecallBladeGuardAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual bool CanTargetTile(const class ARoundGridTile* TargetTile) const override;
    virtual void GetPreviewTiles(const class ARoundGridTile* TargetTile, TArray<class ARoundGridTile*>& OutTiles) const override;
    virtual void OnGrantedToCharacter() override;

protected:
    virtual FRoundActionResult ExecuteResolvedAbility(class ARoundGridTile* TargetTile) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void HandleBladePickedUp(class ARoundGridTile* PickedUpFromTile);

    bool bBladePickupSubscribed = false;
};
