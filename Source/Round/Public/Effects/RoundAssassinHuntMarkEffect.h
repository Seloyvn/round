#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundAssassinHuntMarkEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundAssassinHuntMarkEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Effect")
    double DamageMultiplierFromCauser = 1.15;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual int32 ModifyIncomingDamageBeforeMitigation(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const override;
    virtual bool IsDebuff() const override { return true; }
};
