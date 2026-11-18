#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RoundTypes.h"
#include "RoundCharacter.generated.h"

class ARoundAbilityActor;
class ARoundEffectActor;
class ARoundGridTile;
class ARoundGameState;
class ARoundTeamActor;
class URoundAttributeSetComponent;
class URoundCharacterData;
class URoundGameUIWidget;
class UWidgetComponent;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoundCharacterDied, ARoundCharacter*, Character);

UCLASS(Blueprintable)
class ROUND_API ARoundCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ARoundCharacter();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round")
    TObjectPtr<class URoundCharacterData> CharacterData;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, ReplicatedUsing=OnRep_CharacterDataAsset, Category = "Round")
    TSoftObjectPtr<class URoundCharacterData> CharacterDataAsset;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round")
    ERoundTeam Team = ERoundTeam::Neutral;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round")
    TObjectPtr<class URoundAttributeSetComponent> Attributes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TObjectPtr<UWidgetComponent> HealthbarWidgetComponent = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_GrantedAbilities, Category = "Round")
    TArray<TObjectPtr<class ARoundAbilityActor>> GrantedAbilities;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round")
    TArray<TObjectPtr<class ARoundEffectActor>> ActiveEffects;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentTile, Category = "Round")
    TObjectPtr<ARoundGridTile> CurrentTile = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round")
    TObjectPtr<ARoundTeamActor> OwningTeamActor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CharacterRowName, Category = "Round")
    FName CharacterRowName = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CharacterPassiveSelections, Category = "Round")
    int64 CharacterPassiveSelections = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_AbilitySkillSelections, Category = "Round")
    TArray<FRoundSkillEntry> AbilitySkillSelections;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Round|Native")
    int32 NativeDelayedActionInitiative = 0;

    UPROPERTY(Transient)
    bool bDamageContextIsAbility = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Native")
    TArray<TObjectPtr<ARoundGridTile>> HoverFields;

    UPROPERTY(BlueprintAssignable, Category = "Round")
    FOnRoundCharacterDied OnCharacterDied;

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual void InitializeCharacter();

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual void GrantDefaultAbilities();

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual int32 Heal(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual void ApplyDelay(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual int32 ApplyRoundDamage(int32 Amount, ERoundDamageType DamageType, int32 DefenseIgnore, ARoundCharacter* SourceCharacter, bool& bDidDie);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual int32 ApplyDirectDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* SourceCharacter, bool& bDidDie);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual int32 ApplyUnreactiveFinalDamage(int32 Amount, bool& bDidDie);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual FRoundActionResult AttackField(ARoundGridTile* TargetTile);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual FRoundActionResult MoveToTile(ARoundGridTile* TargetTile);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual FRoundActionResult MoveAlongPath(const TArray<ARoundGridTile*>& Path);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual void SetField(ARoundGridTile* NewTile, bool bUpdateUI = true);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual bool TryForcedSetField(ARoundGridTile* NewTile, ARoundCharacter* SourceCharacter, bool bUpdateUI = true);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual void SetOwningTeamActor(ARoundTeamActor* InTeamActor);

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual ARoundGameState* GetRoundGameState() const;

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual ARoundTeamActor* GetOwningTeamActor() const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual bool IsEnemyFor(const ARoundCharacter* OtherCharacter) const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual bool EnemyField(ARoundGridTile* TargetTile) const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual bool CanMoveTo(ARoundGridTile* TargetTile) const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual int32 GetMoveCost() const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual bool CanGetTurn() const;

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual int32 InitiativeToTurn() const;

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual int32 RemainInitiative() const;

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual double GetValueFromKey(const FString& Key) const;
    virtual double GetPassiveValueFromKey(const FString& Key) const;
    virtual int32 ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const;

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual double GetCritChance() const;

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual double GetCritModifier() const;

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual int32 GetAttackCost() const;

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual int32 GetAttackRange() const;

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual int32 GetTurnDelay() const;

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual TArray<ARoundGridTile*> GetVisionFields() const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual bool HasVision(const ARoundTeamActor* TeamActor) const;

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual bool HasTurnActionBlock() const;

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual bool HasMovementBlock() const;

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual int32 ModifyInitiativeGainFromEffects(int32 Amount) const;

    virtual bool TryBuildMovePath(ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutPath, int32& OutCost, bool bTreatTilesWithoutVisionAsEmpty = false) const;

    virtual void ClearHoverFields();

    virtual void HoverFieldArray(const TArray<ARoundGridTile*>& Fields, ERoundHoverFieldState State);

    virtual void AttackHover(ARoundGridTile* TargetTile);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual void BeginTurn();

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual void AdvanceGlobalInitiative(int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual void AddEffect(class ARoundEffectActor* EffectActor);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual void RemoveEffect(class ARoundEffectActor* EffectActor);

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual FRoundActionResult UseAbilityIndex(int32 AbilityIndex, ARoundGridTile* TargetTile);

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual ERoundButtonState UseableAbilityVisisbility(bool bUseable, ERoundButtonState InputState) const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual void EndTurn();

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual void Die();

    UFUNCTION(BlueprintPure, Category = "Round")
    virtual bool IsAlive() const;

protected:
    virtual void BeginPlay() override;
    void NotifyRoundStateChanged();

    UFUNCTION()
    void OnRep_CurrentTile();

    UFUNCTION()
    void OnRep_CharacterRowName();

    UFUNCTION()
    void OnRep_CharacterDataAsset();

    UFUNCTION()
    void OnRep_CharacterPassiveSelections();

    UFUNCTION()
    void OnRep_AbilitySkillSelections();

    UFUNCTION()
    void OnRep_GrantedAbilities();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastHandleDeathVisuals();

    virtual TSoftObjectPtr<URoundCharacterData> ResolveCharacterDataAsset() const;
    void LoadConfiguredCharacterData();
    void RefreshHealthbarWidget();
    void RecalculateNativeDelayedActionInitiative();
    virtual void UpdateFacingAfterTileChange(ARoundGridTile* PreviousTile, ARoundGridTile* NewTile);
    virtual void FaceTargetTile(const ARoundGridTile* TargetTile);
    virtual void FaceWorldLocation(const FVector& WorldLocation);
    virtual FVector GetBoardCenterWorldLocation() const;
    virtual void RebuildCharacterPassiveValues();
    virtual void ApplyCharacterPassiveStats(FRoundStatBlock& InOutStats) const;
    virtual double GetIncomingDamageMultiplier(ERoundDamageType DamageType) const;
    virtual int32 GetIncomingBonusDefense(ERoundDamageType DamageType) const;
    virtual void HandlePassiveDamageTaken(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType);
    virtual void HandleEnemyHitByCharacter(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType);
    virtual void HandleDamageDealt(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType);
    virtual bool TryPreventDeathFromPassive(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType);
    bool TryResolveCharacterPassiveLevels(TArray<FRoundSkillpointDetails>& OutLevels) const;
    virtual void ApplyAbilitySkillSelectionsToGrantedAbilities();
    virtual int64 ResolveAbilitySkillMask(FName AbilityId) const;
    virtual int32 GetAllowedAbilityCount() const;

    UPROPERTY(Transient)
    TObjectPtr<URoundCharacterData> AppliedCharacterData = nullptr;

    UPROPERTY(Transient)
    TMap<FString, double> CharacterPassiveValues;
};
