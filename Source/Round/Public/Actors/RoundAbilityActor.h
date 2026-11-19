#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoundTypes.h"
#include "RoundAbilityActor.generated.h"

class URoundAbilityData;

UCLASS(Blueprintable)
class ROUND_API ARoundAbilityActor : public AActor
{
    GENERATED_BODY()

public:
    ARoundAbilityActor();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
    TObjectPtr<class ARoundCharacter> OwningCharacter = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
    int64 Points = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_RowName, Category = "Ability")
    FName RowName = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    TObjectPtr<class URoundAbilityData> AbilityData;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentCooldown, Category = "Ability")
    int32 CurrentCooldown = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentCharges, Category = "Ability")
    int32 CurrentCharges = 1;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual ERoundButtonState GetAbilityVisibility() const;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual double GetAbilityValueFromKey(const FString& Key) const;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual TMap<FString, double> GetResolvedAbilityValues() const;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual int32 GetAbilityDurationInitiativeFromKey(const FString& Key) const;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual int32 GetAbilityDamage() const;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual int32 GetAbilityCooldown() const;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual int32 GetAbilityMaxCharges() const;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual bool UsesCharges() const;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual int32 GetAbilityCost() const;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual int32 GetAbilityFragmentCost() const;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual int32 GetAbilityRange() const;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual ERoundDamageType GetAbilityDamageType() const;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual int64 GetAllSkillpoints() const;

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual bool HasSkillpointSkilled(int32 SkillpointIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual void SetCooldown(int32 NewCooldown);

    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual void AdvanceCooldown();

    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual void ReduceCooldown(int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual int32 OnTakeDamage(int32 InDamage);

    UFUNCTION(BlueprintPure, Category = "Ability")
    virtual bool CanTargetTile(const class ARoundGridTile* TargetTile) const;

    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual void GetPreviewTiles(const class ARoundGridTile* TargetTile, TArray<class ARoundGridTile*>& OutTiles) const;

    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile);

    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual FRoundActionResult ExecuteDelayedResolution(class ARoundGridTile* TargetTile);

    UFUNCTION()
    void OnRep_CurrentCooldown();

    UFUNCTION()
    void OnRep_CurrentCharges();

    UFUNCTION()
    void OnRep_RowName();

    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual void InitializeCharges();

    UFUNCTION(BlueprintCallable, Category = "Ability")
    virtual void RefreshRuntimeState();

    virtual void OnGrantedToCharacter();

    UFUNCTION(BlueprintPure, Category = "Ability|Cards")
    virtual bool RequiresHandCardSelection() const;

    UFUNCTION(BlueprintPure, Category = "Ability|Cards")
    virtual int32 GetMinHandCardSelections() const;

    UFUNCTION(BlueprintPure, Category = "Ability|Cards")
    virtual int32 GetMaxHandCardSelections() const;

    UFUNCTION(BlueprintPure, Category = "Ability|Cards")
    virtual bool IsValidHandCardSelectionIndex(int32 CardIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Ability|Cards")
    virtual void SetPendingHandSelectionIndices(const TArray<int32>& InIndices);

    UFUNCTION(BlueprintPure, Category = "Ability|Cards")
    virtual const TArray<int32>& GetPendingHandSelectionIndices() const;

    UFUNCTION(BlueprintCallable, Category = "Ability|Cards")
    virtual void ClearPendingHandSelectionIndices();

protected:
    bool TryGetAbilityValueFromKey(const FString& Key, double& OutValue) const;
    virtual bool HasAvailableCharge() const;
    virtual bool ConsumeCharge();
    virtual bool ConsumeAbilityActivation(const class ARoundGridTile* TargetTile, FRoundActionResult& OutResult);
    virtual bool TryScheduleDelayedAbility(class ARoundGridTile* TargetTile, FRoundActionResult& OutResult);
    virtual FRoundActionResult ExecuteDefaultAbility(class ARoundGridTile* TargetTile);
    virtual FRoundActionResult ExecuteResolvedAbility(class ARoundGridTile* TargetTile);
    virtual int32 ApplyAbilityDamageToTarget(class ARoundCharacter* TargetCharacter, int32 BaseDamage, int32 DefenseIgnore, bool& bDidDie);
    virtual int32 ApplyAbilityDamageToBoardActor(class AActor* BoardActor, int32 BaseDamage, int32 DefenseIgnore, bool& bWasDestroyed);
    virtual int32 ApplyAbilityDamageToTile(class ARoundGridTile* TargetTile, int32 BaseDamage, int32 DefenseIgnore, bool& bAnyDestroyed, bool bIncludeCharacter = true);
    virtual void NotifyAllyAffectedBySpell(class ARoundCharacter* AffectedAlly);
    virtual bool IsTargetInRange(const class ARoundGridTile* TargetTile) const;
    virtual bool MatchesFieldState(const class ARoundGridTile* TargetTile) const;
    virtual class ARoundCharacter* ResolveTargetCharacter(const class ARoundGridTile* TargetTile) const;

    UPROPERTY(Transient)
    TArray<int32> PendingHandSelectionIndices;
};
