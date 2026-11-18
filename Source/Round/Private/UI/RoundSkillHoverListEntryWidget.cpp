#include "UI/RoundSkillHoverListEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "UI/RoundSkillHoverInfoWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
}

void URoundSkillHoverListEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    EnsureLabelWidget();
    RefreshFromItem();
}

TSharedRef<SWidget> URoundSkillHoverListEntryWidget::RebuildWidget()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return Super::RebuildWidget();
    }

    SlateLabel.Reset();
    return SAssignNew(SlateLabel, STextBlock)
        .Text(FText::GetEmpty());
}

void URoundSkillHoverListEntryWidget::ReleaseSlateResources(const bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    SlateLabel.Reset();
}

void URoundSkillHoverListEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
    ItemData = Cast<URoundSkillHoverListItemData>(ListItemObject);
    RefreshFromItem();
}

void URoundSkillHoverListEntryWidget::EnsureLabelWidget()
{
    if (TextBlock_SkillName || TextBlock_Label || TextBlock_40)
    {
        return;
    }

    if (!WidgetTree)
    {
        return;
    }

    UTextBlock* CreatedLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_Label"));
    if (!CreatedLabel)
    {
        return;
    }

    WidgetTree->RootWidget = CreatedLabel;
    TextBlock_Label = CreatedLabel;
}

void URoundSkillHoverListEntryWidget::RefreshFromItem()
{
    UTextBlock* Label = TextBlock_SkillName ? TextBlock_SkillName : (TextBlock_Label ? TextBlock_Label : TextBlock_40);
    if (!Label && !SlateLabel.IsValid())
    {
        return;
    }

    if (!ItemData)
    {
        if (Label)
        {
            Label->SetText(FText::GetEmpty());
        }
        if (SlateLabel.IsValid())
        {
            SlateLabel->SetText(FText::GetEmpty());
        }
        SetToolTip(nullptr);
        return;
    }

    if (Label)
    {
        Label->SetText(ItemData->DisplayName);
    }
    if (SlateLabel.IsValid())
    {
        SlateLabel->SetText(ItemData->DisplayName);
    }

    TSubclassOf<URoundSkillHoverInfoWidget> TooltipClass = URoundSkillHoverInfoWidget::StaticClass();
    if (ItemData->HoverWidgetClass)
    {
        TooltipClass = ItemData->HoverWidgetClass;
    }
    if (TooltipClass)
    {
        URoundSkillHoverInfoWidget* TooltipWidget = CreateWidget<URoundSkillHoverInfoWidget>(this, TooltipClass);
        if (TooltipWidget)
        {
            TooltipWidget->SetHoverContent(
                ItemData->DisplayName,
                ItemData->Description,
                ItemData->DetailsText,
                ItemData->MetaText,
                ItemData->DamageText,
                ItemData->CurrentCooldownText,
                ItemData->MaxCooldownText);
            SetToolTip(TooltipWidget);
            return;
        }
    }

    SetToolTipText(FText::Format(FText::FromString(TEXT("{0}\n{1}\n{2}")), ItemData->Description, ItemData->DetailsText, ItemData->MetaText));
}
