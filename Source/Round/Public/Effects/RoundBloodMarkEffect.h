#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundBloodMarkEffect.generated.h"

UCLASS()
class ROUND_API ARoundBloodMarkEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    double DamageMultiplier = 1.0;

    virtual bool IsDebuff() const override { return true; }
    virtual int32 ModifyIncomingDamage(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;
};
