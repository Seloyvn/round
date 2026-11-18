#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundCharacter1.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundCharacter1 : public ARoundCharacter
{
    GENERATED_BODY()

public:
    ARoundCharacter1();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Character1")
    int32 NativeChainRefills = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_ChainState, Category = "Round|Character1")
    int32 NativeChainCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_ChainState, Category = "Round|Character1")
    int32 NativeChainRechargeProgress = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Character1")
    bool bGrantedChainSpendRefundThisTurn = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Character1")
    bool bPassiveCheatDeathUsed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Character1")
    int32 NativeDamageRechargeCooldownRemaining = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Character1")
    bool bMovedThisTurn = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Character1")
    bool bStationaryGuardActive = false;

    UFUNCTION(BlueprintPure, Category = "Round|Character1")
    virtual int32 GetChainLimit() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character1")
    virtual int32 GetChainRechargeTime() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character1")
    int32 GetCurrentChainCount() const;

    UFUNCTION(BlueprintPure, Category = "Round|Character1")
    bool TryHasChain(int32 Amount = 1) const;

    virtual void InitializeCharacter() override;
    virtual void BeginTurn() override;
    virtual void AdvanceGlobalInitiative(int32 Amount = 1) override;
    virtual void EndTurn() override;
    virtual FRoundActionResult MoveAlongPath(const TArray<ARoundGridTile*>& Path) override;
    virtual void ApplyDelay(int32 Amount) override;
    virtual int32 ApplyRoundDamage(int32 Amount, ERoundDamageType DamageType, int32 DefenseIgnore, ARoundCharacter* SourceCharacter, bool& bDidDie) override;
    virtual double GetIncomingDamageMultiplier(ERoundDamageType DamageType) const override;
    virtual int32 GetIncomingBonusDefense(ERoundDamageType DamageType) const override;
    virtual bool TryPreventDeathFromPassive(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType) override;
    virtual bool TryForcedSetField(ARoundGridTile* NewTile, ARoundCharacter* SourceCharacter, bool bUpdateUI = true) override;

    bool ConsumeChains(int32 Amount, bool bAllowRefund = true);
    int32 AdjustDebuffDurationInitiative(int32 BaseDuration) const;

protected:
    UFUNCTION()
    void OnRep_ChainState();

    bool HasDebuffAppliedToCharacter(const ARoundCharacter* TargetCharacter) const;
    void GainChains(int32 Amount);
    void HandleChainRechargeTick(int32 InitiativeAmount);

    UPROPERTY(Transient)
    bool bChainStateInitialized = false;
};
