#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundGameUIWidget.generated.h"

class ARoundAbilityActor;
class ARoundCharacter;
class ARoundGameState;
class ARoundPlayerPawn;
class ARoundGridTile;
class ARoundObjectiveActor;
class ARoundObjectiveRewardActor;
class UButton;
class UListView;
class UOverlay;
class UPanelWidget;
class UProgressBar;
class UTextBlock;
class UUserWidget;
class UVerticalBox;
class URoundChampionCustomUIWidget;
class URoundInitiativeBarWidget;
class URoundAuraListItemData;
class URoundSkillHoverListItemData;

UCLASS(Blueprintable)
class ROUND_API URoundGameUIWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<ARoundPlayerPawn> Player = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SetPlayerPawn(ARoundPlayerPawn* InPlayer);

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void RefreshFromPlayer();

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void RefreshCharacterUI();

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void RefreshButtons();

    UFUNCTION(BlueprintPure, Category = "Round|UI")
    virtual ARoundCharacter* GetDisplayedCharacter() const;

    UFUNCTION(BlueprintPure, Category = "Round|UI")
    virtual ARoundAbilityActor* GetDisplayedAbility(int32 AbilityIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void OnMoveClicked();

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void OnAttackClicked();

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void OnEndTurnClicked();

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void OnAbilityClicked(int32 AbilityIndex);

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void HasWon();

    void SetHoverInfoOverride(ARoundCharacter* SourceCharacter, const FText& MainText, const FText& SubText);
    void ClearHoverInfoOverride(ARoundCharacter* SourceCharacter);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Move = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Attack = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_EndTurn = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Interact = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Ability1 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Ability2 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Ability3 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Ability4 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Ability5 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Ability6 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Ability7 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_234 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_174 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Interact = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Ability1 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Ability2 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Ability3 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Ability4 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Ability5 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Ability6 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Ability7 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Info = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Info_1 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Info_Objective = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Info_Field = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Health = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Initative = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> Health_Bar = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> InitativeBar = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundInitiativeBarWidget> BP_InitativeBar = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UListView> ListView_0 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UListView> ListView_Skills = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UListView> ListView_Auras = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round|UI")
    TSubclassOf<UUserWidget> SkillListEntryWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round|UI")
    TSubclassOf<UUserWidget> AuraListEntryWidgetClass;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> CustomUIPanel = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> VerticalBox_Actions = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> VerticalBox_Buttons = nullptr;

    UFUNCTION(BlueprintImplementableEvent, Category = "Round|UI")
    void BP_OnUIRefreshed();

private:
    void EnsureInteractWidgets();
    void RefreshInitiativeBar();
    void RefreshEffects();
    void RefreshAuras();
    void RefreshSkills();
    void RefreshCustomUI();
    void RefreshInfoPanel();
    int32 GetDisplayedAbilityDamage(const ARoundAbilityActor* Ability) const;
    void UpdateAbilityButton(int32 AbilityIndex);
    bool IsObjectiveClaimButtonIndex(int32 AbilityIndex) const;
    int32 GetObjectiveClaimButtonIndex() const;
    FText BuildObjectiveInfoText() const;
    FText BuildObjectiveInfoTooltipText() const;
    FText BuildFieldInfoText() const;
    FText BuildFieldInfoTooltipText() const;
    void UpdateActionButton(const TCHAR* ButtonName, const TCHAR* LabelName, const bool bEnabled, const FText& LabelText);
    void UpdateProgressAndText(const TCHAR* ProgressBarName, const TCHAR* TextName, int32 CurrentValue, int32 MaxValue) const;
    void BindToGameState(ARoundGameState* RoundGameState);
    void BindButtons();
    UButton* GetAbilityButtonByIndex(int32 AbilityIndex) const;
    UTextBlock* GetAbilityLabelByIndex(int32 AbilityIndex) const;
    UButton* ResolveButton(const TCHAR* ButtonName) const;
    UTextBlock* ResolveTextBlock(const TCHAR* TextName) const;
    UProgressBar* ResolveProgressBar(const TCHAR* ProgressBarName) const;

    UFUNCTION()
    void HandleBoardStateChanged();

    UFUNCTION()
    void HandleTurnChanged(ARoundCharacter* ActiveCharacter);

    UFUNCTION()
    void HandleAbility1Clicked();

    UFUNCTION()
    void HandleAbility2Clicked();

    UFUNCTION()
    void HandleAbility3Clicked();

    UFUNCTION()
    void HandleAbility4Clicked();

    UFUNCTION()
    void HandleAbility5Clicked();

    UFUNCTION()
    void HandleAbility6Clicked();

    UFUNCTION()
    void HandleAbility7Clicked();

    UFUNCTION()
    void OnInteractClicked();

    TWeakObjectPtr<ARoundGameState> BoundGameState;
    bool bButtonsBound = false;
    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> CurrentCustomUIWidget = nullptr;
    TWeakObjectPtr<ARoundCharacter> CustomUICharacter;
    TWeakObjectPtr<ARoundCharacter> HoverInfoCharacter;
    FText HoverInfoMainText;
    FText HoverInfoSubText;
    bool bHasHoverInfoOverride = false;
    UPROPERTY(Transient)
    TArray<TObjectPtr<URoundSkillHoverListItemData>> SkillListItems;
    UPROPERTY(Transient)
    TArray<TObjectPtr<URoundAuraListItemData>> AuraListItems;
};
