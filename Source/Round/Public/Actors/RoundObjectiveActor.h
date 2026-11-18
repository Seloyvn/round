#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoundTypes.h"
#include "RoundObjectiveActor.generated.h"

class UTextRenderComponent;
class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UStaticMesh;
class UWidgetComponent;
class URoundObjectiveRewardData;
class ARoundObjectiveRewardActor;

UCLASS(Blueprintable)
class ROUND_API ARoundObjectiveActor : public AActor
{
    GENERATED_BODY()

public:
    ARoundObjectiveActor();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PostNetReceive() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
    TObjectPtr<USceneComponent> SceneRoot = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
    TObjectPtr<UTextRenderComponent> TextRender = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
    TObjectPtr<UStaticMeshComponent> MarkerMesh = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
    TObjectPtr<UPointLightComponent> MarkerLight = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
    TObjectPtr<UStaticMeshComponent> ProgressBarBackground = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
    TObjectPtr<UStaticMeshComponent> ProgressBarFill = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
    TObjectPtr<UWidgetComponent> ProgressWidgetComponent = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    FString ObjectiveId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    FName RewardId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    ERoundObjectiveType ObjectiveType = ERoundObjectiveType::HoldZone;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    ERoundObjectiveState ObjectiveState = ERoundObjectiveState::Preview;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    TObjectPtr<class ARoundGridTile> CenterTile = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    TArray<TObjectPtr<class ARoundGridTile>> ObjectiveTiles;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
    TObjectPtr<URoundObjectiveRewardData> RewardData = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    FString RewardDisplayName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    FString RewardDescription;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    ERoundObjectiveRewardType RewardType = ERoundObjectiveRewardType::Heal;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    double RewardValue = 0.0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    int32 RewardDurationInitiative = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    bool bRewardPermanent = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    int32 SpawnAtGlobalInitiative = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    int32 ExpireAtGlobalInitiative = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    int32 RequiredProgressInitiative = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    int32 CurrentProgressInitiative = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    ERoundTeam CapturingTeam = ERoundTeam::Neutral;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    int32 MaxHealth = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    int32 CurrentHealth = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    TObjectPtr<class ARoundCharacter> ChannelingCharacter = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    int32 ChannelRemainingInitiative = 0;

    UFUNCTION(BlueprintCallable, Category = "Objective")
    void InitializeObjective(
        ERoundObjectiveType InType,
        class ARoundGridTile* InCenterTile,
        const TArray<class ARoundGridTile*>& InObjectiveTiles,
        URoundObjectiveRewardData* InRewardData,
        int32 InSpawnAtGlobalInitiative,
        int32 InExpireAtGlobalInitiative,
        int32 InRequiredProgressInitiative,
        int32 InMaxHealth);

    UFUNCTION(BlueprintCallable, Category = "Objective")
    bool AdvanceObjectiveTime(class ARoundGameState* GameState, int32 CurrentGlobalInitiative, int32 InitiativeAmount);

    UFUNCTION(BlueprintPure, Category = "Objective")
    bool IsPreviewing() const { return ObjectiveState == ERoundObjectiveState::Preview; }

    UFUNCTION(BlueprintPure, Category = "Objective")
    bool IsActiveObjective() const { return ObjectiveState == ERoundObjectiveState::Active; }

private:
    void RefreshVisuals();
    void RefreshProgressBarVisuals();
    void RefreshProgressWidget();
    bool DropReward(class ARoundGameState* GameState);
    void CancelChannel();
    void ApplyRewardData(class URoundObjectiveRewardData* InRewardData);

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> ProgressBarMesh = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API ARoundHoldObjectiveActor : public ARoundObjectiveActor
{
    GENERATED_BODY()
};

UCLASS(Blueprintable)
class ROUND_API ARoundDestroyObjectiveActor : public ARoundObjectiveActor
{
    GENERATED_BODY()
};

UCLASS(Blueprintable)
class ROUND_API ARoundChannelObjectiveActor : public ARoundObjectiveActor
{
    GENERATED_BODY()
};
