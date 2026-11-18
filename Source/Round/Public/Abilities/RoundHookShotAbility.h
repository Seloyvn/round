#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundHookShotAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundHookShotAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual ERoundButtonState GetAbilityVisibility() const override;
    virtual bool CanTargetTile(const class ARoundGridTile* TargetTile) const override;
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;

    void OnOwnerTurnStarted();
    void OnOwnerTurnEnded();
    void NotifyOwnerKilledEnemy();

private:
    TWeakObjectPtr<class ARoundGridTile> AnchorTile;
    bool bKillResetArmed = false;
    bool bKillResetUsedThisTurn = false;
};
