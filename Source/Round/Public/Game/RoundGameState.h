#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "RoundTypes.h"
#include "Core/RoundCoreMatchAdapter.h"
#include "RoundGameState.generated.h"

class ARoundCharacter;
class AActor;
class ARoundGridTile;
class ARoundPlayerPawn;
class ARoundTeamActor;
class ARoundObjectiveActor;
class ARoundObjectiveRewardActor;
class URoundObjectiveRewardData;
class UTextRenderComponent;
class UStaticMesh;
class UStaticMeshComponent;
class USoundBase;
class URoundPresentationProfileData;
class URoundTerrainData;
class URoundTerrainRegionRegistryData;
class URoundTerrainRegistryData;
class URoundGameDataRegistry;

USTRUCT(BlueprintType)
struct FRoundMatchFieldSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FVector2D Position = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    ERoundTerrainType TerrainType = ERoundTerrainType::Ground;
};

USTRUCT(BlueprintType)
struct FRoundMatchCharacterSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FString CharacterInstanceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FString CharacterRowName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    ERoundTeam Team = ERoundTeam::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FVector2D SpawnPosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    int64 CharacterPassiveSelections = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    TArray<FRoundSkillEntry> AbilitySkillSelections;
};

USTRUCT(BlueprintType)
struct FRoundMatchEventRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    int32 Sequence = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    int32 TurnNumber = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FString TimestampUtc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FString EventType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FString SourceCharacterId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FString TargetCharacterId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FString AbilityId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FVector2D TilePosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FVector2D SecondaryTilePosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    int32 Amount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    int32 SecondaryAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    bool bSuccess = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FString Reason;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    TMap<FString, FString> StringData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    TMap<FString, double> NumericData;
};

USTRUCT(BlueprintType)
struct FRoundMatchRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FString MatchId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FString StartedAtUtc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FString FinishedAtUtc;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    ERoundFormat Format = ERoundFormat::Format3x16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    FString FormatName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    int32 FieldSize = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    int32 TerrainAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    int32 SkillAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    ERoundTeam WinnerTeam = ERoundTeam::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    TArray<FRoundTeamDefinition> Teams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    TArray<FRoundMatchFieldSnapshot> Fields;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    TArray<FRoundMatchCharacterSnapshot> SpawnedCharacters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Match")
    TArray<FRoundMatchEventRecord> Events;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTurnChanged, ARoundCharacter*, ActiveCharacter);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBoardStateChanged);

USTRUCT(BlueprintType)
struct FRoundObjectivePreviewEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Objective")
    ERoundObjectiveType ObjectiveType = ERoundObjectiveType::HoldZone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Objective")
    FString RewardName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Objective")
    int32 SpawnAtGlobalInitiative = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Objective")
    TObjectPtr<ARoundGridTile> TargetTile = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Objective")
    FString Description;
};

USTRUCT()
struct FRoundPresentationEventRecord
{
    GENERATED_BODY()

    UPROPERTY()
    FString EventType;

    UPROPERTY()
    FString SourceCharacterId;

    UPROPERTY()
    FString TargetCharacterId;

    UPROPERTY()
    FString AbilityId;

    UPROPERTY()
    FVector2D TilePosition = FVector2D::ZeroVector;

    UPROPERTY()
    FVector2D SecondaryTilePosition = FVector2D::ZeroVector;

    UPROPERTY()
    float Amount = 0.0f;

    UPROPERTY()
    bool bSuccess = false;

    UPROPERTY()
    ERoundTeam SourceTeam = ERoundTeam::Neutral;

    UPROPERTY()
    ERoundTeam TargetTeam = ERoundTeam::Neutral;
};

USTRUCT()
struct FRoundFloatingTextEntry
{
    GENERATED_BODY()

    UPROPERTY(Transient)
    TObjectPtr<UTextRenderComponent> Component = nullptr;

    FVector StartWorldLocation = FVector::ZeroVector;
    FVector EndWorldLocation = FVector::ZeroVector;
    FLinearColor Color = FLinearColor::White;
    float ElapsedSeconds = 0.0f;
    float DurationSeconds = 0.0f;
};

USTRUCT()
struct FRoundTransientMeshEntry
{
    GENERATED_BODY()

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> Component = nullptr;

