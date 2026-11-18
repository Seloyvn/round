#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RoundTypes.h"
#include "RoundPlayerPawn.generated.h"

class ARoundCharacter;
class ARoundGameState;
class ARoundGridTile;
class ARoundTeamActor;
class ARoundAbilityActor;
class UUserWidget;
class USoundBase;
class URoundDebugWidget;

UCLASS(Blueprintable)
class ROUND_API ARoundPlayerPawn : public APawn
{
    GENERATED_BODY()

public:
    ARoundPlayerPawn();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_Team, Category = "Round|Player")
    ERoundTeam Team = ERoundTeam::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Round|Player")
    FRoundTeamDefinition TeamDefinition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_TeamActor, Category = "Round|Player")
    TObjectPtr<ARoundTeamActor> TeamActor = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Player")
    TObjectPtr<ARoundGridTile> SelectedField = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Player")
    TObjectPtr<ARoundGridTile> HoveredField = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<ARoundCharacter> SelectedCharacterOverride = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Player")
    ERoundActionType SelectedAction = ERoundActionType::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Player")
    int32 SelectedAbilityIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Player")
    TSubclassOf<UUserWidget> GameUIClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Camera")
    bool bSnapCameraToSelectedField = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Camera", meta=(ClampMin="0.0"))
    float CameraEdgePanPixels = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Camera", meta=(ClampMin="0.0"))
    float CameraMoveSpeed = 700.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Camera", meta=(ClampMin="0.0"))
    float CameraFieldBoundsPadding = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Camera", meta=(ClampMin="0.0"))
    float CameraSnapInterpolationSpeed = 20.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Player")
    TObjectPtr<UUserWidget> UI = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Debug")
    TObjectPtr<URoundDebugWidget> DebugUI = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Debug")
    bool bDebugRevealMap = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Debug")
    bool bDebugTerrainLabels = false;

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void SetTeamActor(ARoundTeamActor* InTeamActor);

    UFUNCTION(BlueprintPure, Category = "Round|Player")
    virtual ARoundGameState* GetRoundGameState() const;

    UFUNCTION(BlueprintPure, Category = "Round|Player")
    virtual ARoundCharacter* SelectedCharacter() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void SelectField(ARoundGridTile* Field);

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void SetUI();

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void AbilitySelected(int32 AbilityIndex);

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void UseAbilityCmd(ARoundGridTile* Field, int32 AbilityIndex);

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void Unselect();

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void Attack();

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void Move();

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void EndTurnServer();

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void EndTurn();

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void UseAbilityDirect(int32 AbilityIndex);

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void UseAbilityDirectWithHandSelection(int32 AbilityIndex, const TArray<int32>& HandIndices);

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void ObjectiveClaim();

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void Klick(ARoundGridTile* Field);

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void HoverField(ARoundGridTile* Field);

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void EndHoverField(ARoundGridTile* Field);

    UFUNCTION(BlueprintPure, Category = "Round|Player")
    virtual bool CanControlCharacter(const ARoundCharacter* Character) const;

    UFUNCTION(BlueprintPure, Category = "Round|Player")
    virtual bool CanSeeCharacter(const ARoundCharacter* Character) const;

    UFUNCTION(BlueprintPure, Category = "Round|Player")
    virtual bool CanSeeTile(const ARoundGridTile* Tile) const;

    UFUNCTION(BlueprintPure, Category = "Round|Player")
    ERoundTeam GetEffectiveTeam() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Player")
    virtual void UpdateVision();

    UFUNCTION(BlueprintCallable, Category = "Round|Debug") void ToggleDebugMode();
    UFUNCTION(BlueprintCallable, Category = "Round|Debug") void SetDebugRevealMap(bool bEnabled);
    UFUNCTION(BlueprintCallable, Category = "Round|Debug") void SetDebugTerrainLabels(bool bEnabled);
    UFUNCTION(BlueprintPure, Category = "Round|Debug") ARoundCharacter* GetDebugSelectedCharacter() const;
    UFUNCTION(BlueprintCallable, Category = "Round|Debug") void DebugFullHeal();
    UFUNCTION(BlueprintCallable, Category = "Round|Debug") void DebugApplyDamage();
    UFUNCTION(BlueprintCallable, Category = "Round|Debug") void DebugAdjustInitiative(bool bIncrease);
    UFUNCTION(BlueprintCallable, Category = "Round|Debug") void DebugResetCooldowns();

    UFUNCTION(Client, Reliable)
    void ClientRefreshVision();

    UFUNCTION(Client, Reliable)
    void ClientPlayPresentationEventSound(USoundBase* Sound, float VolumeMultiplier, float PitchMultiplier, float MaxDurationSeconds);

    UFUNCTION(Server, Reliable)
    void ServerSubmitSelectedTeam(const FRoundTeamDefinition& SelectedTeam);

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Round|Player")
    void RefreshUI();

    UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Round|Player")
    void TeamWin();

