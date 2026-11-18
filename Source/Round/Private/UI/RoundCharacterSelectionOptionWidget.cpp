#include "UI/RoundCharacterSelectionOptionWidget.h"

#include "Components/TextBlock.h"
#include "Data/RoundCharacterData.h"
#include "Data/RoundCharacterRegistryLookup.h"
#include "UI/RoundMainMenuWidget.h"

void URoundCharacterSelectionOptionWidget::SetChampion(FName InChampion)
{
    Champion = InChampion;
    RefreshLabel();
}

void URoundCharacterSelectionOptionWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshLabel();
}

FReply URoundCharacterSelectionOptionWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (MainMenu && SlotIndex != INDEX_NONE)
    {
        MainMenu->SetSelectedCharacterIndex(SlotIndex);
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void URoundCharacterSelectionOptionWidget::RefreshLabel()
{
    if (!TextBlock_66)
    {
        return;
    }

    FText Label = FText::FromName(Champion);
    if (Champion != NAME_None)
    {
        if (const URoundCharacterData* CharacterData = RoundCharacterRegistry::FindCharacterDataById(Champion))
        {
            if (!CharacterData->DisplayName.IsEmpty())
            {
                Label = CharacterData->DisplayName;
            }
        }
    }

    TextBlock_66->SetText(Label);
}
