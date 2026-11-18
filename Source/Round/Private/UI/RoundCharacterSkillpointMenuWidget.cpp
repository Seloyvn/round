#include "UI/RoundCharacterSkillpointMenuWidget.h"

#include "Data/RoundCharacterData.h"
#include "Data/RoundCharacterProgressionData.h"
#include "UI/RoundSkillpointWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Engine.h"
#include "UI/RoundSkillMenuWidget.h"

namespace
{
int32 GetPackedTierSelectionLocal(const int64 PackedValue, const int32 TierIndex)
{
    return static_cast<int32>((PackedValue >> (TierIndex * 2)) & 0x3);
}

int64 SetPackedTierSelectionLocal(const int64 PackedValue, const int32 TierIndex, const int32 SelectionValue)
{
    const int64 Shift = TierIndex * 2;
    const int64 Cleared = PackedValue & ~(static_cast<int64>(0x3) << Shift);
    return Cleared | ((static_cast<int64>(SelectionValue) & 0x3) << Shift);
}

int32 CountLearnedTiersLocal(const int64 PackedValue)
{
    int32 Count = 0;
    for (int32 TierIndex = 0; TierIndex < 4; ++TierIndex)
    {
        if (GetPackedTierSelectionLocal(PackedValue, TierIndex) == 0)
        {
            break;
        }

        ++Count;
    }

    return Count;
}

bool ResolveCharacterTierChoices(const URoundSkillMenuWidget* SkillMenu, TArray<FRoundSkillpointDetails>& OutLevels, FText& OutDisplayName)
{
    OutLevels.Reset();
    OutDisplayName = FText::FromString(TEXT("Character"));

    if (!SkillMenu)
    {
        return false;
    }

    URoundCharacterData* CharacterData = nullptr;
    if (!SkillMenu->TryGetSelectedCharacterData(CharacterData) || !CharacterData || !CharacterData->CharacterProgression)
    {
        return false;
    }

    URoundCharacterProgressionData* Progression = CharacterData->CharacterProgression;
    OutDisplayName = !Progression->DisplayName.IsEmpty()
        ? Progression->DisplayName
        : (!CharacterData->DisplayName.IsEmpty() ? CharacterData->DisplayName : FText::FromString(TEXT("Character")));

    for (const FRoundSkillpointTier& Tier : Progression->Tiers)
    {
        OutLevels.Append(Tier.Choices);
    }

    return OutLevels.Num() > 0;
}

FString FormatCharacterNumericValue(const double Value)
{
    const double Rounded = FMath::RoundToDouble(Value);
    if (FMath::IsNearlyEqual(Value, Rounded))
    {
        return FString::FromInt(static_cast<int32>(Rounded));
    }

    FString Result = FString::SanitizeFloat(Value);
    while (Result.Contains(TEXT(".")) && Result.EndsWith(TEXT("0")))
    {
        Result.LeftChopInline(1);
    }
    if (Result.EndsWith(TEXT(".")))
    {
        Result.LeftChopInline(1);
    }
    return Result;
}

FString FormatCharacterSkillpointDescription(const FRoundSkillpointDetails& Level)
{
    FString Result = Level.Description;
    for (const TPair<FString, double>& Pair : Level.Values)
    {
        const FString BraceToken = FString::Printf(TEXT("{%s}"), *Pair.Key);
        const FString NegativeBraceToken = FString::Printf(TEXT("{-%s}"), *Pair.Key);
        const FString PercentToken = FString::Printf(TEXT("%%%s%%"), *Pair.Key);
        const FString FormattedValue = FormatCharacterNumericValue(Pair.Value);
        const FString InvertedFormattedValue = FormatCharacterNumericValue(-Pair.Value);
        Result.ReplaceInline(*BraceToken, *FormattedValue, ESearchCase::IgnoreCase);
        Result.ReplaceInline(*NegativeBraceToken, *InvertedFormattedValue, ESearchCase::IgnoreCase);
        Result.ReplaceInline(*PercentToken, *FormattedValue, ESearchCase::IgnoreCase);
    }

    return Result;
}
}