protected:
    virtual void BeginPlay() override;
    virtual void PawnClientRestart() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION(Client, Reliable)
    void ClientPostActionResult(ARoundGridTile* FieldToSelect, ARoundCharacter* CharacterToKeepSelected);

    UFUNCTION(Server, Reliable)
    void ServerExecuteAction(ARoundCharacter* Character, ARoundGridTile* Field, ERoundActionType Action, int32 AbilityIndex);

    UFUNCTION(Server, Reliable)
    void ServerExecuteMoveAlongPath(ARoundCharacter* Character, const TArray<ARoundGridTile*>& Path);

    UFUNCTION(Server, Reliable)
    void ServerEndTurnCharacter(ARoundCharacter* Character);

    UFUNCTION(Server, Reliable)
    void ServerUseAbilityDirect(ARoundCharacter* Character, int32 AbilityIndex);

    UFUNCTION(Server, Reliable)
    void ServerUseAbilityDirectWithHandSelection(ARoundCharacter* Character, int32 AbilityIndex, const TArray<int32>& HandIndices);

    UFUNCTION(Server, Reliable)
    void ServerStartObjectiveClaim(ARoundCharacter* Character);

    UFUNCTION(Server, Reliable)
    void ServerRequestInitialVisionRefresh();

    UFUNCTION(Server, Reliable) void ServerDebugModifyCharacter(ARoundCharacter* Character, int32 Operation, int32 Amount);

    UFUNCTION()
    void HandleTurnChanged(ARoundCharacter* ActiveCharacter);

    UFUNCTION()
    void HandleBoardStateChanged();

    UFUNCTION()
    void OnRep_Team();

    UFUNCTION()
    void OnRep_TeamActor();

    void BindToGameState(ARoundGameState* RoundGameState);
    bool TrySelectActiveCharacterForLocalTurn(bool bSnapCamera);
    bool CanIssueCommandForCharacter(const ARoundCharacter* Character) const;
    void ScheduleVisionRetry();
    void SubmitSelectedTeam();
    void ConfigureLocalInput();
    void UpdateCameraMovement(float DeltaSeconds);
    void UpdateCameraZoom();
    FVector ClampCameraLocationToBoard(const FVector& DesiredLocation) const;
    void SnapCameraToField(const ARoundGridTile* Field, bool bInstant);
    void RefreshActionPreview();
    void RefreshActionRangePreview(ARoundCharacter* Character);
    void RefreshActionImpactPreview(ARoundCharacter* Character);
    void ClearActionPreview();
    void InvalidateActionRangePreview();
    void ResetPreviewTiles(TArray<TObjectPtr<ARoundGridTile>>& Tiles);
    void ApplyPreviewStateToTile(ARoundGridTile* Tile, ERoundHoverFieldState State) const;
    void BuildMoveRangePreview(ARoundCharacter* Character, TArray<ARoundGridTile*>& OutRangeTiles) const;
    void BuildAttackRangePreview(ARoundCharacter* Character, TArray<ARoundGridTile*>& OutRangeTiles) const;
    void BuildAbilityRangePreview(ARoundCharacter* Character, ARoundAbilityActor* Ability, TArray<ARoundGridTile*>& OutRangeTiles) const;
    void BuildCurrentHoverImpactPreview(ARoundCharacter* Character, TArray<ARoundGridTile*>& OutImpactTiles, ERoundHoverFieldState& OutState) const;

    TWeakObjectPtr<ARoundGameState> BoundGameState;
    FTimerHandle VisionRetryTimerHandle;
    int32 RemainingVisionRetryCount = 0;
    TArray<TObjectPtr<ARoundGridTile>> ActionRangePreviewTiles;
    TArray<TObjectPtr<ARoundGridTile>> ActionImpactPreviewTiles;
    TWeakObjectPtr<ARoundCharacter> CachedRangePreviewCharacter;
    TWeakObjectPtr<ARoundGridTile> CachedRangePreviewSourceTile;
    ERoundActionType CachedRangePreviewAction = ERoundActionType::None;
    int32 CachedRangePreviewAbilityIndex = INDEX_NONE;
    uint32 ActionPreviewBoardRevision = 0;
    uint32 CachedRangePreviewBoardRevision = 0;
    bool bActionRangePreviewDirty = true;
    bool bDebugInputBound = false;
};
