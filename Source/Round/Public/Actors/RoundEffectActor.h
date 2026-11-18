#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoundTypes.h"
#include "RoundEffectActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundEffectActor : public AActor
{
    GENERATED_BODY()

public:
    ARoundEffectActor();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Effect")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Effect")
    FString Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Effect")
    FName EffectId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Effect")
    double Amount = 0.0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Effect")
    int32 RemainingTurns = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Effect")
    int32 RemainingInitiative = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Effect")
    TObjectPtr<class ARoundCharacter> OwningCharacter = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Effect")
    TObjectPtr<class ARoundCharacter> CauserCharacter = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Effect")
    bool bAutoExpireOnTurnEnd = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Effect")
    TMap<FString, int32> IntegerState;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Effect")
    TMap<FString, double> NumericState;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Effect")
    TMap<FString, FString> StringState;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual void ApplyToCharacter(class ARoundCharacter* TargetCharacter);

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual void RemoveEffect();

    UFUNCTION(BlueprintCallable, Category = "Effect")
    void SetDurationInitiative(int32 DurationInitiative, bool bExpireOnTurnEnd = false);

    UFUNCTION(BlueprintCallable, Category = "Effect")
    void SetDurationLegacyTurns(int32 DurationTurns, bool bExpireOnTurnEnd = true);

    UFUNCTION(BlueprintPure, Category = "Effect")
    int32 GetRemainingDurationInitiative() const;

    UFUNCTION(BlueprintNativeEvent, Category = "Effect")
    void OnApplied(class ARoundCharacter* TargetCharacter);

    UFUNCTION(BlueprintNativeEvent, Category = "Effect")
    bool SetHasVision(class ARoundCharacter* TargetCharacter, bool bCurrentVision) const;

    UFUNCTION(BlueprintNativeEvent, Category = "Effect")
    int32 GetMovecost(int32 In) const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual int32 ModifyIncomingDamage(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual int32 ModifyOutgoingDamage(class ARoundCharacter* SourceCharacter, class ARoundCharacter* TargetCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual int32 ModifyIncomingDamageBeforeMitigation(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual bool PreventsAbilityUse(const class ARoundAbilityActor* Ability) const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual bool PreventsEffectApplication(const ARoundEffectActor* IncomingEffect) const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual bool IsBuff() const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual bool IsDebuff() const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual bool IsRemovable() const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual bool PreventsTurnActions() const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual bool PreventsMovement() const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual int32 GetTurnBlockRemainingInitiative() const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual int32 ModifyInitiativeGain(int32 InitiativeAmount) const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual int32 ModifyBasicAttackCost(class ARoundCharacter* AttackingCharacter, int32 AttackCost) const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual int32 ModifyBasicAttackDamage(class ARoundCharacter* AttackingCharacter, class ARoundCharacter* TargetCharacter, int32 Damage) const;

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual void OnBasicAttackExecuted(class ARoundCharacter* AttackingCharacter, class ARoundCharacter* TargetCharacter);

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual void OnTurnStarted(class ARoundCharacter* TargetCharacter);

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual void OnTurnEnded(class ARoundCharacter* TargetCharacter);

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount);

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual void OnOwnerMoved(class ARoundCharacter* TargetCharacter, class ARoundGridTile* PreviousTile, class ARoundGridTile* NewTile);

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual void OnOtherCharacterMoved(class ARoundCharacter* TargetCharacter, class ARoundCharacter* MovedCharacter, class ARoundGridTile* PreviousTile, class ARoundGridTile* NewTile);

    UFUNCTION(BlueprintCallable, Category = "Effect")
    virtual void OnDamageTaken(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 AppliedDamage, ERoundDamageType DamageType);
};