bool URoundCharacterSkillpointMenuWidget::LearnSkillpoint(const int32 Point, const int32 Value)
{
    const int32 TierIndex = FMath::Clamp((Point - 1) / 3, 0, 3);
    const int32 ChoiceIndex = FMath::Clamp((Point - 1) % 3, 0, 2);
    return SelectTierChoice(TierIndex, ChoiceIndex);
}

void URoundCharacterSkillpointMenuWidget::ReloadPoints()
{
    TArray<FRoundSkillpointDetails> ResolvedLevels;
    FText ResolvedDisplayName = FText::FromString(SkillName);
    const bool bHasCharacterProgression = ResolveCharacterTierChoices(SkillMenu, ResolvedLevels, ResolvedDisplayName);

    if (bHasCharacterProgression)
    {
        if (TextBlock_0)
        {
            TextBlock_0->SetText(ResolvedDisplayName);
        }

        EnsureDynamicSkillpointWidgets(ResolvedLevels.Num());
        EnsureLearnButton();

        if (DynamicLearnButton)
        {
            DynamicLearnButton->SetVisibility(ESlateVisibility::Collapsed);
        }
        if (Image_0)
        {
            Image_0->SetVisibility(ESlateVisibility::Collapsed);
        }

        TArray<URoundSkillpointWidget*> Widgets = GetSkillpointWidgets();
        for (int32 Index = 0; Index < Widgets.Num(); ++Index)
        {
            if (URoundSkillpointWidget* Widget = Widgets[Index])
            {
                Widget->SkillpointMenu = this;
                Widget->PointIndex = Index + 1;
                Widget->TierIndex = Index / 3;
                Widget->ChoiceIndex = Index % 3;
                const bool bVisible = Index < ResolvedLevels.Num();
                Widget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
                Widget->SetIsEnabled(
                    bVisible
                    && CanSelectTierChoice(Widget->TierIndex, Widget->ChoiceIndex));

                if (ResolvedLevels.IsValidIndex(Index))
                {
                    const FRoundSkillpointDetails& Level = ResolvedLevels[Index];
                    const FText LevelTitle = !Level.Title.IsEmpty() ? Level.Title : FText::Format(FText::FromString(TEXT("{0} {1}")), ResolvedDisplayName, FText::AsNumber(Index + 1));
                    const FString FormattedDescription = FormatCharacterSkillpointDescription(Level);
                    const FText LevelDescription = FormattedDescription.IsEmpty() ? LevelTitle : FText::FromString(FormattedDescription);
                    Widget->SetToolTipText(FText::Format(FText::FromString(TEXT("{0}\n{1}")), LevelTitle, LevelDescription));
                }

                Widget->Load();
            }
        }

        return;
    }

    Super::ReloadPoints();
}

bool URoundCharacterSkillpointMenuWidget::UnlearnSkillpoint(const int32 Point, const int32 Value)
{
    const int32 TierIndex = FMath::Clamp((Point - 1) / 3, 0, 3);
    return ClearTierChoice(TierIndex);
}

int32 URoundCharacterSkillpointMenuWidget::CurrentSkillPoints() const
{
    return CountLearnedTiersLocal(CurrentSkillMask());
}

int64 URoundCharacterSkillpointMenuWidget::CurrentSkillMask() const
{
    FRoundCharacterEntry Character;
    if (!SkillMenu || !SkillMenu->TryGetSelectedCharacter(Character))
    {
        return 0;
    }

    return Character.CharacterSkillSelections;
}

bool URoundCharacterSkillpointMenuWidget::CurrentSkillLearned() const
{
    return true;
}

bool URoundCharacterSkillpointMenuWidget::IsTierUnlocked(const int32 TierIndex) const
{
    if (TierIndex <= 0)
    {
        return true;
    }

    return GetPackedTierSelectionLocal(CurrentSkillMask(), TierIndex - 1) != 0;
}

bool URoundCharacterSkillpointMenuWidget::IsTierChoiceSelected(const int32 TierIndex, const int32 ChoiceIndex) const
{
    return GetPackedTierSelectionLocal(CurrentSkillMask(), TierIndex) == (ChoiceIndex + 1);
}

bool URoundCharacterSkillpointMenuWidget::CanSelectTierChoice(const int32 TierIndex, const int32 ChoiceIndex) const
{
    return ChoiceIndex >= 0 && ChoiceIndex < 3 && TierIndex >= 0 && TierIndex < 4 && IsTierUnlocked(TierIndex);
}

