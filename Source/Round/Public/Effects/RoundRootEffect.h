#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundRootEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundRootEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool IsDebuff() const override { return true; }
    virtual bool PreventsMovement() const override { return true; }
    virtual int32 GetMovecost_Implementation(int32 In) const override;
    virtual int32 ModifyIncomingDamage(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;
    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Druid") double IncomingDamageMultiplierFromCauser = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Druid") int32 InitiativeLossOnExpire = 0;
};
