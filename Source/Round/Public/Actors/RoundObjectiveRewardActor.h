#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundFieldTriggerActor.h"
#include "RoundTypes.h"
#include "RoundObjectiveRewardActor.generated.h"

class UTextRenderComponent;
class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class URoundObjectiveRewardData;

UCLASS(Blueprintable)
class ROUND_API ARoundObjectiveRewardActor : public ARoundFieldTriggerActor
{
    GENERATED_BODY()

public:
    ARoundObjectiveRewardActor();
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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    TObjectPtr<class ARoundGridTile> RewardTile = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objective")
    TObjectPtr<URoundObjectiveRewardData> RewardData = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Objective")
    FName RewardId = NAME_None;

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

    UFUNCTION(BlueprintCallable, Category = "Objective")
    void InitializeReward(class ARoundGridTile* InTile, URoundObjectiveRewardData* InRewardData);

protected:
    virtual bool CanTriggerCharacter_Implementation(class ARoundCharacter* EnteringCharacter, class ARoundGridTile* EnteredTile, class ARoundGridTile* PreviousTile) const override;
    virtual void ExecuteTrigger_Implementation(class ARoundCharacter* EnteringCharacter, class ARoundGridTile* EnteredTile, class ARoundGridTile* PreviousTile) override;

private:
    void RefreshVisuals();
    bool GrantReward(class ARoundCharacter* Character);
};
