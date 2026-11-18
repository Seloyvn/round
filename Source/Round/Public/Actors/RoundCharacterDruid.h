#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundCharacterDruid.generated.h"

class ARoundAbilityActor;
class ARoundGridTile;

UCLASS(Blueprintable)
class ROUND_API ARoundCharacterDruid : public ARoundCharacter
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void BeginTurn() override;
    virtual int32 GetMoveCost() const override;

    UFUNCTION(BlueprintPure, Category = "Round|Druid")
    bool IsBearFormActive() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Druid")
    TArray<ARoundGridTile*> GetConnectedNatureFields() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Druid")
    bool EnterBearForm(ARoundAbilityActor* SourceAbility);

    UFUNCTION(BlueprintCallable, Category = "Round|Druid")
    bool ExitBearForm();

protected:
    virtual bool TryPreventDeathFromPassive(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType) override;
    virtual double GetIncomingDamageMultiplier(ERoundDamageType DamageType) const override;
    virtual void HandleDamageDealt(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType) override;

private:
    void ApplyBearStats(ARoundAbilityActor* SourceAbility);
    void RebuildGrantedAbilitiesForBearForm();
    void RestoreGrantedAbilitiesAfterBearForm();
    ARoundAbilityActor* GetOrCreateBearCancelAbility();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Druid", meta=(AllowPrivateAccess="true"))
    bool bBearFormActive = false;

    UPROPERTY(Transient)
    FRoundStatBlock StoredDruidStats;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Druid", meta=(AllowPrivateAccess="true"))
    int32 StoredDruidHealth = 0;

    UPROPERTY(Transient)
    TArray<TObjectPtr<ARoundAbilityActor>> StoredDruidAbilities;

    UPROPERTY(Transient)
    TObjectPtr<ARoundAbilityActor> BearCancelAbility = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<ARoundAbilityActor> ActiveBearSourceAbility = nullptr;
};
