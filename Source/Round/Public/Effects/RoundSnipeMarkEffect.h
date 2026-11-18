#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundSnipeMarkEffect.generated.h"

UCLASS()
class ROUND_API ARoundSnipeMarkEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual bool IsDebuff() const override { return true; }
    virtual int32 ModifyIncomingDamage(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;
};
