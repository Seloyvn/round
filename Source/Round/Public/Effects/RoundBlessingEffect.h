#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEffectActor.h"
#include "RoundBlessingEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundBlessingEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void OnDamageTaken(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 AppliedDamage, ERoundDamageType DamageType);
    virtual bool IsBuff() const override { return true; }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Blessing")
    int32 Stacks = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blessing")
    int32 MaxStacks = 3;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blessing")
    int32 HealAmount = 100;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blessing")
    int32 StackHealThreshold = 3;

    UFUNCTION(BlueprintCallable, Category = "Blessing")
    void AddBlessingStack(class ARoundCharacter* TargetCharacter, int32 AddedStacks = 1, bool bRefreshDuration = true);

protected:
    void TryResolveBlessingHeal(class ARoundCharacter* TargetCharacter);
};
