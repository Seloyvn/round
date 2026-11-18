#pragma once
#include "Actors/RoundEffectActor.h"
#include "RoundNextOutgoingDamageEffect.generated.h"

UCLASS()
class ROUND_API ARoundNextOutgoingDamageEffect : public ARoundEffectActor
{
    GENERATED_BODY()
public:
    virtual bool IsDebuff() const override { return true; }
    virtual int32 ModifyOutgoingDamage(class ARoundCharacter*, class ARoundCharacter*, int32 Damage, ERoundDamageType, int32&) const override;
};
