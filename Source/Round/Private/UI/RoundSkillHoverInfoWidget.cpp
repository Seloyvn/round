#include "UI/RoundSkillHoverInfoWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void URoundSkillHoverInfoWidget::SetHoverContent(
    const FText& InTitle,
    const FText& InDescription,
    const FText& InDetails,
    const FText& InMeta,
    const FText& InDamageText,
    const FText& InCurrentCDText,
    const FText& InMaxCDText)
{
    PendingTitle = InTitle;
    PendingDescription = InDescription;
    PendingDetails = InDetails;
    PendingMeta = InMeta;
    PendingDamageText = InDamageText;
    PendingCurrentCDText = InCurrentCDText;
    PendingMaxCDText = InMaxCDText;

    EnsureDefaultLayout();

    if (TextBlock_Title)
    {
        TextBlock_Title->SetText(PendingTitle);
    }

    if (TextBlock_Description)
    {
        TextBlock_Description->SetText(PendingDescription);
    }

    if (TextBlock_Details)
    {
        TextBlock_Details->SetText(PendingDetails);
        TextBlock_Details->SetVisibility(PendingDetails.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
    }

    if (TextBlock_Meta)
    {
        TextBlock_Meta->SetText(PendingMeta);
        TextBlock_Meta->SetVisibility(PendingMeta.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
    }

    if (TextBlock_Info_Damage)
    {
        TextBlock_Info_Damage->SetText(PendingDamageText);
        TextBlock_Info_Damage->SetVisibility(PendingDamageText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
    }

    if (TextBlock_Info_CurrentCD)
    {
        TextBlock_Info_CurrentCD->SetText(PendingCurrentCDText);
        TextBlock_Info_CurrentCD->SetVisibility(PendingCurrentCDText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
    }

    if (TextBlock_Info_MaxCD)
    {
        TextBlock_Info_MaxCD->SetText(PendingMaxCDText);
        TextBlock_Info_MaxCD->SetVisibility(PendingMaxCDText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
    }
}

void URoundSkillHoverInfoWidget::NativeConstruct()
{
    Super::NativeConstruct();
    EnsureDefaultLayout();
    SetHoverContent(PendingTitle, PendingDescription, PendingDetails, PendingMeta, PendingDamageText, PendingCurrentCDText, PendingMaxCDText);
}

void URoundSkillHoverInfoWidget::EnsureDefaultLayout()
{
    if (!WidgetTree)
    {
        return;
    }

    if (!VerticalBox_Root)
    {
        VerticalBox_Root = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("VerticalBox_Root")));
    }
    if (!TextBlock_Title)
    {
        TextBlock_Title = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TextBlock_Title")));
    }
    if (!TextBlock_Description)
    {
        TextBlock_Description = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TextBlock_Description")));
    }
    if (!TextBlock_Details)
    {
        TextBlock_Details = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TextBlock_Details")));
    }
    if (!TextBlock_Meta)
    {
        TextBlock_Meta = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TextBlock_Meta")));
    }
    if (!TextBlock_Info_Damage)
    {
        TextBlock_Info_Damage = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TextBlock_Info_Damage")));
    }
    if (!TextBlock_Info_CurrentCD)
    {
        TextBlock_Info_CurrentCD = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TextBlock_Info_CurrentCD")));
    }
    if (!TextBlock_Info_MaxCD)
    {
        TextBlock_Info_MaxCD = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("TextBlock_Info_MaxCD")));
    }

    const bool bHasBlueprintProvidedLayout =
        WidgetTree->RootWidget != nullptr
        && (VerticalBox_Root != nullptr || TextBlock_Title != nullptr || TextBlock_Description != nullptr || TextBlock_Details != nullptr || TextBlock_Meta != nullptr || TextBlock_Info_Damage != nullptr || TextBlock_Info_CurrentCD != nullptr || TextBlock_Info_MaxCD != nullptr);

    if (!VerticalBox_Root && !bHasBlueprintProvidedLayout)
    {
        VerticalBox_Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VerticalBox_Root"));
        WidgetTree->RootWidget = VerticalBox_Root;
    }

    auto EnsureTextBlock = [this](TObjectPtr<UTextBlock>& Target, const TCHAR* Name) -> UTextBlock*
    {
        if (Target)
        {
            return Target;
        }

        Target = Cast<UTextBlock>(WidgetTree->FindWidget(FName(Name)));
        if (Target)
        {
            return Target;
        }

        if (!VerticalBox_Root)
        {
            return nullptr;
        }

        Target = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(Name));
        if (Target)
        {
            VerticalBox_Root->AddChildToVerticalBox(Target);
        }
        return Target;
    };

    EnsureTextBlock(TextBlock_Title, TEXT("TextBlock_Title"));
    EnsureTextBlock(TextBlock_Description, TEXT("TextBlock_Description"));
    EnsureTextBlock(TextBlock_Details, TEXT("TextBlock_Details"));
    EnsureTextBlock(TextBlock_Meta, TEXT("TextBlock_Meta"));
    EnsureTextBlock(TextBlock_Info_Damage, TEXT("TextBlock_Info_Damage"));
    EnsureTextBlock(TextBlock_Info_CurrentCD, TEXT("TextBlock_Info_CurrentCD"));
    EnsureTextBlock(TextBlock_Info_MaxCD, TEXT("TextBlock_Info_MaxCD"));

    auto ConfigureSlotPadding = [](UTextBlock* TextBlock, const FMargin& SlotPadding)
    {
        if (!TextBlock)
        {
            return;
        }

        if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(TextBlock->Slot))
        {
            Slot->SetPadding(SlotPadding);
        }
    };

    if (VerticalBox_Root)
    {
        ConfigureSlotPadding(TextBlock_Title, FMargin(0.f, 0.f, 0.f, 4.f));
        ConfigureSlotPadding(TextBlock_Description, FMargin(0.f, 0.f, 0.f, 6.f));
        ConfigureSlotPadding(TextBlock_Details, FMargin(0.f, 0.f, 0.f, 6.f));
        ConfigureSlotPadding(TextBlock_Meta, FMargin(0.f, 0.f, 0.f, 6.f));
        ConfigureSlotPadding(TextBlock_Info_Damage, FMargin(0.f, 0.f, 0.f, 4.f));
        ConfigureSlotPadding(TextBlock_Info_CurrentCD, FMargin(0.f, 0.f, 0.f, 4.f));
        ConfigureSlotPadding(TextBlock_Info_MaxCD, FMargin(0.f));
    }
}
