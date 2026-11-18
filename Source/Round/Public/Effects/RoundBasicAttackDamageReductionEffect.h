#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundBasicAttackDamageReductionEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundBasicAttackDamageReductionEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Effect")
    double DamageMultiplier = 0.7;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual int32 ModifyBasicAttackDamage(ARoundCharacter* AttackingCharacter, ARoundCharacter* TargetCharacter, int32 Damage) const override;
    virtual bool IsDebuff() const override { return true; }
};
