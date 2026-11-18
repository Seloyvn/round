#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEffectActor.h"
#include "RoundDeferredDamageEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundDeferredDamageEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool IsBuff() const override { return true; }
    virtual int32 ModifyIncomingDamageBeforeMitigation(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const override;
    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Chronomancer")
    double DeferredDamageFraction = 0.5;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Chronomancer")
    int32 StoredDeferredDamage = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Chronomancer")
    double DeferredDamageForgivenFraction = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Chronomancer")
    int32 HealOnResolve = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Chronomancer")
    int32 InitiativeGainOnResolve = 0;
};
