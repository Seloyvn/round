#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundTauntEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundTauntEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 InitiativeLossWhenIgnoringCauser = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 InitiativeGainForCauserWhenAttacked = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    double IncomingDamageMultiplier = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    double IncomingDamageMultiplierFromCauser = 1.0;

    virtual bool IsDebuff() const override { return true; }
    virtual int32 ModifyOutgoingDamage(class ARoundCharacter* SourceCharacter, class ARoundCharacter* TargetCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;
    virtual int32 ModifyIncomingDamage(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;
};
