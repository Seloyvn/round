#include "UI/RoundStartFieldWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "UI/RoundStartPositionWidget.h"

void URoundStartFieldWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Button_375)
    {
        Button_375->OnClicked.AddDynamic(this, &URoundStartFieldWidget::HandleClicked);
    }

    if (main)
    {
        main->RegisterField(this);
    }
}

void URoundStartFieldWidget::Refresh()
{
    if (main)
    {
        main->Refresh();
    }
}

void URoundStartFieldWidget::SetColor(const FLinearColor& InColor)
{
    if (Image_41)
    {
        Image_41->SetColorAndOpacity(InColor);
    }
}

void URoundStartFieldWidget::HandleClicked()
{
    if (main)
    {
        main->HandleFieldClicked(this);
    }
}
