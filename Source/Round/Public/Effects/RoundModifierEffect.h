#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundModifierEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundModifierEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    double IncomingDamageMultiplier = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    double OutgoingDamageMultiplier = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 MoveCostAdd = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    bool bBlockMovementBonuses = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    bool bOnlyAffectCauserDamage = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 IncomingEffectDurationAdd = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 DefenseIgnoreAdd = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    bool bOnlyAgainstRangedSources = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    double HealFractionFromDamageDealt = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    double RedirectPreMitigationFraction = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    bool bTreatAsBuff = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    bool bTreatAsDebuff = false;

    virtual bool IsBuff() const override { return bTreatAsBuff; }
    virtual bool IsDebuff() const override { return bTreatAsDebuff; }
    virtual int32 GetMovecost_Implementation(int32 In) const override;
    virtual int32 ModifyIncomingDamage(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;
    virtual int32 ModifyOutgoingDamage(class ARoundCharacter* SourceCharacter, class ARoundCharacter* TargetCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;
};
