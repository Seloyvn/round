#pragma once
#include "Actors/RoundEffectActor.h"
#include "RoundAimTrackingEffect.generated.h"

UCLASS()
class ROUND_API ARoundAimTrackingEffect : public ARoundEffectActor
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 TrackingDamage = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) ERoundDamageType DamageType = ERoundDamageType::Physical;
    virtual bool IsDebuff() const override { return true; }
    virtual void OnOwnerMoved(class ARoundCharacter* Target, class ARoundGridTile* Previous, class ARoundGridTile* Next) override;
};
