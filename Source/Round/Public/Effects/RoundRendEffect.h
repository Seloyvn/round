#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundRendEffect.generated.h"

UCLASS()
class ROUND_API ARoundRendEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    int32 BloodPerTick = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    int32 MovementTriggerDamage = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Effect")
    bool bMovementTriggered = false;

    virtual bool IsDebuff() const override { return true; }
    virtual void OnTurnStarted(class ARoundCharacter* TargetCharacter) override;
    virtual void OnOwnerMoved(class ARoundCharacter* TargetCharacter, class ARoundGridTile* PreviousTile, class ARoundGridTile* NewTile) override;
};
