#include "UI/RoundObjectiveMarkerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "InputCoreTypes.h"
#include "Input/Reply.h"
#include "Styling/SlateBrush.h"
#include "Game/RoundPlayerPawn.h"
#include "UI/RoundInitiativeBarWidget.h"

void URoundObjectiveMarkerWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!WidgetTree)
    {
        return;
    }

    if (!MarkerImage)
    {
        MarkerImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MarkerImage"));
        MarkerImage->SetDesiredSizeOverride(FVector2D(18.0f, 18.0f));
        MarkerImage->SetVisibility(ESlateVisibility::Visible);
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::Box;
        Brush.TintColor = FSlateColor(FLinearColor(1.0f, 0.85f, 0.15f, 1.0f));
        Brush.ImageSize = FVector2D(18.0f, 18.0f);
        MarkerImage->SetBrush(Brush);
        WidgetTree->RootWidget = MarkerImage;
    }

    if (!PendingTooltip.IsEmpty())
    {
        SetToolTipText(FText::FromString(PendingTooltip));
    }

    SetVisibility(ESlateVisibility::Visible);
    SetIsEnabled(true);
}

FReply URoundObjectiveMarkerWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        SelectTargetTile();
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void URoundObjectiveMarkerWidget::InitializeMarker(URoundInitiativeBarWidget* InOwnerBar, ARoundGridTile* InTargetTile, const FString& InLabel, const FString& InTooltip)
{
    OwnerBar = InOwnerBar;
    TargetTile = InTargetTile;
    PendingLabel = InLabel;
    PendingTooltip = InTooltip;

    SetToolTipText(FText::FromString(PendingTooltip));

    const FLinearColor FillColor =
        PendingLabel.Equals(TEXT("Hold"), ESearchCase::IgnoreCase) ? FLinearColor(0.15f, 0.75f, 1.0f, 1.0f) :
        PendingLabel.Equals(TEXT("Destroy"), ESearchCase::IgnoreCase) ? FLinearColor(1.0f, 0.3f, 0.2f, 1.0f) :
        PendingLabel.Equals(TEXT("Channel"), ESearchCase::IgnoreCase) ? FLinearColor(0.7f, 0.35f, 1.0f, 1.0f) :
        FLinearColor(1.0f, 0.85f, 0.15f, 1.0f);

    if (MarkerImage)
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::Box;
        Brush.TintColor = FSlateColor(FillColor);
        Brush.ImageSize = FVector2D(18.0f, 18.0f);
        MarkerImage->SetBrush(Brush);
        MarkerImage->SetColorAndOpacity(FLinearColor::White);
    }
}

void URoundObjectiveMarkerWidget::UpdatePosition(const float XPosition)
{
    if (UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(this))
    {
        CanvasSlot->SetAutoSize(false);
        CanvasSlot->SetSize(FVector2D(20.0f, 20.0f));
        CanvasSlot->SetPosition(FVector2D(XPosition, -8.0f));
    }
}

void URoundObjectiveMarkerWidget::SelectTargetTile() const
{
    if (!TargetTile)
    {
        return;
    }

    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (ARoundPlayerPawn* PlayerPawn = Cast<ARoundPlayerPawn>(PlayerController->GetPawn()))
        {
            PlayerPawn->SelectField(TargetTile);
        }
    }
}
