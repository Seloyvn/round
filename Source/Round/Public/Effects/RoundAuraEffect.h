#pragma once

#include "CoreMinimal.h"
#include "RoundTypes.h"
#include "Actors/RoundEffectActor.h"
#include "RoundAuraEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundAuraEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Aura")
    int32 Radius = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Aura")
    bool bAffectEnemies = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Aura")
    bool bAffectAllies = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Aura")
    bool bAffectSelf = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Aura")
    FText AuraModifierDisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Aura")
    FText AuraModifierDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Aura")
    bool bAuraUsesFixedPosition = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Aura")
    FVector2D AuraFixedPosition = FVector2D::ZeroVector;

    virtual void OnApplied_Implementation(class ARoundCharacter* TargetCharacter) override;
    virtual void RemoveEffect() override;
    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount) override;
    virtual bool IsRemovable() const override { return false; }

    UFUNCTION(BlueprintPure, Category = "Round|Aura")
    virtual bool AffectsCharacter(const class ARoundCharacter* Character) const;

    UFUNCTION(BlueprintCallable, Category = "Round|Aura")
    virtual int32 ModifyIncomingDamageForCharacter(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore);

    UFUNCTION(BlueprintPure, Category = "Round|Aura")
    virtual bool TryGetIncomingDamageReductionFraction(double& OutFraction) const;

protected:
    UPROPERTY(Transient)
    TMap<TWeakObjectPtr<class ARoundCharacter>, TObjectPtr<class ARoundModifierEffect>> ActiveAuraModifiers;
};
