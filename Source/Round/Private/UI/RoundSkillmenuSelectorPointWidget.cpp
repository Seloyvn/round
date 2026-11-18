#include "UI/RoundSkillmenuSelectorPointWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/RoundSkillMenuWidget.h"
#include "UI/RoundSkillpointMenuWidget.h"

void URoundSkillmenuSelectorPointWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Button_42)
    {
        Button_42->OnClicked.AddDynamic(this, &URoundSkillmenuSelectorPointWidget::HandleClicked);
    }

    if (TextBlock_40)
    {
        TextBlock_40->SetText(FText::FromString(!SkillName.IsEmpty() ? SkillName : SkillDetail.Name));
    }

    const FText Description = FText::FromString(SkillDetail.Description);
    SetToolTipText(Description);
    if (Button_42)
    {
        Button_42->SetToolTipText(Description);
    }
    if (TextBlock_40)
    {
        TextBlock_40->SetToolTipText(Description);
    }

    SetLearned(SkillpointMenu && SkillpointMenu->CurrentSkillLearned());
}

void URoundSkillmenuSelectorPointWidget::SetLearned(const bool bLearned)
{
    if (TextBlock_40)
    {
        TextBlock_40->SetColorAndOpacity(bLearned ? FSlateColor(FLinearColor(0.95f, 0.78f, 0.18f, 1.f)) : FSlateColor(FLinearColor::White));
    }
}

void URoundSkillmenuSelectorPointWidget::HandleClicked()
{
    if (IsValid(Skillmenu) && IsValid(SkillpointMenu))
    {
        Skillmenu->SetSelectedSkillIndex(SelectorIndex);
        SkillpointMenu->SkillMenu = Skillmenu;
        SkillpointMenu->SkillName = SkillDetail.Name;
        SkillpointMenu->SkillDetail = SkillDetail;
        Skillmenu->SetSkillpointMenu(SkillpointMenu);
        SkillpointMenu->ReloadPoints();
        SetLearned(SkillpointMenu->CurrentSkillLearned());
    }
}
