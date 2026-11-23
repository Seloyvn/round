#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundCharacter4.generated.h"

UENUM(BlueprintType)
enum class ERoundMageElementBonus : uint8
{
    None,
    FirePower,
    IceControl,
    LightningTempo,
    RockGuard
};

UCLASS(Blueprintable)
class ROUND_API ARoundCharacter4 : public ARoundCharacter
{
    GENERATED_BODY()

public:
    friend class FRoundCoreMatchAdapter;
    friend class ARoundGameState;
    ARoundCharacter4();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ElementBonusState, Category = "Round|Character4")
    TArray<ERoundMageElementBonus> ActiveElementBonuses;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ElementBonusState, Category = "Round|Character4")
    TArray<ERoundMageElementBonus> UsedElementBonusesThisTurn;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ElementBonusState, Category = "Round|Character4")
    bool bMovedThisTurn = false;

    UFUNCTION(BlueprintCallable, Category = "Round|Character4")
    virtual void RegisterAbilityDamageType(ERoundDamageType DamageType);

    UFUNCTION(BlueprintPure, Category = "Round|Character4")
    virtual ERoundMageElementBonus GetActiveElementBonus() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Character4")
    virtual void ClearElementBonus();

    UFUNCTION(BlueprintCallable, Category = "Round|Character4")
    virtual void ClearSpecificElementBonus(ERoundMageElementBonus Bonus);

    UFUNCTION(BlueprintPure, Category = "Round|Character4")
    virtual bool IsElementBonusActive(ERoundMageElementBonus Bonus) const;

    UFUNCTION(BlueprintPure, Category = "Round|Character4")
    virtual bool WasElementBonusUsedThisTurn(ERoundMageElementBonus Bonus) const;

    UFUNCTION(BlueprintPure, Category = "Round|Character4")
    int32 GetActiveElementBonusCount() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character4")
    ERoundMageElementBonus GetLastCastElementBonus() const { return LastCastElementBonus; }

    UFUNCTION(BlueprintPure, Category = "Round|Character4")
    ERoundMageElementBonus GetFirstCastElementBonusThisTurn() const { return FirstCastElementBonusThisTurn; }

    UFUNCTION(BlueprintPure, Category = "Round|Character4")
    int32 GetElementSwapCooldownTurnsRemaining() const { return ElementSwapCooldownTurnsRemaining; }

    UFUNCTION(BlueprintPure, Category = "Round|Character4")
    double GetCurrentAbilityDamageMultiplier() const { return CurrentAbilityDamageMultiplier; }

    UFUNCTION(BlueprintPure, Category = "Round|Character4")
    double GetStoredElementDamageMultiplier() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character4")
    int32 GetStoredElementRangeAdd() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character4")
    double GetStoredElementCooldownMultiplier() const;

    int32 GetStoredElementAbilityCostReduction() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Character4")
    void ApplySimMageState(
        const TArray<ERoundMageElementBonus>& InActiveElementBonuses,
        const TArray<ERoundMageElementBonus>& InUsedElementBonusesThisTurn,
        bool bInMovedThisTurn,
        ERoundMageElementBonus InLastCastElementBonus,
        ERoundMageElementBonus InFirstCastElementBonusThisTurn,
        int32 InElementSwapCooldownTurnsRemaining,
        double InCurrentAbilityDamageMultiplier);

    void ApplyCoreMageState(int32 InActiveElement, uint32 InUsedElementMask, int32 InLastCastElement,
        int32 InFirstCastElement, int32 InElementSwapCooldownRemaining, int32 InRockDefenseBonus,
        int32 InRockDefenseRemainingInitiative, int32 InProtectionRemainingInitiative,
        int32 InEchoElement, uint32 InElementCycleMask, bool bInElementCostReductionUsed,
        bool bInElementSwapRefundUsed, bool bInStoredElementEmpowered, bool bInProtectionReady);

    virtual int32 AdjustEffectDuration(int32 Duration);

    virtual void InitializeCharacter() override;
    virtual void BeginTurn() override;
    virtual void AdvanceGlobalInitiative(int32 Amount = 1) override;
    virtual void EndTurn() override;
    virtual FRoundActionResult MoveAlongPath(const TArray<ARoundGridTile*>& Path) override;
    virtual int32 GetAttackCost() const override;
    virtual FRoundActionResult UseAbilityIndex(int32 AbilityIndex, ARoundGridTile* TargetTile) override;
    virtual int32 ApplyRoundDamage(int32 Amount, ERoundDamageType DamageType, int32 DefenseIgnore, ARoundCharacter* SourceCharacter, bool& bDidDie) override;
    virtual int32 ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const override;

protected:
    ERoundMageElementBonus DamageTypeToElementBonus(ERoundDamageType DamageType) const;
    void ConsumeElementBonus(ERoundMageElementBonus Bonus);
    void MaybeGrantElementSwapInitiative(ERoundMageElementBonus NewBonus);
    virtual int32 GetIncomingBonusDefense(ERoundDamageType DamageType) const override;
    virtual void HandleDamageDealt(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType) override;

    UFUNCTION()
    void OnRep_ElementBonusState();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ElementBonusState, Category = "Round|Character4")
    ERoundMageElementBonus LastCastElementBonus = ERoundMageElementBonus::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ElementBonusState, Category = "Round|Character4")
    ERoundMageElementBonus FirstCastElementBonusThisTurn = ERoundMageElementBonus::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ElementBonusState, Category = "Round|Character4")
    int32 ElementSwapCooldownTurnsRemaining = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ElementBonusState, Category = "Round|Character4")
    double CurrentAbilityDamageMultiplier = 1.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ElementBonusState, Category = "Round|Character4")
    int32 ElementalRockDefenseBonus = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ElementBonusState, Category = "Round|Character4")
    int32 ElementalRockDefenseRemainingInitiative = 0;

    bool bResolvingElementalAbility = false;
    bool bCountRockElementHits = false;
    int32 RockElementEnemyHits = 0;
    bool bElementCostReductionUsedThisTurn = false;
    bool bElementSwapRefundUsedThisTurn = false;
    bool bStoredElementEmpowered = false;
    bool bProtectionReady = false;
    int32 ProtectionRemainingInitiative = 0;
    ERoundMageElementBonus EchoElement = ERoundMageElementBonus::None;
    TArray<ERoundMageElementBonus> ElementCycle;
    int32 CurrentAbilityEnemyHits = 0;
    ERoundMageElementBonus CurrentCastElement = ERoundMageElementBonus::None;
    bool bAllElementsActiveForCast = false;

    double GetElementStrength(ERoundMageElementBonus Element) const;
    bool HasElementForCurrentCast(ERoundMageElementBonus Element) const;
};
