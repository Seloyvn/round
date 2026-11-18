#include "UI/RoundSkillpointWidget.h"

#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Input/Reply.h"
#include "Styling/SlateColor.h"
#include "UI/RoundSkillpointMenuWidget.h"
#include "Blueprint/WidgetTree.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundSkillpointInput, Log, All);

void URoundSkillpointWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!Image_80 && WidgetTree)
    {
        Image_80 = Cast<UImage>(WidgetTree->FindWidget(TEXT("Image_80")));
        if (!Image_80)
        {
            WidgetTree->ForEachWidget([this](UWidget* Widget)
            {
                if (!Image_80)
                {
                    Image_80 = Cast<UImage>(Widget);
                }
            });
        }
    }
    UE_LOG(LogRoundSkillpointInput, Log,
        TEXT("Construct point - Widget=%s Image=%s Menu=%s Point=%d Tier=%d Choice=%d Visibility=%d Enabled=%d"),
        *GetNameSafe(this), *GetNameSafe(Image_80), *GetNameSafe(SkillpointMenu), PointIndex, TierIndex, ChoiceIndex,
        static_cast<int32>(GetVisibility()), GetIsEnabled() ? 1 : 0);
    Load();
}

void URoundSkillpointWidget::Load()
{
    if (!Image_80 || !SkillpointMenu)
    {
        UE_LOG(LogRoundSkillpointInput, Warning,
            TEXT("Load point skipped - Widget=%s Image=%s Menu=%s Point=%d Tier=%d Choice=%d"),
            *GetNameSafe(this), *GetNameSafe(Image_80), *GetNameSafe(SkillpointMenu), PointIndex, TierIndex, ChoiceIndex);
        return;
    }

    SetVisibility(ESlateVisibility::Visible);
    SetRenderOpacity(1.f);
    Image_80->SetVisibility(ESlateVisibility::Visible);
    Image_80->SetRenderOpacity(1.f);

    if (!Image_80->GetBrush().GetResourceObject())
    {
        if (UTexture2D* WhiteTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture")))
        {
            Image_80->SetBrushFromTexture(WhiteTexture, true);
        }
    }

    Image_80->SetDesiredSizeOverride(FVector2D(28.f, 28.f));

    const bool bSelected = SkillpointMenu->IsTierChoiceSelected(TierIndex, ChoiceIndex);
    const bool bUnlocked = SkillpointMenu->CanSelectTierChoice(TierIndex, ChoiceIndex);
    const FLinearColor Color = bSelected
        ? FLinearColor(0.95f, 0.78f, 0.18f, 1.f)
        : (bUnlocked ? FLinearColor(0.45f, 0.75f, 1.f, 1.f) : FLinearColor(0.25f, 0.25f, 0.25f, 1.f));
    Image_80->SetColorAndOpacity(Color);
    UE_LOG(LogRoundSkillpointInput, Log,
        TEXT("Load point state - Widget=%s Skill=%s Point=%d Tier=%d Choice=%d Mask=%lld Selected=%d Unlocked=%d WidgetEnabled=%d Color=%s"),
        *GetNameSafe(this), *SkillpointMenu->SkillName, PointIndex, TierIndex, ChoiceIndex,
        SkillpointMenu->CurrentSkillMask(), bSelected ? 1 : 0, bUnlocked ? 1 : 0, GetIsEnabled() ? 1 : 0,
        *Color.ToString());
    if (GetToolTipText().IsEmpty())
    {
        SetToolTipText(FText::FromString(FString::Printf(TEXT("%s T%d Option %d"), *SkillpointMenu->SkillName, TierIndex + 1, ChoiceIndex + 1)));
    }
}

FReply URoundSkillpointWidget::NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent& InMouseEvent)
{
    UE_LOG(LogRoundSkillpointInput, Log,
        TEXT("Point mouse down - Widget=%s Menu=%s Skill=%s Point=%d Tier=%d Choice=%d Button=%s Enabled=%d Visibility=%d"),
        *GetNameSafe(this), *GetNameSafe(SkillpointMenu), SkillpointMenu ? *SkillpointMenu->SkillName : TEXT("None"),
        PointIndex, TierIndex, ChoiceIndex, *InMouseEvent.GetEffectingButton().ToString(), GetIsEnabled() ? 1 : 0,
        static_cast<int32>(GetVisibility()));
    if (!SkillpointMenu)
    {
        return Super::NativeOnMouseButtonDown(FGeometry(), InMouseEvent);
    }

    if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
    {
        const bool bApplied = SkillpointMenu->LearnSkillpoint(PointIndex, 1);
        UE_LOG(LogRoundSkillpointInput, Log, TEXT("Point left click result - Skill=%s Point=%d Applied=%d NewMask=%lld"),
            *SkillpointMenu->SkillName, PointIndex, bApplied ? 1 : 0, SkillpointMenu->CurrentSkillMask());
        return FReply::Handled();
    }

    if (InMouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
    {
        const bool bApplied = SkillpointMenu->UnlearnSkillpoint(PointIndex, 1);
        UE_LOG(LogRoundSkillpointInput, Log, TEXT("Point right click result - Skill=%s Point=%d Applied=%d NewMask=%lld"),
            *SkillpointMenu->SkillName, PointIndex, bApplied ? 1 : 0, SkillpointMenu->CurrentSkillMask());
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(FGeometry(), InMouseEvent);
}
