#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundSkillHoverInfoWidget.generated.h"

class UTextBlock;
class UVerticalBox;

UCLASS(Blueprintable)
class ROUND_API URoundSkillHoverInfoWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SetHoverContent(
        const FText& InTitle,
        const FText& InDescription,
        const FText& InDetails,
        const FText& InMeta,
        const FText& InDamageText = FText::GetEmpty(),
        const FText& InCurrentCDText = FText::GetEmpty(),
        const FText& InMaxCDText = FText::GetEmpty());

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
    TObjectPtr<UTextBlock> TextBlock_Meta = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Info_Damage = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Info_CurrentCD = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Info_MaxCD = nullptr;

private:
    void EnsureDefaultLayout();

    FText PendingTitle;
    FText PendingDescription;
    FText PendingDetails;
    FText PendingMeta;
    FText PendingDamageText;
    FText PendingCurrentCDText;
    FText PendingMaxCDText;
};
