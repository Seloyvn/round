#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundCharacterDragoon.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundCharacterDragoon : public ARoundCharacter
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginTurn() override;
    virtual void EndTurn() override;
    virtual int32 ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const override;
    virtual void HandleEnemyHitByCharacter(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType) override;

    UFUNCTION(BlueprintPure, Category = "Round|Dragoon")
    int32 GetMomentumStacks() const;

    UFUNCTION(BlueprintPure, Category = "Round|Dragoon")
    int32 GetMomentumMaxStacks() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Dragoon")
    int32 GainMomentumStacks(int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category = "Round|Dragoon")
    void RecordEnemyHit(class ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType);

    UFUNCTION(BlueprintCallable, Category = "Round|Dragoon")
    void ResetMomentumStacks();

    UFUNCTION(BlueprintPure, Category = "Round|Dragoon")
    int32 GetDamagePerStack() const;

    bool HasHitEnemyThisTurn() const { return bHitEnemyThisTurn; }

    void ApplyCoreDragoonState(int32 InMomentumStacks, bool bInHitEnemyThisTurn);

protected:
    virtual int32 GetIncomingBonusDefense(ERoundDamageType DamageType) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Dragoon")
    int32 MomentumStacks = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Dragoon")
    bool bHitEnemyThisTurn = false;
};
