#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEffectActor.h"
#include "RoundBladeGuardEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundBladeGuardEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Effect")
    double DamageReductionPerStack = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Effect")
    TArray<int32> StackRemainingInitiatives;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    void AddStack(int32 DurationInitiative);

    virtual bool IsBuff() const override { return true; }
    virtual int32 ModifyIncomingDamage(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;
    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount) override;

protected:
    void RefreshDisplayedDuration();
};
