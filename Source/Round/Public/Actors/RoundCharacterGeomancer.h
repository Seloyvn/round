#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundCharacterGeomancer.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundCharacterGeomancer : public ARoundCharacter
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual FRoundActionResult AttackField(class ARoundGridTile* TargetTile) override;
    virtual void AdvanceGlobalInitiative(int32 Amount = 1) override;

    UFUNCTION(BlueprintCallable, Category = "Round|Geomancer")
    void ApplyGeomancerMark(class ARoundCharacter* TargetCharacter);

    UFUNCTION(BlueprintPure, Category = "Round|Geomancer")
    bool HasGeomancerMarkOn(const class ARoundCharacter* TargetCharacter) const;

    UFUNCTION(BlueprintCallable, Category = "Round|Geomancer")
    bool ConsumeGeomancerMarkOn(class ARoundCharacter* TargetCharacter);

    UFUNCTION(BlueprintPure, Category = "Round|Geomancer")
    int32 GetMarkedEnemyCount() const;

    UFUNCTION(BlueprintPure, Category = "Round|Geomancer")
    int32 GetPassivePerTargetCooldownInitiative() const;

    UFUNCTION(BlueprintPure, Category = "Round|Geomancer")
    int32 GetMarkedAttackBonusDamage() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Geomancer")
    TArray<TObjectPtr<ARoundCharacter>> MarkedEnemies;

    UPROPERTY(Transient)
    TMap<TWeakObjectPtr<ARoundCharacter>, int32> PassiveCooldownRemainingByTarget;
};
