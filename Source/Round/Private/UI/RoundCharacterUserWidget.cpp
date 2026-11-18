#include "UI/RoundCharacterUserWidget.h"

#include "Components/TextBlock.h"
#include "Input/Reply.h"
#include "UI/RoundSkillMenuWidget.h"

void URoundCharacterUserWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshEntry();
}

void URoundCharacterUserWidget::RefreshEntry()
{
    if (!TextBlock_338)
    {
        return;
    }

    const FString Label = CharacterEntry.Character.IsEmpty()
        ? FString::Printf(TEXT("Charakter %d"), CharacterIndex + 1)
        : CharacterEntry.Character;
    TextBlock_338->SetText(FText::FromString(Label));
}

FReply URoundCharacterUserWidget::NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && SkillMenu)
    {
        SkillMenu->SelectCharacter(CharacterIndex);
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(FGeometry(), InMouseEvent);
}
