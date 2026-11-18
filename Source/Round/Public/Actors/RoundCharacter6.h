#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundTypes.h"
#include "RoundCharacter6.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundCharacter6 : public ARoundCharacter
{
    GENERATED_BODY()

public:
    ARoundCharacter6();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void GrantDefaultAbilities() override;
    virtual void BeginTurn() override;
    virtual void AdvanceGlobalInitiative(int32 Amount = 1) override;
    virtual FRoundActionResult UseAbilityIndex(int32 AbilityIndex, ARoundGridTile* TargetTile) override;
    virtual double GetIncomingDamageMultiplier(ERoundDamageType DamageType) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Character6")
    FName PuppetRowName = TEXT("Character6Puppet");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Character6")
    bool bSpawnPuppetOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Round|Character6")
    TObjectPtr<ARoundCharacter> NativePuppetCharacter = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Round|Character6")
    virtual void SetPuppetCharacter(ARoundCharacter* InPuppetCharacter);

    UFUNCTION(BlueprintCallable, Category = "Round|Character6")
    virtual ARoundCharacter* EnsurePuppetCharacter();

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    virtual ARoundCharacter* TryGetPuppetCharacter() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    virtual int32 GetPuppetControlRange() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    virtual bool IsPuppetWithinControlRange() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    virtual bool IsPuppetDefendActive() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    virtual bool IsTargetNearPuppet(const ARoundCharacter* TargetCharacter) const;

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    virtual bool IsTargetInPuppetAttackRange(const ARoundCharacter* TargetCharacter) const;

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    virtual bool WasTargetDamagedByCharacter6ThisTurn(const ARoundCharacter* TargetCharacter) const;

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    virtual bool WasTargetDamagedByPuppetThisTurn(const ARoundCharacter* TargetCharacter) const;

    UFUNCTION(BlueprintCallable, Category = "Round|Character6")
    virtual void SetPendingPuppetDamageMultiplier(double InMultiplier);

    UFUNCTION(BlueprintCallable, Category = "Round|Character6")
    virtual void AddPendingActionInitiativeRefund(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Round|Character6")
    void ApplySimCoordinatorState(bool bInPuppetMovedThisTurn, double InPendingPuppetDamageMultiplier, int32 InPendingActionInitiativeRefund);

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    bool GetPuppetMovedThisTurn() const { return bPuppetMovedThisTurn; }

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    double GetPendingPuppetDamageMultiplier() const { return PendingPuppetDamageMultiplier; }

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    int32 GetPendingActionInitiativeRefund() const { return PendingActionInitiativeRefund; }

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    int32 GetCharacter6StacksAgainstTarget(const ARoundCharacter* TargetCharacter) const { return TargetCharacter ? Character6TargetStacks.FindRef(TargetCharacter) : 0; }

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    int32 GetPuppetStacksAgainstTarget(const ARoundCharacter* TargetCharacter) const { return TargetCharacter ? PuppetTargetStacks.FindRef(TargetCharacter) : 0; }

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    int32 GetInitiativeLossCooldownForTarget(const ARoundCharacter* TargetCharacter) const { return TargetCharacter ? InitiativeLossCooldowns.FindRef(TargetCharacter) : 0; }

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    int32 GetSharedHitCooldownForTarget(const ARoundCharacter* TargetCharacter) const { return TargetCharacter ? SharedHitInitiativeLossCooldowns.FindRef(TargetCharacter) : 0; }

    UFUNCTION(BlueprintPure, Category = "Round|Character6")
    class ARoundAbilityActor* FindGrantedAbilityById(FName AbilityId) const;

    static ARoundCharacter6* ResolveCoordinatorForCharacter(const ARoundCharacter* Character);
    int32 ModifyCoordinatedOutgoingDamage(const ARoundCharacter* DamageSource, ARoundCharacter* TargetCharacter, int32 Amount) const;
    void HandleCoordinatedDamageDealt(const ARoundCharacter* DamageSource, ARoundCharacter* TargetCharacter, int32 AppliedDamage);
    virtual void HandlePassiveDamageTaken(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType) override;

protected:
    virtual void BeginPlay() override;
    void ApplyPuppetRoleStats();
    bool IsCharacterAdjacentToAnyEnemy(const ARoundCharacter* Character) const;
    int32 GetStacksAgainstTarget(const ARoundCharacter* DamageSource, const ARoundCharacter* TargetCharacter) const;
    void AddHuntStackForOtherActor(const ARoundCharacter* DamageSource, ARoundCharacter* TargetCharacter);
    void TickTargetCooldownMap(TMap<TWeakObjectPtr<ARoundCharacter>, int32>& CooldownMap, int32 Amount = 1);

private:
    TObjectPtr<class URoundAbilityData> LoadPuppetMoveAbilityData() const;

    bool bHasCachedPuppetBaseStats = false;
    FRoundStatBlock CachedPuppetBaseStats;
    bool bPuppetMovedThisTurn = false;
    double PendingPuppetDamageMultiplier = 1.0;
    int32 PendingActionInitiativeRefund = 0;
    TMap<TWeakObjectPtr<ARoundCharacter>, int32> Character6TargetStacks;
    TMap<TWeakObjectPtr<ARoundCharacter>, int32> PuppetTargetStacks;
    TMap<TWeakObjectPtr<ARoundCharacter>, int32> InitiativeLossCooldowns;
    TMap<TWeakObjectPtr<ARoundCharacter>, int32> SharedHitInitiativeLossCooldowns;
};
