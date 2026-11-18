#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEffectActor.h"
#include "RoundShieldOathEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundShieldOathEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Paladin") double RedirectedDamageMultiplier = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Paladin") double PaladinHealFraction = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Paladin") int32 ProtectedAllyInitiativePerHit = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    double RedirectPreMitigationFraction = 0.0;

    virtual bool IsBuff() const override { return true; }
    virtual int32 ModifyIncomingDamageBeforeMitigation(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const override;
};