    FVector StartWorldLocation = FVector::ZeroVector;
    FVector EndWorldLocation = FVector::ZeroVector;
    FVector StartScale = FVector::OneVector;
    FVector EndScale = FVector::OneVector;
    float ElapsedSeconds = 0.0f;
    float DurationSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct FRoundPresentationProfile
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Presentation")
    FLinearColor TextColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Presentation")
    float TextHeightOffset = 125.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Presentation")
    float BeamThicknessScale = 0.14f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Presentation")
    FVector PulseStartScale = FVector(0.18f, 0.18f, 0.18f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Presentation")
    FVector PulseEndScale = FVector(0.44f, 0.44f, 0.44f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Presentation")
    float SoundVolumeMultiplier = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Presentation")
    float SoundPitchMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Presentation")
    float SoundMaxDurationSeconds = 0.12f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Presentation")
    float ScalePunch = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Presentation")
    FString TextOverride;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Presentation")
    TObjectPtr<USoundBase> SoundOverride = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API ARoundGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ARoundGameState();
    virtual ~ARoundGameState() override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintPure, Category = "Round", meta = (WorldContext = "WorldContextObject"))
    static ARoundGameState* GetRoundGameState(const UObject* WorldContextObject);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain")
    TSubclassOf<ARoundGridTile> DefaultFieldClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain")
    TMap<ERoundTerrainType, TSubclassOf<ARoundGridTile>> TerrainFieldClasses;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain")
    TMap<ERoundTerrainType, int32> TerrainClusterSizes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain")
    TObjectPtr<URoundTerrainRegistryData> TerrainRegistryData = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain")
    TObjectPtr<URoundTerrainRegionRegistryData> TerrainRegionRegistryData = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Setup")
    TObjectPtr<URoundGameDataRegistry> GameDataRegistry = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain")
    TArray<FVector2D> InitialFieldPositions;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain")
    TArray<ERoundTerrainType> InitialTerrainSequence;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Setup")
    bool bCreateTeamsOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Presentation")
    TMap<FString, FRoundPresentationProfile> PresentationProfilesByEvent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Presentation")
    TMap<FString, FRoundPresentationProfile> PresentationProfilesByAbility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Setup")
    int32 InitialFieldSize = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Setup")
    FVector2D InitialFieldStartPosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Setup")
    int32 InitialSkillAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Setup")
    FString MatchSeedOverride;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain")
    int32 InitialTerrainAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Terrain")
    TMap<ERoundTerrainType, float> InitialTerrainWeights;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    TArray<ERoundTerrainType> TerrainTypes;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Setup")
    bool bFinishedGenerating = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Teams")
    TSubclassOf<AActor> TeamActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Characters")
    TMap<FName, TSubclassOf<ARoundCharacter>> CharacterRowClasses;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Objectives")
    TArray<TSoftObjectPtr<URoundObjectiveRewardData>> ObjectiveRewardPool;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Objectives")
    TSubclassOf<ARoundObjectiveActor> ObjectiveActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Objectives")
    TSubclassOf<ARoundObjectiveRewardActor> ObjectiveRewardActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Teams")
    FTransform TeamOneSpawnTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Teams")
    FTransform TeamTwoSpawnTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TArray<TObjectPtr<UObject>> Teams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<UObject> Player1 = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<UObject> Player2 = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round")
    TArray<TObjectPtr<class ARoundGridTile>> Fields;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round")
    TArray<TObjectPtr<class ARoundCharacter>> RegisteredCharacters;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round")
    TArray<FRoundTurnEntry> TurnOrder;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round")
    int32 ActiveTurnIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentTurnCharacter, Category = "Round")
    TObjectPtr<ARoundCharacter> CurrentTurnCharacter = nullptr;

    UPROPERTY(BlueprintAssignable, Category = "Round")
    FOnTurnChanged OnTurnChanged;

    UPROPERTY(BlueprintAssignable, Category = "Round")
    FOnBoardStateChanged OnBoardStateChanged;

    UFUNCTION(BlueprintCallable, Category = "Round")
    void RegisterField(class ARoundGridTile* Field);

    UFUNCTION(BlueprintCallable, Category = "Round")
    void RegisterCharacter(ARoundCharacter* Character);

