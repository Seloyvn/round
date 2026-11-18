#pragma once

#include "Effects/RoundSlowEffect.h"
#include "RoundSuppressingFireEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundSuppressingFireEffect : public ARoundSlowEffect
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") int32 FlatMoveCostAdd = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") int32 BasicAttackCostAdd = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") bool bRevealToAllTeams = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") int32 AllyHitDurationExtension = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") int32 DefenseReduction = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") bool bGrantFreeCauserAttackAfterAllyHit = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") int32 CooldownReductionOnMove = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") int32 AmmoOnKill = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") bool bChainOnKill = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") int32 ChainRange = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") double ReactionDamageMultiplier = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect") bool bRootUntilDamaged = false;

    virtual bool SetHasVision_Implementation(class ARoundCharacter* TargetCharacter, bool bCurrentVision) const override;
    virtual int32 GetMovecost_Implementation(int32 In) const override;
    virtual bool PreventsAbilityUse(const class ARoundAbilityActor* Ability) const override;
    virtual bool PreventsMovement() const override;
    virtual int32 ModifyIncomingDamage(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;
    virtual int32 ModifyBasicAttackCost(class ARoundCharacter* AttackingCharacter, int32 AttackCost) const override;
    virtual void OnBasicAttackExecuted(class ARoundCharacter* AttackingCharacter, class ARoundCharacter* TargetCharacter) override;
    virtual void OnOwnerMoved(class ARoundCharacter* TargetCharacter, class ARoundGridTile* PreviousTile, class ARoundGridTile* NewTile) override;
    virtual void OnDamageTaken(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 AppliedDamage, ERoundDamageType DamageType) override;

private:
    bool bBasicAttackTaxAvailable = true;
    bool bMovementReactionUsed = false;
    bool bKillResolved = false;
    bool bRootBroken = false;
    TSet<TWeakObjectPtr<class ARoundCharacter>> AlliesThatExtendedDuration;
};
