#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundTypes.h"
#include "RoundSkillpointMenuWidget.generated.h"

class URoundSkillMenuWidget;
class URoundSkillpointWidget;
class UButton;
class UCanvasPanel;
class UImage;
class UTextBlock;

UCLASS(Blueprintable)
class ROUND_API URoundSkillpointMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    URoundSkillpointMenuWidget(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<URoundSkillMenuWidget> SkillMenu = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu", meta = (ExposeOnSpawn = "true"))
    FString SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu", meta = (ExposeOnSpawn = "true"))
    FRoundSkillDetails SkillDetail;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    virtual void ReloadPoints();

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    virtual bool LearnSkillpoint(int32 Point, int32 Value = 1);

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    virtual bool UnlearnSkillpoint(int32 Point, int32 Value = 1);

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    virtual int32 CurrentSkillPoints() const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    virtual int64 CurrentSkillMask() const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    virtual bool CurrentSkillLearned() const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    virtual int32 GetTierSelection(int32 TierIndex) const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    virtual bool IsTierUnlocked(int32 TierIndex) const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    virtual bool IsTierChoiceSelected(int32 TierIndex, int32 ChoiceIndex) const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|SkillMenu")
    virtual bool CanSelectTierChoice(int32 TierIndex, int32 ChoiceIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    virtual bool SelectTierChoice(int32 TierIndex, int32 ChoiceIndex);

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    virtual bool ClearTierChoice(int32 TierIndex);

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    virtual bool LearnSkill();

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    virtual bool UnlearnSkill();

protected:
    virtual void NativeConstruct() override;
    bool ResolveSkillpointLevels(TArray<FRoundSkillpointDetails>& OutLevels, FText& OutDisplayName) const;
    TArray<URoundSkillpointWidget*> GetSkillpointWidgets() const;
    void EnsureDynamicSkillpointWidgets(int32 NeededPoints);
    void EnsureLearnButton();

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> CanvasPanel_Skills1 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_0 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointWidget> Skillpoint = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointWidget> Skillpoint_1 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointWidget> Skillpoint_2 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointWidget> Skillpoint_3 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointWidget> Skillpoint_4 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointWidget> Skillpoint_5 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointWidget> Skillpoint_6 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointWidget> Skillpoint_7 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointWidget> Skillpoint_8 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointWidget> Skillpoint_9 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointWidget> Skillpoint_10 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundSkillpointWidget> Skillpoint_11 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_0 = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu")
    TSubclassOf<URoundSkillpointWidget> SkillpointWidgetClass;

    UPROPERTY(Transient)
    TObjectPtr<UButton> DynamicLearnButton = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DynamicLearnButtonLabel = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<URoundSkillpointWidget>> DynamicSkillpointWidgets;

private:
    UFUNCTION()
    void HandleLearnButtonClicked();

    UFUNCTION()
    FEventReply HandleLearnImageMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

    static int32 GetPackedTierSelection(int64 PackedValue, int32 TierIndex);
    static int64 SetPackedTierSelection(int64 PackedValue, int32 TierIndex, int32 SelectionValue);
    static int32 CountLearnedTiers(int64 PackedValue);

    bool TryGetCharacterAndSkill(FRoundCharacterEntry& OutCharacter, int32& OutCharacterIndex, FRoundSkillEntry*& OutSkill);
};
