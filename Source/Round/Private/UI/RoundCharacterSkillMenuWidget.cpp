#include "UI/RoundCharacterSkillMenuWidget.h"

#include "UI/RoundMainMenuWidget.h"

FRoundCharacterEntry URoundCharacterSkillMenuWidget::Character() const
{
    FRoundCharacterEntry Result;
    if (MainMenu)
    {
        MainMenu->TryGetEditedCharacter(MainMenu->GetCurrentSelectedCharacterIndex(), Result);
    }
    return Result;
}

void URoundCharacterSkillMenuWidget::SetSelectedCharacter(const FRoundCharacterEntry& CharacterEntry)
{
    if (!MainMenu)
    {
        return;
    }

    MainMenu->UpdateEditedCharacter(MainMenu->GetCurrentSelectedCharacterIndex(), CharacterEntry, true);
    BP_OnCharacterSkillMenuRefreshed();
}

void URoundCharacterSkillMenuWidget::SetPosition(const FVector2D InPosition)
{
    Position = InPosition;
}

void URoundCharacterSkillMenuWidget::LoadSkillpoints()
{
    BP_OnCharacterSkillMenuRefreshed();
}
