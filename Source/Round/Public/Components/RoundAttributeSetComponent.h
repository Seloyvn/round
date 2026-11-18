#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RoundTypes.h"
#include "RoundAttributeSetComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRoundHealthChanged, int32, NewHealth, int32, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundInitiativeChanged, int32, NewInitiative);

UCLASS(ClassGroup=(Round), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class ROUND_API URoundAttributeSetComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URoundAttributeSetComponent();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_BaseStats, Category = "Round")
    FRoundStatBlock BaseStats;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentHealth, Category = "Round")
    int32 CurrentHealth = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentInitiative, Category = "Round")
    int32 CurrentInitiative = 0;

    UPROPERTY(BlueprintAssignable, Category = "Round")
    FOnRoundHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "Round")
    FOnRoundInitiativeChanged OnInitiativeChanged;

    UFUNCTION(BlueprintCallable, Category = "Round")
    void InitializeFromStats(const FRoundStatBlock& InStats);

    UFUNCTION(BlueprintCallable, Category = "Round")
    int32 ApplyDamage(int32 RawDamage);

    UFUNCTION(BlueprintCallable, Category = "Round")
    int32 ApplyFinalDamage(int32 FinalDamage);

    UFUNCTION(BlueprintCallable, Category = "Round")
    void Heal(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Round")
    bool SpendInitiative(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Round")
    void GainInitiative(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Round")
    void PassInitiative(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Round")
    void SetCurrentHealthValue(int32 NewHealth);

    UFUNCTION(BlueprintCallable, Category = "Round")
    void SetCurrentInitiativeValue(int32 NewInitiative);

    UFUNCTION(BlueprintPure, Category = "Round")
    bool IsAlive() const;

protected:
    UFUNCTION()
    void OnRep_BaseStats();

    UFUNCTION()
    void OnRep_CurrentHealth();

    UFUNCTION()
    void OnRep_CurrentInitiative();
};
