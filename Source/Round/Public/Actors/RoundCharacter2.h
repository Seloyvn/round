#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundCharacter2.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundCharacter2 : public ARoundCharacter
{
    GENERATED_BODY()

public:
    ARoundCharacter2();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_ResourceState, Category = "Round|Character2")
    int32 Ammo = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_ResourceState, Category = "Round|Character2")
    bool bAttackedThisTurn = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ResourceState, Category = "Round|Character2")
    bool bSpentAmmoThisTurn = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ResourceState, Category = "Round|Character2")
    bool bMovedThisTurn = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ResourceState, Category = "Round|Character2")
    bool bStartedTurnWithFullAmmo = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ResourceState, Category = "Round|Character2")
    bool bHeldPositionFromPreviousTurn = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ResourceState, Category = "Round|Character2")
    bool bUsedFreeBasicAttackThisTurn = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ResourceState, Category = "Round|Character2")
    bool bTriggeredEmptyMagazineThisTurn = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ResourceState, Category = "Round|Character2")
    bool bRefundedLastAmmoKillThisTurn = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ResourceState, Category = "Round|Character2")
    bool bUsedSnipeRangeRefundThisTurn = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ResourceState, Category = "Round|Character2")
    bool bUsedSnipeKillCooldownReductionThisTurn = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ResourceState, Category = "Round|Character2")
    int32 StationaryShotStacks = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_ResourceState, Category = "Round|Character2")
    bool bHasFreeConcentrateStep = false;

    UFUNCTION(BlueprintPure, Category = "Round|Character2")
    virtual int32 GetMaxAmmo() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character2")
    virtual bool HasAmmo(int32 RequiredAmmo = 1) const;

    UFUNCTION(BlueprintCallable, Category = "Round|Character2")
    virtual int32 ConsumeAmmo(int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category = "Round|Character2")
    virtual int32 RegenerateAmmo(int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category = "Round|Character2")
    virtual void ResolveAmmoShot(bool bKilledTarget, bool bRefundAmmo = false);
    void ResolveConcentrateHit(class ARoundCharacter* TargetCharacter, class ARoundAbilityActor* UsedAbility, bool bKilledTarget, bool bWasBasicAttack);
    void FireConcentrateMagazineVolley(class ARoundCharacter* TargetCharacter);
    void AddPiercingChainMarks(const TArray<class ARoundCharacter*>& Targets, double DamageMultiplier);
    void AddPiercingDefenseStacks(int32 StackCount, int32 ReductionPerStack, int32 DurationInitiative);
    void GrantFreeSuppressionFollowup(class ARoundCharacter* TargetCharacter);
    void FireSuppressionReaction(class ARoundCharacter* TargetCharacter, double DamageMultiplier);

    virtual void InitializeCharacter() override;
    virtual void BeginTurn() override;
    virtual FRoundActionResult AttackField(ARoundGridTile* TargetTile) override;
    virtual FRoundActionResult MoveAlongPath(const TArray<ARoundGridTile*>& Path) override;
    virtual void EndTurn() override;
    virtual int32 ApplyRoundDamage(int32 Amount, ERoundDamageType DamageType, int32 DefenseIgnore, ARoundCharacter* SourceCharacter, bool& bDidDie) override;
    virtual int32 GetTurnDelay() const override;
    virtual int32 GetAttackRange() const override;
    virtual int32 ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const override;

protected:
    virtual int32 GetIncomingBonusDefense(ERoundDamageType DamageType) const override;

    UPROPERTY(Transient)
    bool bHasCompletedTurn = false;

    UPROPERTY(Transient)
    TArray<TObjectPtr<class ARoundCharacter>> PiercingChainMarkedTargets;

    UPROPERTY(Transient)
    double PiercingChainDamageMultiplier = 0.0;

    UPROPERTY(Transient)
    int32 PendingPiercingDefenseStacks = 0;

    UPROPERTY(Transient)
    int32 PiercingDefenseReductionPerStack = 0;

    UPROPERTY(Transient)
    int32 PiercingDefenseReductionDuration = 0;

    UPROPERTY(Transient)
    int32 PiercingMarkedAttackRangeBonus = 0;

    UPROPERTY(Transient)
    TObjectPtr<class ARoundCharacter> FreeSuppressionFollowupTarget = nullptr;

    UFUNCTION()
    void OnRep_ResourceState();
};