    UFUNCTION(BlueprintPure, Category = "Round")
    TArray<ARoundGridTile*> GetFields() const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    ARoundGridTile* GetField(const FVector2D& Position) const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    ARoundGridTile* GetFieldAt(const FVector2D& Position) const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    ARoundGridTile* GetCloseFreeField(ARoundGridTile* From) const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    ARoundGridTile* GetCloseFreeFieldFrom(ARoundGridTile* Target, ARoundGridTile* From) const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    ARoundCharacter* SpawnCharacter(TSubclassOf<ARoundCharacter> CharacterClass, const FVector2D& Position, UObject* TeamObject, int64 Skills, int64 CharacterPassiveSelections, const TArray<FRoundSkillEntry>& AbilitySkillSelections, FName RowName);

    UFUNCTION(BlueprintCallable, Category = "Round")
    ARoundCharacter* SpawnCharacterByRowName(FName RowName, const FVector2D& Position, UObject* TeamObject, int64 Skills, int64 CharacterPassiveSelections, const TArray<FRoundSkillEntry>& AbilitySkillSelections);

    UFUNCTION(BlueprintCallable, Category = "Round")
    AActor* SpawnTrap(TSubclassOf<AActor> TrapClass, int32 Duration, ARoundGridTile* Field, ARoundCharacter* OwnerCharacter);

    UFUNCTION(BlueprintCallable, Category = "Round")
    void CreateTeams();

    UFUNCTION(BlueprintCallable, Category = "Round|Teams")
    bool TryCreateTeamsFromPlayerSelections();

    UFUNCTION(BlueprintCallable, Category = "Round")
    void CreateFieldFromParameters(int32 FieldSize, FVector2D StartPosition, int32 SkillAmount, int32 TerrainAmount, const TMap<ERoundTerrainType, float>& TerrainWeights);

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    const URoundTerrainData* GetTerrainData(ERoundTerrainType TerrainType) const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    TArray<ERoundTerrainType> OrderTerrainTypes(const TArray<ERoundTerrainType>& InTerrainTypes) const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    ARoundGridTile* SpawnFieldAt(const FVector2D& GridPosition, ERoundTerrainType TerrainType);

    UFUNCTION(BlueprintCallable, Category = "Round")
    void CreateTerrain(ERoundTerrainType TerrainType);

    UFUNCTION(BlueprintCallable, Category = "Round")
    void ApplyTerrainTypeToField(ARoundGridTile* Field, ERoundTerrainType TerrainType);

