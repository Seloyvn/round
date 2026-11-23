#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundCharacter3.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundCharacter3 : public ARoundCharacter
{
    GENERATED_BODY()

public:
    friend class FRoundCoreMatchAdapter;
    friend class ARoundGameState;
    ARoundCharacter3();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_ComboState, Category = "Round|Character3")
    int32 NativeComboPoints = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Character3")
    int32 NativeMovedFieldsThisTurn = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Character3")
    int32 NativeMarkedTargetCooldownTurnsRemaining = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Character3")
    int32 NativeColdPressureCooldownTurnsRemaining = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Character3")
    int32 NativeComboSpentThisTurn = 0;

    UFUNCTION(BlueprintPure, Category = "Round|Character3")
    virtual int32 GetMaxComboPoints() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character3")
    virtual int32 GetComboPointsStart() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character3")
    int32 TryGetComboPoints() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character3")
    int32 GetNativeTurnSequence() const { return NativeTurnSequence; }

    UFUNCTION(BlueprintCallable, Category = "Round|Character3")
    int32 TryConsumeComboPoints(int32 MaxAmount = 2147483647);

    UFUNCTION(BlueprintCallable, Category = "Round|Character3")
    void TryGainComboPoints(int32 Amount = 1);

    void GrantShadowstrikeFollowup(ARoundCharacter* TargetCharacter, int32 CostReduction);
    void GrantShadowstrikeFreeStep();
    void GrantShadowstrikeDamageReduction(double IncomingDamageMultiplier);
    void GrantHookShotBasicAttackCostReduction(int32 CostReduction);

    UFUNCTION(BlueprintPure, Category = "Round|Character3")
    bool IsFirstDamageTurnConsumed() const { return bNativeFirstDamageTurnConsumed; }

    UFUNCTION(BlueprintPure, Category = "Round|Character3")
    bool IsFirstDamageTurnActive() const { return bNativeFirstDamageTurnActive; }

    UFUNCTION(BlueprintPure, Category = "Round|Character3")
    bool IsStartInvisibilityApplied() const { return bNativeStartInvisibilityApplied; }

    UFUNCTION(BlueprintCallable, Category = "Round|Character3")
    void ApplySimAssassinState(
        int32 InComboPoints,
        int32 InMovedFieldsThisTurn,
        int32 InMarkedTargetCooldownTurnsRemaining,
        int32 InColdPressureCooldownTurnsRemaining,
        int32 InComboSpentThisTurn,
        bool bInFirstDamageTurnConsumed,
        bool bInFirstDamageTurnActive,
        bool bInStartInvisibilityApplied);

    void ApplyCoreAssassinState(
        int32 InComboPoints,
        int32 InMovedFieldsThisTurn,
        int32 InComboSpentThisTurn,
        int32 InHuntMarkCooldownInitiativeRemaining,
        int32 InTurnSequence,
        bool bInOpeningRewardUsedThisTurn,
        bool bInOverflowRewardUsedThisTurn,
        bool bInFreeAdjacentMoveAvailable,
        bool bInFreeAdjacentMoveUsedThisTurn,
        bool bInNextDamageAfterMoveReady,
        bool bInNextReductionAfterMoveReady,
        bool bInStartInvisibilityApplied);

    virtual void InitializeCharacter() override;
    virtual void BeginTurn() override;
    virtual void AdvanceGlobalInitiative(int32 Amount = 1) override;
    virtual void EndTurn() override;
    virtual FRoundActionResult AttackField(ARoundGridTile* TargetTile) override;
    virtual FRoundActionResult MoveAlongPath(const TArray<ARoundGridTile*>& Path) override;
    virtual int32 GetAttackCost() const override;
    virtual int32 ApplyRoundDamage(int32 Amount, ERoundDamageType DamageType, int32 DefenseIgnore, ARoundCharacter* SourceCharacter, bool& bDidDie) override;
    virtual int32 ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const override;

protected:
    virtual void HandleDamageDealt(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType) override;
    void EnsureStartInvisibility();
    bool HasInvisibilityEffect() const;

    UFUNCTION()
    void OnRep_ComboState();

    bool bNativeFirstDamageTurnConsumed = false;
    bool bNativeFirstDamageTurnActive = false;
    bool bNativeStartInvisibilityApplied = false;
    bool bOpeningCutUsedThisTurn = false;
    bool bFirstBasicAttackUsedThisTurn = false;
    bool bComboOverflowInitiativeUsedThisTurn = false;
    bool bFreeRetreatAvailable = false;
    bool bFreeRetreatUsedThisTurn = false;
    bool bDamageAfterMoveReady = false;
    bool bDamageReductionAfterMoveReady = false;
    int32 HuntMarkCooldownInitiativeRemaining = 0;
    TWeakObjectPtr<ARoundCharacter> ShadowstrikeFollowupTarget;
    int32 ShadowstrikeFollowupCostReduction = 0;
    int32 ShadowstrikeFollowupTurnsRemaining = 0;
    int32 HookShotBasicAttackCostReduction = 0;
    double ShadowstrikeIncomingDamageMultiplier = 0.0;
    mutable TWeakObjectPtr<ARoundCharacter> PendingBasicAttackTarget;
    int32 NativeTurnSequence = 0;
};
