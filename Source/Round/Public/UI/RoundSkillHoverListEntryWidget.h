#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "RoundSkillHoverListEntryWidget.generated.h"

class UTextBlock;
class URoundSkillHoverInfoWidget;
class STextBlock;

UCLASS(BlueprintType)
class ROUND_API URoundSkillHoverListItemData : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI")
    FText DetailsText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI")
    FText MetaText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI")
    FText DamageText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI")
    FText CurrentCooldownText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI")
    FText MaxCooldownText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI")
    TSubclassOf<URoundSkillHoverInfoWidget> HoverWidgetClass;

};

UCLASS(Blueprintable)
class ROUND_API URoundSkillHoverListEntryWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_SkillName = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Label = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_40 = nullptr;

private:
    void EnsureLabelWidget();
    void RefreshFromItem();

    UPROPERTY(Transient)
    TObjectPtr<URoundSkillHoverListItemData> ItemData = nullptr;

    TSharedPtr<STextBlock> SlateLabel;
};