    UFUNCTION(BlueprintPure, Category = "Round")
    TArray<ARoundGridTile*> GetNeighborFields(ARoundGridTile* Field) const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    TArray<ARoundGridTile*> GetFieldsBetween(ARoundGridTile* FromField, ARoundGridTile* ToField, bool bIncludeEndpoints = true) const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    void CharacterDie(ARoundCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Round")
    ARoundCharacter* SetNextTurn();

    UFUNCTION(BlueprintCallable, Category = "Round")
    void SetVision();

    UFUNCTION(BlueprintCallable, Category = "Round")
    void RebuildTurnOrder();

    UFUNCTION(BlueprintCallable, Category = "Round")
    ARoundCharacter* GetActiveCharacter() const;

    UFUNCTION(BlueprintCallable, Category = "Round")
    bool AdvanceTurn();

    UFUNCTION(BlueprintCallable, Category = "Round|Core")
    bool IsCoreMoveLegal(const ARoundCharacter* Character, const ARoundGridTile* TargetTile) const;

    bool GetCoreVisionFields(const ARoundCharacter* Character, TArray<ARoundGridTile*>& OutVisibleFields) const;

    UFUNCTION(Exec, BlueprintCallable, Category = "Round|Core|Training")
    bool ExportCoreTrainingPackage(const FString& FilePath);

    FRoundActionResult ExecuteCoreMove(ARoundCharacter* Character, ARoundGridTile* TargetTile);
    FRoundActionResult ExecuteCoreMovePath(ARoundCharacter* Character, const TArray<ARoundGridTile*>& Path);
    ARoundCharacter* AdvanceCoreTurn(ARoundCharacter* EndingCharacter);

    UFUNCTION(BlueprintCallable, Category = "Round|Deprecated", meta = (DeprecatedFunction, DeprecationMessage = "Use IsCoreMoveLegal. This now forwards to the authoritative-core comparison adapter."))
    bool IsSimMoveLegal(const ARoundCharacter* Character, const ARoundGridTile* TargetTile) const;

    UFUNCTION(BlueprintCallable, Category = "Round|Sim")
    bool IsSimAttackLegal(const ARoundCharacter* Character, const ARoundGridTile* TargetTile) const;

    UFUNCTION(BlueprintCallable, Category = "Round|Sim")
    bool IsSimAbilityLegal(const ARoundCharacter* Character, int32 AbilityIndex, const ARoundGridTile* TargetTile) const;

    UFUNCTION(BlueprintPure, Category = "Round|Objectives")
    ARoundObjectiveActor* GetCurrentObjectiveActor() const { return CurrentObjectiveActor; }

    UFUNCTION(BlueprintCallable, Category = "Round|Objectives")
    TArray<FRoundObjectivePreviewEntry> GetObjectivePreviewEntries() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Objectives")
    bool TryStartObjectiveClaim(ARoundCharacter* Character, FString& OutReason);

    UFUNCTION(BlueprintPure, Category = "Round|Objectives")
    bool IsCharacterBlockedByObjectiveChannel(const ARoundCharacter* Character) const;

    UFUNCTION(BlueprintPure, Category = "Round|Objectives")
    bool CanCharacterStartObjectiveClaim(const ARoundCharacter* Character, FString& OutReason) const;

    UFUNCTION(BlueprintPure, Category = "Round|Match")
    int32 GetMatchRandomSeed() const { return MatchRandomSeed; }

    int32 GetRandomRange(int32 MinInclusive, int32 MaxInclusive) const;
    float GetRandomFraction(float MinInclusive, float MaxInclusive) const;

    bool ShouldUseSimMoveValidation() const;
    bool ShouldUseSimAttackValidation() const;
    bool ShouldUseSimAbilityValidation() const;
    bool ShouldRunSimShadowValidation() const;
    bool ShouldLogSimProfiling() const;
    bool ShouldUseSimSimpleReconciliation() const;
    bool ShouldUseSimAuthoritativeSimple() const;
    bool ShouldUseSimAuthoritativeMove() const;
    bool ShouldUseSimAuthoritativeAttack() const;
    bool ShouldUseSimAuthoritativeAbility() const;
    bool ShouldUseSimAuthoritativePuppetAbility() const;
    bool ShouldExecuteSimAttackResult() const;
    bool ShouldExecuteSimAbilityResult() const;
    bool ExecutePythonLiveStep(const FString& PolicyName, FString& OutReason, float* OutNextAutoplayDelaySeconds = nullptr);
    void StartPythonLiveAutoplay(const FString& PolicyName, float InitialDelaySeconds = -1.0f);
    void StopPythonLiveAutoplay();
    bool IsPythonLiveAutoplayEnabled() const { return bPythonLiveAutoplayEnabled; }
    bool IsReplayActive() const { return bReplayActive; }
    bool LoadReplayFromFile(const FString& FilePath, FString& OutError);
    void StopReplay();

    const FRoundMatchRecord& GetCurrentMatchRecord() const { return CurrentMatchRecord; }

    UFUNCTION(BlueprintPure, Category = "Round|Sim")
    int32 GetRecordedTurnCounter() const { return MatchTurnCounter; }

    UFUNCTION(BlueprintPure, Category = "Round|Sim")
    int32 GetRecordedGlobalInitiativeElapsed() const { return MatchGlobalInitiativeElapsed; }
    double GetGlobalDamageEscalationMultiplier() const;
    USoundBase* GetPresentationSoundForEvent(const FString& EventType) const;

    void RecordCharacterSpawned(ARoundCharacter* Character);
    void RecordTurnStarted(ARoundCharacter* Character);
    void RecordTurnEnded(ARoundCharacter* Character);
    void RecordMovement(ARoundCharacter* Character, const FVector2D& FromPosition, const TArray<FVector2D>& TraversedPositions, int32 TotalCost);
    void RecordBasicAttack(ARoundCharacter* SourceCharacter, ARoundCharacter* TargetCharacter, int32 DamageDone);
    void RecordAbilityUse(ARoundCharacter* SourceCharacter, FName AbilityId, ARoundGridTile* TargetTile, ARoundCharacter* TargetCharacter, const FRoundActionResult& Result);
    void RecordDamage(ARoundCharacter* SourceCharacter, ARoundCharacter* TargetCharacter, int32 RequestedDamage, int32 AppliedDamage, ERoundDamageType DamageType, int32 DefenseIgnore, bool bCrit);
    void RecordHeal(ARoundCharacter* TargetCharacter, int32 HealAmount);
    void RecordDelayApplied(ARoundCharacter* TargetCharacter, int32 Amount);
    void RecordCharacterDeath(ARoundCharacter* Character);

private:
    UFUNCTION(NetMulticast, Reliable)
    void MulticastEnqueuePresentationEvent(const FRoundPresentationEventRecord& EventRecord);

    UFUNCTION()
    void OnRep_CurrentTurnCharacter();

    void ScheduleInitialVisionRefresh();
    TMap<ERoundTerrainType, float> BuildDefaultTerrainWeights() const;
    void EnsureInitialFields();
    void GenerateInitialTerrain();

    bool GenerateTerrainRegions();
    void ResetBoardFields();
    TArray<FVector2D> BuildHexSpiralPositions(FVector2D StartPosition, int32 Radius) const;
    ERoundTerrainType GetWeightedTerrainType(const TMap<ERoundTerrainType, float>& TerrainWeights) const;
    void InvokeNoArgFunction(UObject* TargetObject, const FName FunctionName) const;
    void NotifyVisionUpdated(UObject* PlayerObject) const;
    void TrySetObjectProperty(UObject* TargetObject, const FName PropertyName, UObject* Value) const;
    void TrySetIntProperty(UObject* TargetObject, const FName PropertyName, int32 Value) const;
    void TrySetInt64Property(UObject* TargetObject, const FName PropertyName, int64 Value) const;
    void TrySetNameProperty(UObject* TargetObject, const FName PropertyName, FName Value) const;
    void ConfigureFieldForTerrain(ARoundGridTile* Field, ERoundTerrainType TerrainType) const;
    void InitializeMatchRecording();
    void ResetAiKnowledge();
    FString ResolveCharacterRecordId(const ARoundCharacter* Character) const;
    void AppendMatchEvent(const FRoundMatchEventRecord& EventRecord);
    bool ShouldQueuePresentationEvent(const FRoundPresentationEventRecord& EventRecord) const;
    void EnqueuePresentationEvent(const FRoundPresentationEventRecord& EventRecord);
    void TryStartNextPresentationEvent();
    void UpdateActivePresentationEvent(float DeltaSeconds);
    void FinishActivePresentationEvent();
    FRoundPresentationEventRecord BuildPresentationEvent(const FRoundMatchEventRecord& EventRecord) const;
    ARoundCharacter* FindLiveCharacterByRecordId(const FString& CharacterId) const;
    FVector ResolveTileWorldLocation(const FVector2D& TilePosition) const;
    void SpawnFloatingText(const FVector& WorldLocation, const FString& Text, const FLinearColor& Color);
    void UpdateFloatingTexts(float DeltaSeconds);
    void SpawnTransientSpherePulse(const FVector& WorldLocation, float DurationSeconds, const FVector& StartScale, const FVector& EndScale);
    void SpawnTransientBeam(const FVector& StartWorldLocation, const FVector& EndWorldLocation, float DurationSeconds, float ThicknessScale);
    void UpdateTransientMeshes(float DeltaSeconds);
    void InitializePresentationProfiles();
    FRoundPresentationProfile ResolvePresentationProfile(const FRoundPresentationEventRecord& EventRecord) const;
    bool CanPlayerPerceivePresentationEvent(const ARoundPlayerPawn* PlayerPawn, const FRoundPresentationEventRecord& EventRecord) const;
    void DispatchPresentationSoundToPlayers(const FRoundPresentationEventRecord& EventRecord, float VolumeMultiplier, float PitchMultiplier, float MaxDurationSeconds) const;
    bool CanAnyLocalPlayerPerceivePresentationEvent(const FRoundPresentationEventRecord& EventRecord) const;
    void PlayPresentationSound(USoundBase* Sound, const FVector& WorldLocation, float VolumeMultiplier = 1.0f, float PitchMultiplier = 1.0f, float MaxDurationSeconds = 0.0f) const;
    void TryFinalizeMatchRecordingAfterDeath();
    void FinalizeAndSaveMatchRecording(ERoundTeam WinnerTeam);
    void ResetReplayBoard();
    void StartReplay(const FRoundMatchRecord& MatchRecord);
    void AdvanceReplay();
    void ApplyReplayEvent(const FRoundMatchEventRecord& EventRecord);
    void RefreshReplayVisionForPlayers() const;
    ARoundTeamActor* SpawnReplayTeamActor(ERoundTeam TeamId, const FRoundTeamDefinition& TeamDefinition);
    ARoundCharacter* FindReplayCharacterByRecordId(const FString& CharacterId) const;
    ARoundCharacter* FindReplayCharacterAtTile(const FVector2D& TilePosition) const;
    void EnsureObjectiveCycleInitialized();
    void AdvanceObjectivesByInitiative(int32 InitiativeAmount);
    void SpawnNextObjectivePreview();
    TArray<ARoundGridTile*> BuildObjectiveZone(ARoundGridTile* CenterTile) const;
    ARoundGridTile* FindObjectiveSpawnTile() const;
    URoundObjectiveRewardData* ChooseObjectiveRewardData() const;

    FTimerHandle InitialVisionRefreshTimerHandle;
    FTimerHandle PythonLiveAutoplayTimerHandle;
    FTimerHandle ReplayTimerHandle;
    int32 RemainingInitialVisionRefreshes = 0;
    FRoundMatchRecord CurrentMatchRecord;
    int32 MatchEventSequence = 0;
    int32 MatchTurnCounter = 0;
    UPROPERTY(Replicated)
    int32 MatchGlobalInitiativeElapsed = 0;

    UPROPERTY(ReplicatedUsing=OnRep_MatchRandomSeed)
    int32 MatchRandomSeed = 0;
    bool bMatchRecordingFinalized = false;
    bool bPythonLiveAutoplayEnabled = false;
    TArray<FRoundPresentationEventRecord> PendingPresentationEvents;
    FRoundPresentationEventRecord ActivePresentationEvent;
    TWeakObjectPtr<ARoundCharacter> ActivePresentationSourceCharacter;
    TWeakObjectPtr<ARoundCharacter> ActivePresentationTargetCharacter;
    FVector ActivePresentationStartLocation = FVector::ZeroVector;
    FVector ActivePresentationEndLocation = FVector::ZeroVector;
    FVector ActivePresentationSourceBaseScale = FVector::OneVector;
    FVector ActivePresentationTargetBaseScale = FVector::OneVector;
    FRoundPresentationProfile ActivePresentationProfile;
    float ActivePresentationElapsedSeconds = 0.0f;
    float ActivePresentationDurationSeconds = 0.0f;
    bool bPresentationEventActive = false;
    TArray<FRoundFloatingTextEntry> ActiveFloatingTexts;
    TArray<FRoundTransientMeshEntry> ActiveTransientMeshes;
    FString PythonLiveAutoplayPolicyName;
    TMap<TWeakObjectPtr<ARoundCharacter>, FString> CharacterRecordIds;
    TMap<FString, TWeakObjectPtr<ARoundCharacter>> ReplayCharactersByRecordId;
    FRoundMatchRecord ActiveReplayRecord;
    int32 ReplayNextEventIndex = 0;
    bool bReplayActive = false;
    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> PresentationSphereMesh = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> PresentationCylinderMesh = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<USoundBase> PresentationMoveSound = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<USoundBase> PresentationAttackSound = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<USoundBase> PresentationAbilitySound = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<USoundBase> PresentationImpactSound = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<USoundBase> PresentationHealSound = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<USoundBase> PresentationTurnSound = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<USoundBase> PresentationDeathSound = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<ARoundObjectiveActor> CurrentObjectiveActor = nullptr;

    int32 NextObjectivePreviewAtGlobalInitiative = 100;
    bool bObjectiveCycleInitialized = false;
    mutable FRandomStream MatchRandomStream;
    mutable bool bMatchRandomStreamInitialized = false;
    mutable TUniquePtr<FRoundCoreMatchAdapter> CoreMatchAdapter;
    UFUNCTION()
    void OnRep_MatchRandomSeed();

    void InitializeMatchRandomSeed() const;
};
