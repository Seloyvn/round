#include "UI/RoundAuraEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "UI/RoundAuraListItemData.h"
#include "Widgets/Text/STextBlock.h"

void URoundAuraEntryWidget::SetAuraItem(URoundAuraListItemData* InItem)
{
    AuraItem = InItem;
    RefreshFromItem();
}

void URoundAuraEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    EnsureLabelWidget();
    RefreshFromItem();
}

TSharedRef<SWidget> URoundAuraEntryWidget::RebuildWidget()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return Super::RebuildWidget();
    }

    SlateLabel.Reset();
    return SAssignNew(SlateLabel, STextBlock)
        .Text(FText::GetEmpty());
}

void URoundAuraEntryWidget::ReleaseSlateResources(const bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    SlateLabel.Reset();
}

void URoundAuraEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
    AuraItem = Cast<URoundAuraListItemData>(ListItemObject);
    RefreshFromItem();
}

void URoundAuraEntryWidget::EnsureLabelWidget()
{
    if (TextBlock_203 || TextBlock)
    {
        return;
    }

    if (!WidgetTree)
    {
        return;
    }

    UTextBlock* CreatedLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock"));
    if (!CreatedLabel)
    {
        return;
    }

    WidgetTree->RootWidget = CreatedLabel;
    TextBlock = CreatedLabel;
}

void URoundAuraEntryWidget::RefreshFromItem()
{
    UTextBlock* LabelBlock = TextBlock_203 ? TextBlock_203.Get() : TextBlock.Get();
    if (!LabelBlock && !SlateLabel.IsValid())
    {
        return;
    }

    if (!AuraItem)
    {
        if (LabelBlock)
        {
            LabelBlock->SetText(FText::GetEmpty());
        }
        if (SlateLabel.IsValid())
        {
            SlateLabel->SetText(FText::GetEmpty());
        }
        SetToolTip(nullptr);
        return;
    }

    if (LabelBlock)
    {
        LabelBlock->SetText(BuildLabelText());
    }
    if (SlateLabel.IsValid())
    {
        SlateLabel->SetText(BuildLabelText());
    }
    SetToolTipText(BuildTooltipText());
}

FText URoundAuraEntryWidget::BuildLabelText() const
{
    if (!AuraItem)
    {
        return FText::GetEmpty();
    }

    const FString Label = !AuraItem->DisplayName.IsEmpty()
        ? AuraItem->DisplayName.ToString()
        : TEXT("Aura");
    return FText::FromString(Label);
}

FText URoundAuraEntryWidget::BuildTooltipText() const
{
    if (!AuraItem)
    {
        return FText::GetEmpty();
    }

    if (!AuraItem->Description.IsEmpty())
    {
        return AuraItem->Description;
    }

    return AuraItem->DisplayName;
}
