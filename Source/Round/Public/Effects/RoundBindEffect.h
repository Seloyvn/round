#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundBindEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundBindEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 TriggerDamage = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 DelayAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    double TriggerOutgoingDamageMultiplier = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    double TriggerSlowAmount = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 TriggerSlowDuration = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 TriggerAoEInitiativeLossRadius = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 TriggerAoEInitiativeLossAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 TriggerInitiativeGainForCauser = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    bool bBlockMovementBonusesOnTarget = false;

    virtual bool IsDebuff() const override { return true; }
    virtual int32 GetMovecost_Implementation(int32 In) const override;
    virtual void OnOwnerMoved(class ARoundCharacter* TargetCharacter, class ARoundGridTile* PreviousTile, class ARoundGridTile* NewTile) override;
};