bool URoundCharacterSkillpointMenuWidget::SelectTierChoice(const int32 TierIndex, const int32 ChoiceIndex)
{
    if (!CanSelectTierChoice(TierIndex, ChoiceIndex))
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterSkillpoint SelectTierChoice denied - Tier=%d Choice=%d"), TierIndex, ChoiceIndex);
        return false;
    }

    FRoundCharacterEntry Character;
    if (!SkillMenu || !SkillMenu->TryGetSelectedCharacter(Character))
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterSkillpoint SelectTierChoice no selected character - Tier=%d Choice=%d"), TierIndex, ChoiceIndex);
        return false;
    }

    const int32 PreviousSelection = GetPackedTierSelectionLocal(Character.CharacterSkillSelections, TierIndex);
    if (PreviousSelection == ChoiceIndex + 1)
    {
        return true;
    }

    if (PreviousSelection == 0)
    {
        const int32 RemainingPoints = SkillMenu ? SkillMenu->GetRemainingTeamSkillPoints() : 0;
        if (RemainingPoints <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("CharacterSkillpoint SelectTierChoice no remaining points - Tier=%d Choice=%d"), TierIndex, ChoiceIndex);
            return false;
        }

    }

    Character.CharacterSkillSelections = SetPackedTierSelectionLocal(Character.CharacterSkillSelections, TierIndex, ChoiceIndex + 1);
    Character.CharacterSkillpoints = CountLearnedTiersLocal(Character.CharacterSkillSelections);
    if (!SkillMenu->CommitSelectedCharacter(Character, false))
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterSkillpoint SelectTierChoice commit failed - Tier=%d Choice=%d"), TierIndex, ChoiceIndex);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("CharacterSkillpoint SelectTierChoice applied - Tier=%d Choice=%d Mask=%lld Spent=%lld"),
        TierIndex, ChoiceIndex, Character.CharacterSkillSelections, Character.CharacterSkillpoints);
    if (SkillMenu)
    {
        SkillMenu->RefreshAfterSkillpointChange();
    }
    else
    {
        ReloadPoints();
    }
    return true;
}

bool URoundCharacterSkillpointMenuWidget::ClearTierChoice(const int32 TierIndex)
{
    if (TierIndex < 0 || TierIndex >= 4)
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterSkillpoint ClearTierChoice invalid tier - Tier=%d"), TierIndex);
        return false;
    }

    FRoundCharacterEntry Character;
    if (!SkillMenu || !SkillMenu->TryGetSelectedCharacter(Character))
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterSkillpoint ClearTierChoice no selected character - Tier=%d"), TierIndex);
        return false;
    }

    int32 RefundedPoints = 0;
    for (int32 CurrentTier = TierIndex; CurrentTier < 4; ++CurrentTier)
    {
        if (GetPackedTierSelectionLocal(Character.CharacterSkillSelections, CurrentTier) != 0)
        {
            ++RefundedPoints;
            Character.CharacterSkillSelections = SetPackedTierSelectionLocal(Character.CharacterSkillSelections, CurrentTier, 0);
        }
    }

    if (RefundedPoints <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterSkillpoint ClearTierChoice nothing to clear - Tier=%d"), TierIndex);
        return false;
    }

    Character.CharacterSkillpoints = CountLearnedTiersLocal(Character.CharacterSkillSelections);
    if (!SkillMenu->CommitSelectedCharacter(Character, false))
    {
        UE_LOG(LogTemp, Warning, TEXT("CharacterSkillpoint ClearTierChoice commit failed - Tier=%d"), TierIndex);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("CharacterSkillpoint ClearTierChoice applied - Tier=%d Mask=%lld Spent=%lld"),
        TierIndex, Character.CharacterSkillSelections, Character.CharacterSkillpoints);
    if (SkillMenu)
    {
        SkillMenu->RefreshAfterSkillpointChange();
    }
    else
    {
        ReloadPoints();
    }
    return true;
}

bool URoundCharacterSkillpointMenuWidget::LearnSkill()
{
    return false;
}

bool URoundCharacterSkillpointMenuWidget::UnlearnSkill()
{
    return false;
}
