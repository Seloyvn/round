#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/RoundGameState.h"
#include "RoundInitiativeBarWidget.generated.h"

class UCanvasPanel;
class UUserWidget;
class ARoundCharacter;
class ARoundPlayerPawn;
class URoundInitiativeIconWidget;
class URoundObjectiveMarkerWidget;

UCLASS(Blueprintable)
class ROUND_API URoundInitiativeBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void AddIcon(UUserWidget* Icon);

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void ClearIcons();

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SyncIcons(const TArray<ARoundCharacter*>& Characters, TSubclassOf<URoundInitiativeIconWidget> IconClass, APlayerController* OwningController);

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SyncObjectiveMarkers(const TArray<FRoundObjectivePreviewEntry>& Entries, int32 CurrentGlobalInitiative);

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> CanvasPanel_34 = nullptr;

private:
    UPROPERTY(Transient)
    TMap<TObjectPtr<ARoundCharacter>, TObjectPtr<URoundInitiativeIconWidget>> CharacterIcons;

    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<URoundObjectiveMarkerWidget>> ObjectiveMarkers;
};
