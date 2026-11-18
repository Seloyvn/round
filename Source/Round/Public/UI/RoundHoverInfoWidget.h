#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundHoverInfoWidget.generated.h"

class UTextBlock;
class UListView;
class UVerticalBox;
class URoundSkillHoverListItemData;

UCLASS(Blueprintable)
class ROUND_API URoundHoverInfoWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SetHoverContent(const FText& InTitle, const FText& InDescription, const FText& InDetails);

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SetSkillItems(const TArray<URoundSkillHoverListItemData*>& InSkillItems);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> VerticalBox_Root = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Title = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Description = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Details = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UListView> ListView_Skills = nullptr;

private:
    void EnsureDefaultLayout();
    void RefreshSkillItems();

    FText PendingTitle;
    FText PendingDescription;
    FText PendingDetails;

    UPROPERTY(Transient)
    TArray<TObjectPtr<URoundSkillHoverListItemData>> PendingSkillItems;
};
