#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "RoundAuraEntryWidget.generated.h"

class UImage;
class UTextBlock;
class URoundAuraListItemData;
class STextBlock;

UCLASS(Blueprintable)
class ROUND_API URoundAuraEntryWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SetAuraItem(URoundAuraListItemData* InItem);
    virtual void NativeConstruct() override;
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TObjectPtr<URoundAuraListItemData> AuraItem = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_49 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_203 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;

private:
    void EnsureLabelWidget();
    void RefreshFromItem();
    FText BuildLabelText() const;
    FText BuildTooltipText() const;

    TSharedPtr<STextBlock> SlateLabel;
};
