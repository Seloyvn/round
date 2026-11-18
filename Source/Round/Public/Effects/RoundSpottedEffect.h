#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEffectActor.h"
#include "RoundSpottedEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundSpottedEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Spotter")
    double DamageTakenMultiplier = 1.25;

    virtual bool IsDebuff() const override { return true; }
    virtual bool SetHasVision_Implementation(class ARoundCharacter* TargetCharacter, bool bCurrentVision) const override;
    virtual int32 ModifyIncomingDamage(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;
};
