#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundShadowWoundEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundShadowWoundEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Effect")
    int32 BonusDamage = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Effect")
    int32 ComboGain = 1;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void OnDamageTaken(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 AppliedDamage, ERoundDamageType DamageType) override;
    virtual bool IsDebuff() const override { return true; }

private:
    bool bTriggered = false;
};
