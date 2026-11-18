#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundOneHitProtectionEffect.generated.h"

UCLASS()
class ROUND_API ARoundOneHitProtectionEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY()
    double IncomingDamageMultiplier = 1.0;

    virtual bool IsBuff() const override { return true; }
    virtual int32 ModifyIncomingDamage(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;
};
