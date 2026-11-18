#include "UI/RoundHealthbarWidget.h"

#include "Actors/RoundCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundCharacterData.h"

void URoundHealthbarWidget::SetCharacter(ARoundCharacter* InCharacter)
{
    if (Character && Character->Attributes)
    {
        Character->Attributes->OnHealthChanged.RemoveDynamic(this, &URoundHealthbarWidget::HandleHealthChanged);
    }

    Character = InCharacter;
    if (Character && Character->Attributes)
    {
        Character->Attributes->OnHealthChanged.AddDynamic(this, &URoundHealthbarWidget::HandleHealthChanged);
    }

    UpdateName();
    UpdateHealth();
}

void URoundHealthbarWidget::UpdateName()
{
    if (!TextBlock_Name || !IsValid(Character))
    {
        return;
    }

    if (Character->CharacterData && !Character->CharacterData->DisplayName.IsEmpty())
    {
        TextBlock_Name->SetText(Character->CharacterData->DisplayName);
        return;
    }

    TextBlock_Name->SetText(FText::FromName(Character->GetFName()));
}

void URoundHealthbarWidget::UpdateHealth()
{
    if (!ProgressBar_Health || !IsValid(Character) || !Character->Attributes)
    {
        return;
    }

    const int32 MaxHealth = FMath::Max(1, Character->Attributes->BaseStats.MaxHealth);
    const float Percent = static_cast<float>(Character->Attributes->CurrentHealth) / static_cast<float>(MaxHealth);
    ProgressBar_Health->SetPercent(Percent);
}

void URoundHealthbarWidget::NativeConstruct()
{
    Super::NativeConstruct();
    UpdateName();
    UpdateHealth();
}

void URoundHealthbarWidget::HandleHealthChanged(int32 NewHealth, int32 Delta)
{
    UpdateHealth();
}
