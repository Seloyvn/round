#include "UI/RoundHoverInfoWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/RoundSkillHoverListEntryWidget.h"

namespace
{
void AssignHoverListEntryClass(UListView* ListView, UClass* EntryClass)
{
    if (!ListView || !EntryClass)
    {
        return;
    }

    if (ListView->GetEntryWidgetClass() == EntryClass)
    {
        return;
    }

    if (FClassProperty* EntryClassProperty = FindFProperty<FClassProperty>(UListViewBase::StaticClass(), TEXT("EntryWidgetClass")))
    {
        EntryClassProperty->SetPropertyValue_InContainer(ListView, EntryClass);
    }
}
}

void URoundHoverInfoWidget::SetHoverContent(const FText& InTitle, const FText& InDescription, const FText& InDetails)
{
    PendingTitle = InTitle;
    PendingDescription = InDescription;
    PendingDetails = InDetails;

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
    }

    RefreshSkillItems();
}

void URoundHoverInfoWidget::SetSkillItems(const TArray<URoundSkillHoverListItemData*>& InSkillItems)
{
    PendingSkillItems.Reset();
    for (URoundSkillHoverListItemData* Item : InSkillItems)
    {
        if (Item)
        {
            PendingSkillItems.Add(Item);
        }
    }

    EnsureDefaultLayout();
    RefreshSkillItems();
}

void URoundHoverInfoWidget::NativeConstruct()
{
    Super::NativeConstruct();
    EnsureDefaultLayout();
    SetHoverContent(PendingTitle, PendingDescription, PendingDetails);
    RefreshSkillItems();
}

void URoundHoverInfoWidget::EnsureDefaultLayout()
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
    if (!ListView_Skills)
    {
        ListView_Skills = Cast<UListView>(WidgetTree->FindWidget(TEXT("ListView_Skills")));
    }

    const bool bHasBlueprintProvidedLayout =
        WidgetTree->RootWidget != nullptr
        && (VerticalBox_Root != nullptr || TextBlock_Title != nullptr || TextBlock_Description != nullptr || TextBlock_Details != nullptr);

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
        if (Target && VerticalBox_Root)
        {
            VerticalBox_Root->AddChildToVerticalBox(Target);
        }
        return Target;
    };

    EnsureTextBlock(TextBlock_Title, TEXT("TextBlock_Title"));
    EnsureTextBlock(TextBlock_Description, TEXT("TextBlock_Description"));
    EnsureTextBlock(TextBlock_Details, TEXT("TextBlock_Details"));

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
        ConfigureSlotPadding(TextBlock_Details, FMargin(0.f));
    }
}

void URoundHoverInfoWidget::RefreshSkillItems()
{
    if (!ListView_Skills)
    {
        return;
    }

    AssignHoverListEntryClass(ListView_Skills, URoundSkillHoverListEntryWidget::StaticClass());

    if (PendingSkillItems.Num() == 0)
    {
        ListView_Skills->ClearListItems();
        ListView_Skills->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    TArray<UObject*> Items;
    Items.Reserve(PendingSkillItems.Num());
    for (URoundSkillHoverListItemData* Item : PendingSkillItems)
    {
        if (Item)
        {
            Items.Add(Item);
        }
    }

    ListView_Skills->SetVisibility(ESlateVisibility::Visible);
    ListView_Skills->SetListItems(Items);
}
