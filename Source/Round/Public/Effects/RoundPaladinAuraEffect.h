#pragma once

#include "CoreMinimal.h"
#include "Effects/RoundAuraEffect.h"
#include "RoundPaladinAuraEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundPaladinAuraEffect : public ARoundAuraEffect
{
    GENERATED_BODY()

public:
    virtual int32 ModifyIncomingDamageForCharacter(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) override;
    virtual bool TryGetIncomingDamageReductionFraction(double& OutFraction) const override;

    UFUNCTION(BlueprintPure, Category = "Round|Aura")
    bool HasRecentPreventedDamageFrom(const class ARoundCharacter* SourceCharacter, int32 WindowInitiative) const;

protected:
    UPROPERTY(Transient)
    TMap<TWeakObjectPtr<class ARoundCharacter>, int32> RecentPreventedDamageAtBySource;
};
