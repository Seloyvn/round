#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundCharacter5.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundCharacter5 : public ARoundCharacter
{
    GENERATED_BODY()

public:
    ARoundCharacter5();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginTurn() override;
    virtual void AdvanceGlobalInitiative(int32 Amount = 1) override;
    virtual int32 ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const override;
    virtual double GetIncomingDamageMultiplier(ERoundDamageType DamageType) const override;
    virtual int32 GetIncomingBonusDefense(ERoundDamageType DamageType) const override;
    virtual void HandlePassiveDamageTaken(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType) override;
    virtual void HandleEnemyHitByCharacter(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType) override;
    virtual bool TryPreventDeathFromPassive(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Round|Character5")
    int32 NativeBlood = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Character5")
    bool bFirstDamageReductionAvailableThisTurn = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Character5")
    int32 BloodCheatDeathCooldownTurnsRemaining = 0;

    UFUNCTION(BlueprintPure, Category = "Round|Character5")
    virtual int32 GetCurrentBlood() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character5")
    virtual int32 GetBloodCap() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Character5")
    virtual void GainBlood(int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category = "Round|Character5")
    virtual void GainRawBlood(int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category = "Round|Character5")
    int32 TryConsumeBlood(int32 MaximumAmount = 0);

    UFUNCTION(BlueprintPure, Category = "Round|Character5")
    double GetBloodGainFractionValue() const { return GetBloodGainFraction(); }

protected:
    double GetBloodGainFraction() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Character5")
    bool bBasicAttackBloodAvailableThisTurn = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Character5")
    bool bBloodCapTriggerAvailable = true;
};
