#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundTypes.h"
#include "RoundSkillMenuWidget.generated.h"

class UButton;
class UCanvasPanel;
class UComboBoxKey;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
class URoundCharacterUserWidget;
class URoundCharacterSkillpointMenuWidget;
class URoundMainMenuWidget;
class URoundSkillmenuSelectorPointWidget;
class URoundSkillpointMenuWidget;
class URoundStartPositionWidget;
class UWidget;

UCLASS(Blueprintable)
class ROUND_API URoundSkillMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    URoundSkillMenuWidget(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<URoundMainMenuWidget> MainMenu = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    void SetMainMenuWidget(URoundMainMenuWidget* InMainMenu);

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    void SelectCharacter(int32 CharacterIndex);

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    void SetSkillpointMenu(UWidget* Content);

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    void LoadSkillpoints();

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    void RefreshAfterSkillpointChange();

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    bool TryGetSelectedCharacter(FRoundCharacterEntry& OutCharacter) const;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    bool CommitSelectedCharacter(const FRoundCharacterEntry& InCharacter, bool bRefreshSelection = true);

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    int32 GetSelectedCharacterIndex() const;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    void SetSelectedSkillIndex(int32 InSelectedSkillIndex);

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    int32 GetSelectedSkillIndex() const { return SelectedSkillIndex; }

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    int32 GetRemainingTeamSkillPoints() const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    int32 GetCurrentCharacterSkillPoints() const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    int32 GetCurrentCharacterLearnedSkillCount() const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    int32 GetCurrentCharacterSkillLimit() const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    bool CanLearnAdditionalSkill() const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    class URoundAbilityData* FindAbilityDataBySkillName(const FString& InSkillName) const;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    bool TryGetSelectedCharacterData(class URoundCharacterData*& OutCharacterData) const;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    void ShowSkill(const FString& InSkillName, const FRoundSkillDetails& InSkillDetail);

    UFUNCTION(BlueprintImplementableEvent, Category = "Round|UI|SkillMenu")
    void BP_OnSkillMenuRefreshed();

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_0 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> MenuHolder = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UComboBoxKey> ComboBoxKey_0 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> SkillpointCounter = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> SkillmenuSelector = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartPositionWidget> UI_StartPosition_0 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointMenuWidget> SkillpointMenu = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointMenuWidget> SkillpointMenu_0 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_SkillpointsTab = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_StartPositionTab = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidgetSwitcher> ContentSwitcher = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu")
    TSubclassOf<URoundCharacterUserWidget> CharacterUserWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu")
    TSubclassOf<URoundSkillpointMenuWidget> SkillpointMenuClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu")
    TSubclassOf<URoundSkillmenuSelectorPointWidget> SkillSelectorPointClass;

private:
    UFUNCTION()
    void HandleCharacterClassChanged(FName SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void HandleBackClicked();

    UFUNCTION()
    void HandleSkillpointsTabClicked();

    UFUNCTION()
    void HandleStartPositionTabClicked();

    UFUNCTION()
    UWidget* HandleGenerateCharacterClassWidget(FName Item);

    void RefreshSkillSelector();
    void RefreshCharacterClassCombo();
    void RefreshSkillpointCounter();
    void RefreshSkillpointMenu();
    TArray<class URoundAbilityData*> GetAvailableAbilities() const;
    URoundSkillpointMenuWidget* GetSharedSkillpointMenu() const;
    static FName NormalizeCharacterId(const FString& InCharacterId);
    static FSoftObjectPath BuildCharacterDataPath(const FName CharacterId);

    TObjectPtr<UWidget> CurrentContentWidget = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<URoundCharacterSkillpointMenuWidget> CharacterSkillpointMenuWidget = nullptr;

    int32 SelectedSkillIndex = 0;
};
