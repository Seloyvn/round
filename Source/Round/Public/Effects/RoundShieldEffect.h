#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEffectActor.h"
#include "RoundShieldEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundShieldEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield")
    int32 ShieldAmountRemaining = 0;

    virtual bool IsBuff() const override { return true; }
    virtual int32 ModifyIncomingDamageBeforeMitigation(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const override;
};
