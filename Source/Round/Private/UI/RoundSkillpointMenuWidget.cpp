#include "UI/RoundSkillpointMenuWidget.h"

#include "Data/RoundAbilityData.h"
#include "Data/RoundCharacterData.h"
#include "Data/RoundSkillProgressionData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Input/Events.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/RoundSkillMenuWidget.h"
#include "UI/RoundSkillpointWidget.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundSkillpointMenu, Log, All);

namespace
{
FRoundSkillEntry* FindOrAddSkillEntry(FRoundCharacterEntry& Character, const FString& SkillName)
{
    if (FRoundSkillEntry* Existing = Character.Skills.FindByPredicate([&SkillName](const FRoundSkillEntry& Entry)
        {
            return Entry.Name.Equals(SkillName, ESearchCase::IgnoreCase);
        }))
    {
        return Existing;
    }

    FRoundSkillEntry& NewEntry = Character.Skills.AddDefaulted_GetRef();
    NewEntry.Name = SkillName;
    return &NewEntry;
}

FString FormatNumericValue(const double Value)
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

FString FormatSkillpointDescription(const FRoundSkillpointDetails& Level)
{
    FString Result = Level.Description;
    for (const TPair<FString, double>& Pair : Level.Values)
    {
        const FString BraceToken = FString::Printf(TEXT("{%s}"), *Pair.Key);
        const FString NegativeBraceToken = FString::Printf(TEXT("{-%s}"), *Pair.Key);
        const FString PercentToken = FString::Printf(TEXT("%%%s%%"), *Pair.Key);
        const FString FormattedValue = FormatNumericValue(Pair.Value);
        const FString InvertedFormattedValue = FormatNumericValue(-Pair.Value);
        Result.ReplaceInline(*BraceToken, *FormattedValue, ESearchCase::IgnoreCase);
        Result.ReplaceInline(*NegativeBraceToken, *InvertedFormattedValue, ESearchCase::IgnoreCase);
        Result.ReplaceInline(*PercentToken, *FormattedValue, ESearchCase::IgnoreCase);
    }

    return Result;
}
}

int32 URoundSkillpointMenuWidget::GetPackedTierSelection(const int64 PackedValue, const int32 TierIndex)
{
    return static_cast<int32>((PackedValue >> (TierIndex * 2)) & 0x3);
}

int64 URoundSkillpointMenuWidget::SetPackedTierSelection(const int64 PackedValue, const int32 TierIndex, const int32 SelectionValue)
{
    const int64 Shift = TierIndex * 2;
    const int64 Cleared = PackedValue & ~(static_cast<int64>(0x3) << Shift);
    return Cleared | ((static_cast<int64>(SelectionValue) & 0x3) << Shift);
}

int32 URoundSkillpointMenuWidget::CountLearnedTiers(const int64 PackedValue)
{
    int32 Count = 0;
    for (int32 TierIndex = 0; TierIndex < 4; ++TierIndex)
    {
        if (GetPackedTierSelection(PackedValue, TierIndex) == 0)
        {
            break;
        }

        ++Count;
    }

    return Count;
}

int32 CountAllSkillSelections(const FRoundCharacterEntry& Character)
{
    int32 Count = 0;
    for (const FRoundSkillEntry& Skill : Character.Skills)
    {
        for (int32 TierIndex = 0; TierIndex < 4; ++TierIndex)
        {
            if (((Skill.Points >> (TierIndex * 2)) & 0x3) != 0)
            {
                ++Count;
            }
        }
    }
    return Count;
}

URoundSkillpointMenuWidget::URoundSkillpointMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    static ConstructorHelpers::FClassFinder<URoundSkillpointWidget> SkillpointWidgetBlueprint(
        TEXT("/Game/TopDown/Blueprints/UI_Skillpoint"));
    if (SkillpointWidgetBlueprint.Succeeded())
    {
        SkillpointWidgetClass = SkillpointWidgetBlueprint.Class;
    }
}

void URoundSkillpointMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (!CanvasPanel_Skills1)
    {
        CanvasPanel_Skills1 = WidgetTree ? Cast<UCanvasPanel>(WidgetTree->RootWidget) : nullptr;
        if (!CanvasPanel_Skills1 && WidgetTree)
        {
            CanvasPanel_Skills1 = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CanvasPanel_Skills1"));
            WidgetTree->RootWidget = CanvasPanel_Skills1;
        }
    }
    if (!TextBlock_0 && CanvasPanel_Skills1 && WidgetTree)
    {
        TextBlock_0 = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TextBlock_0"));
        if (TextBlock_0)
        {
            TextBlock_0->SetText(FText::FromString(SkillName));
            CanvasPanel_Skills1->AddChild(TextBlock_0);
            if (UCanvasPanelSlot* TextSlot = Cast<UCanvasPanelSlot>(TextBlock_0->Slot))
            {
                TextSlot->SetAutoSize(true);
                TextSlot->SetPosition(FVector2D(20.f, 20.f));
            }
        }
    }
    if (Image_0)
    {
        Image_0->OnMouseButtonDownEvent.BindDynamic(this, &URoundSkillpointMenuWidget::HandleLearnImageMouseButtonDown);
    }
    SetVisibility(ESlateVisibility::Visible);
    SetRenderOpacity(1.f);
    if (CanvasPanel_Skills1)
    {
        CanvasPanel_Skills1->SetVisibility(ESlateVisibility::Visible);
        CanvasPanel_Skills1->SetRenderOpacity(1.f);
    }
    UE_LOG(LogRoundSkillpointMenu, Log, TEXT("SkillpointMenu NativeConstruct - Skill=%s Canvas=%s"),
        *SkillName, *GetNameSafe(CanvasPanel_Skills1));
    ReloadPoints();
}

void URoundSkillpointMenuWidget::ReloadPoints()
{
    UE_LOG(LogRoundSkillpointMenu, Log, TEXT("ReloadPoints start - Skill=%s Canvas=%s ExistingChildren=%d Dynamic=%d"),
        *SkillName,
        *GetNameSafe(CanvasPanel_Skills1),
        CanvasPanel_Skills1 ? CanvasPanel_Skills1->GetChildrenCount() : -1,
        DynamicSkillpointWidgets.Num());

    const int32 LearnedPoints = CurrentSkillPoints();
    int32 MaxPoints = 12;
    TArray<FRoundSkillpointDetails> ResolvedLevels;
    FText ResolvedDisplayName = FText::FromString(SkillName);
    const bool bHasResolvedLevels = ResolveSkillpointLevels(ResolvedLevels, ResolvedDisplayName);
    if (bHasResolvedLevels)
    {
        MaxPoints = ResolvedLevels.Num();
    }

    EnsureDynamicSkillpointWidgets(MaxPoints);
    EnsureLearnButton();
    UE_LOG(LogRoundSkillpointMenu, Log, TEXT("ReloadPoints after ensure - Skill=%s Max=%d Children=%d Dynamic=%d Learned=%d"),
        *SkillName,
        MaxPoints,
        CanvasPanel_Skills1 ? CanvasPanel_Skills1->GetChildrenCount() : -1,
        DynamicSkillpointWidgets.Num(),
        LearnedPoints);

    if (TextBlock_0)
    {
        TextBlock_0->SetText(ResolvedDisplayName);
    }

    if (DynamicLearnButton)
    {
        DynamicLearnButton->SetVisibility(ESlateVisibility::Collapsed);
        DynamicLearnButton->SetIsEnabled(false);
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
            Widget->SetVisibility(Index < MaxPoints ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
            Widget->SetIsEnabled(Widget->TierIndex < 4 && CanSelectTierChoice(Widget->TierIndex, Widget->ChoiceIndex) && Index < MaxPoints);

            if (ResolvedLevels.IsValidIndex(Index))
            {
                const FRoundSkillpointDetails& Level = ResolvedLevels[Index];
                const FText LevelTitle = !Level.Title.IsEmpty() ? Level.Title : FText::Format(FText::FromString(TEXT("{0} {1}")), TextBlock_0 ? TextBlock_0->GetText() : FText::FromString(SkillName), FText::AsNumber(Index + 1));
                const FString FormattedDescription = FormatSkillpointDescription(Level);
                const FText LevelDescription = FormattedDescription.IsEmpty() ? LevelTitle : FText::FromString(FormattedDescription);
                Widget->SetToolTipText(FText::Format(FText::FromString(TEXT("{0}\n{1}")), LevelTitle, LevelDescription));
            }

            Widget->Load();
        }
    }
}

bool URoundSkillpointMenuWidget::ResolveSkillpointLevels(TArray<FRoundSkillpointDetails>& OutLevels, FText& OutDisplayName) const
{
    OutLevels.Reset();
    OutDisplayName = FText::FromString(SkillName);

    if (!SkillDetail.SkillpointDescription && SkillMenu)
    {
        if (URoundAbilityData* AbilityData = SkillMenu->FindAbilityDataBySkillName(SkillName))
        {
            if (AbilityData->SkillProgression)
            {
                OutDisplayName = !AbilityData->SkillProgression->DisplayName.IsEmpty()
                    ? AbilityData->SkillProgression->DisplayName
                    : (!AbilityData->DisplayName.IsEmpty() ? AbilityData->DisplayName : FText::FromString(SkillName));
                OutLevels = AbilityData->SkillProgression->Levels;
                return OutLevels.Num() > 0;
            }

            OutDisplayName = !AbilityData->DisplayName.IsEmpty() ? AbilityData->DisplayName : FText::FromString(SkillName);
        }
    }

    if (SkillDetail.SkillpointDescription)
    {
        UDataTable* SkillpointTable = SkillDetail.SkillpointDescription;
        if (SkillpointTable->GetRowStruct() == FRoundSkillpointDetails::StaticStruct())
        {
            TArray<FRoundSkillpointDetails*> Rows;
            SkillpointTable->GetAllRows(TEXT("ResolveSkillpointLevels.Character"), Rows);
            for (const FRoundSkillpointDetails* Row : Rows)
            {
                if (Row)
                {
                    OutLevels.Add(*Row);
                }
            }
        }
        else if (SkillpointTable->GetRowStruct() == FRoundSkillpointsDetails::StaticStruct())
        {
            TArray<FRoundSkillpointsDetails*> Rows;
            SkillpointTable->GetAllRows(TEXT("ResolveSkillpointLevels.Character"), Rows);
            for (const FRoundSkillpointsDetails* Row : Rows)
            {
                if (Row)
                {
                    OutLevels.Append(Row->Skillpoints);
                }
            }
        }

        if (OutLevels.Num() > 0)
        {
            OutDisplayName = !SkillDetail.Name.IsEmpty() ? FText::FromString(SkillDetail.Name) : FText::FromString(SkillName);
            return true;
        }
    }

    const FString CharacterDetailsCsvPath = FPaths::ProjectDir() / TEXT("currentexport/details.csv");
    FString CharacterDetailsCsv;
    if (FFileHelper::LoadFileToString(CharacterDetailsCsv, *CharacterDetailsCsvPath))
    {
        int32 TupleCount = 0;
        int32 SearchIndex = 0;
        while ((SearchIndex = CharacterDetailsCsv.Find(TEXT("Description="), ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchIndex)) != INDEX_NONE)
        {
            ++TupleCount;
            SearchIndex += 12;
        }

        for (int32 Index = 0; Index < TupleCount; ++Index)
        {
            FRoundSkillpointDetails Level;
            Level.Title = FText::Format(FText::FromString(TEXT("{0} {1}")), OutDisplayName, FText::AsNumber(Index + 1));
            Level.Description = FString::Printf(TEXT("Character upgrade %d"), Index + 1);
            OutLevels.Add(Level);
        }

        if (OutLevels.Num() > 0)
        {
            return true;
        }
    }

    return false;
}

bool URoundSkillpointMenuWidget::LearnSkillpoint(const int32 Point, const int32 Value)
{
    const int32 TierIndex = FMath::Clamp((Point - 1) / 3, 0, 3);
    const int32 ChoiceIndex = FMath::Clamp((Point - 1) % 3, 0, 2);
    return SelectTierChoice(TierIndex, ChoiceIndex);
}

bool URoundSkillpointMenuWidget::UnlearnSkillpoint(const int32 Point, const int32 Value)
{
    const int32 TierIndex = FMath::Clamp((Point - 1) / 3, 0, 3);
    return ClearTierChoice(TierIndex);
}

int32 URoundSkillpointMenuWidget::CurrentSkillPoints() const
{
    return CountLearnedTiers(CurrentSkillMask());
}

int64 URoundSkillpointMenuWidget::CurrentSkillMask() const
{
    FRoundCharacterEntry Character;
    if (!SkillMenu || !SkillMenu->TryGetSelectedCharacter(Character))
    {
        return 0;
    }

    if (const FRoundSkillEntry* Skill = Character.Skills.FindByPredicate([this](const FRoundSkillEntry& Entry)
        {
            return Entry.Name.Equals(SkillName, ESearchCase::IgnoreCase);
        }))
    {
        return Skill->Points;
    }

    return 0;
}

bool URoundSkillpointMenuWidget::CurrentSkillLearned() const
{
    if (SkillMenu && SkillMenu->FindAbilityDataBySkillName(SkillName))
    {
        return true;
    }

    FRoundCharacterEntry Character;
    if (!SkillMenu || !SkillMenu->TryGetSelectedCharacter(Character))
    {
        return false;
    }

    return Character.Skills.ContainsByPredicate([this](const FRoundSkillEntry& Entry)
        {
            return Entry.Name.Equals(SkillName, ESearchCase::IgnoreCase);
        });
}

int32 URoundSkillpointMenuWidget::GetTierSelection(const int32 TierIndex) const
{
    return GetPackedTierSelection(CurrentSkillMask(), TierIndex);
}

bool URoundSkillpointMenuWidget::IsTierUnlocked(const int32 TierIndex) const
{
    if (!CurrentSkillLearned())
    {
        return false;
    }

    if (TierIndex <= 0)
    {
        return true;
    }

    return GetTierSelection(TierIndex - 1) != 0;
}

bool URoundSkillpointMenuWidget::IsTierChoiceSelected(const int32 TierIndex, const int32 ChoiceIndex) const
{
    return GetTierSelection(TierIndex) == (ChoiceIndex + 1);
}

bool URoundSkillpointMenuWidget::CanSelectTierChoice(const int32 TierIndex, const int32 ChoiceIndex) const
{
    return ChoiceIndex >= 0 && ChoiceIndex < 3 && TierIndex >= 0 && TierIndex < 4 && IsTierUnlocked(TierIndex);
}

bool URoundSkillpointMenuWidget::SelectTierChoice(const int32 TierIndex, const int32 ChoiceIndex)
{
    UE_LOG(LogRoundSkillpointMenu, Log,
        TEXT("SelectTierChoice request - Skill=%s Tier=%d Choice=%d Mask=%lld Learned=%d Unlocked=%d Remaining=%d"),
        *SkillName, TierIndex, ChoiceIndex, CurrentSkillMask(), CurrentSkillLearned() ? 1 : 0,
        IsTierUnlocked(TierIndex) ? 1 : 0, SkillMenu ? SkillMenu->GetRemainingTeamSkillPoints() : -1);
    if (!CanSelectTierChoice(TierIndex, ChoiceIndex))
    {
        UE_LOG(LogRoundSkillpointMenu, Warning, TEXT("SelectTierChoice denied by unlock rules - Skill=%s Tier=%d Choice=%d"),
            *SkillName, TierIndex, ChoiceIndex);
        return false;
    }

    FRoundCharacterEntry Character;
    int32 CharacterIndex = INDEX_NONE;
    FRoundSkillEntry* Skill = nullptr;
    if (!TryGetCharacterAndSkill(Character, CharacterIndex, Skill))
    {
        UE_LOG(LogRoundSkillpointMenu, Warning, TEXT("SelectTierChoice failed to resolve character/skill - Skill=%s"), *SkillName);
        return false;
    }

    const int32 PreviousSelection = GetPackedTierSelection(Skill->Points, TierIndex);
    if (PreviousSelection == ChoiceIndex + 1)
    {
        return true;
    }

    const bool bConsumesPoint = PreviousSelection == 0;
    if (bConsumesPoint)
    {
        const int32 RemainingPoints = SkillMenu ? SkillMenu->GetRemainingTeamSkillPoints() : 0;
        if (RemainingPoints <= 0)
        {
            UE_LOG(LogRoundSkillpointMenu, Warning, TEXT("SelectTierChoice denied by budget - Skill=%s Tier=%d Choice=%d Remaining=%d"),
                *SkillName, TierIndex, ChoiceIndex, RemainingPoints);
            return false;
        }

    }

    Skill->Points = SetPackedTierSelection(Skill->Points, TierIndex, ChoiceIndex + 1);
    Character.Skillpoints = CountAllSkillSelections(Character);

    if (!SkillMenu || !SkillMenu->CommitSelectedCharacter(Character, false))
    {
        UE_LOG(LogRoundSkillpointMenu, Warning, TEXT("SelectTierChoice commit failed - Skill=%s Tier=%d Choice=%d"),
            *SkillName, TierIndex, ChoiceIndex);
        return false;
    }

    UE_LOG(LogRoundSkillpointMenu, Log, TEXT("SelectTierChoice applied - Skill=%s Tier=%d Choice=%d NewMask=%lld StoredSkillpoints=%lld"),
        *SkillName, TierIndex, ChoiceIndex, Skill->Points, Character.Skillpoints);

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

bool URoundSkillpointMenuWidget::ClearTierChoice(const int32 TierIndex)
{
    if (TierIndex < 0 || TierIndex >= 4)
    {
        return false;
    }

    FRoundCharacterEntry Character;
    int32 CharacterIndex = INDEX_NONE;
    FRoundSkillEntry* Skill = nullptr;
    if (!TryGetCharacterAndSkill(Character, CharacterIndex, Skill))
    {
        return false;
    }

    int32 RefundedPoints = 0;
    for (int32 CurrentTier = TierIndex; CurrentTier < 4; ++CurrentTier)
    {
        if (GetPackedTierSelection(Skill->Points, CurrentTier) != 0)
        {
            ++RefundedPoints;
            Skill->Points = SetPackedTierSelection(Skill->Points, CurrentTier, 0);
        }
    }

    if (RefundedPoints <= 0)
    {
        return false;
    }

    if (Skill->Points == 0)
    {
        Character.Skills.RemoveAll([this](const FRoundSkillEntry& Entry)
            {
                return Entry.Name.Equals(SkillName, ESearchCase::IgnoreCase);
            });
    }

    Character.Skillpoints = CountAllSkillSelections(Character);

    if (!SkillMenu || !SkillMenu->CommitSelectedCharacter(Character, false))
    {
        return false;
    }

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

bool URoundSkillpointMenuWidget::LearnSkill()
{
    return false;
}

bool URoundSkillpointMenuWidget::UnlearnSkill()
{
    return false;
}

TArray<URoundSkillpointWidget*> URoundSkillpointMenuWidget::GetSkillpointWidgets() const
{
    TArray<URoundSkillpointWidget*> Widgets;

    const auto AddWidgetOnce = [&Widgets](URoundSkillpointWidget* Widget)
    {
        if (Widget)
        {
            Widgets.AddUnique(Widget);
        }
    };

    AddWidgetOnce(Skillpoint);
    AddWidgetOnce(Skillpoint_1);
    AddWidgetOnce(Skillpoint_2);
    AddWidgetOnce(Skillpoint_3);
    AddWidgetOnce(Skillpoint_4);
    AddWidgetOnce(Skillpoint_5);
    AddWidgetOnce(Skillpoint_6);
    AddWidgetOnce(Skillpoint_7);
    AddWidgetOnce(Skillpoint_8);
    AddWidgetOnce(Skillpoint_9);
    AddWidgetOnce(Skillpoint_10);
    AddWidgetOnce(Skillpoint_11);

    for (URoundSkillpointWidget* Widget : DynamicSkillpointWidgets)
    {
        AddWidgetOnce(Widget);
    }

    return Widgets;
}

bool URoundSkillpointMenuWidget::TryGetCharacterAndSkill(FRoundCharacterEntry& OutCharacter, int32& OutCharacterIndex, FRoundSkillEntry*& OutSkill)
{
    if (!SkillMenu || !SkillMenu->TryGetSelectedCharacter(OutCharacter))
    {
        return false;
    }

    OutCharacterIndex = SkillMenu->GetSelectedCharacterIndex();
    OutSkill = FindOrAddSkillEntry(OutCharacter, SkillName);
    return OutSkill != nullptr;
}

void URoundSkillpointMenuWidget::EnsureDynamicSkillpointWidgets(const int32 NeededPoints)
{
    if (!CanvasPanel_Skills1 || NeededPoints <= 0)
    {
        UE_LOG(LogRoundSkillpointMenu, Warning, TEXT("EnsureDynamicSkillpointWidgets skipped - Canvas=%s Needed=%d"),
            *GetNameSafe(CanvasPanel_Skills1), NeededPoints);
        return;
    }

    int32 ExistingBoundWidgets = 0;
    for (URoundSkillpointWidget* Widget : GetSkillpointWidgets())
    {
        if (Widget)
        {
            ++ExistingBoundWidgets;
        }
    }

    if (ExistingBoundWidgets >= NeededPoints)
    {
        UE_LOG(LogRoundSkillpointMenu, Log, TEXT("EnsureDynamicSkillpointWidgets using existing blueprint widgets - Needed=%d ExistingBound=%d Children=%d"),
            NeededPoints, ExistingBoundWidgets, CanvasPanel_Skills1->GetChildrenCount());
        return;
    }

    const bool bNeedsRebuild = DynamicSkillpointWidgets.Num() < NeededPoints
        || CanvasPanel_Skills1->GetChildrenCount() == 0;

    if (!bNeedsRebuild)
    {
        UE_LOG(LogRoundSkillpointMenu, Log, TEXT("EnsureDynamicSkillpointWidgets reuse - Needed=%d Children=%d Dynamic=%d"),
            NeededPoints, CanvasPanel_Skills1->GetChildrenCount(), DynamicSkillpointWidgets.Num());
        return;
    }

    CanvasPanel_Skills1->ClearChildren();
    DynamicLearnButton = nullptr;
    DynamicLearnButtonLabel = nullptr;
    DynamicSkillpointWidgets.Reset();
    UE_LOG(LogRoundSkillpointMenu, Log, TEXT("EnsureDynamicSkillpointWidgets rebuilding - Needed=%d"), NeededPoints);

    TSubclassOf<URoundSkillpointWidget> WidgetClass = SkillpointWidgetClass;
    if (!WidgetClass)
    {
        WidgetClass = URoundSkillpointWidget::StaticClass();
    }

    while (DynamicSkillpointWidgets.Num() < NeededPoints)
    {
        URoundSkillpointWidget* Widget = CreateWidget<URoundSkillpointWidget>(this, WidgetClass);
        if (!Widget)
        {
            UE_LOG(LogRoundSkillpointMenu, Warning, TEXT("Failed to create skillpoint widget for %s at index %d"), *SkillName, DynamicSkillpointWidgets.Num());
            break;
        }

        Widget->SkillpointMenu = this;
        Widget->PointIndex = DynamicSkillpointWidgets.Num() + 1;
        Widget->SetVisibility(ESlateVisibility::Visible);
        Widget->SetRenderOpacity(1.f);
        CanvasPanel_Skills1->AddChild(Widget);

        if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
        {
            const int32 WidgetIndex = DynamicSkillpointWidgets.Num();
            const int32 Column = WidgetIndex / 3;
            const int32 Row = WidgetIndex % 3;
            CanvasSlot->SetAutoSize(false);
            CanvasSlot->SetSize(FVector2D(32.f, 32.f));
            CanvasSlot->SetPosition(FVector2D(20.f + Column * 48.f, 72.f + Row * 48.f));
        }

        DynamicSkillpointWidgets.Add(Widget);
        UE_LOG(LogRoundSkillpointMenu, Log, TEXT("Added skillpoint widget - Skill=%s Index=%d Children=%d"),
            *SkillName, DynamicSkillpointWidgets.Num(), CanvasPanel_Skills1->GetChildrenCount());
    }
}

void URoundSkillpointMenuWidget::EnsureLearnButton()
{
    if (!CanvasPanel_Skills1 || DynamicLearnButton)
    {
        return;
    }

    if (CanvasPanel_Skills1->GetChildrenCount() > DynamicSkillpointWidgets.Num())
    {
        return;
    }

    DynamicLearnButton = NewObject<UButton>(this);
    DynamicLearnButtonLabel = NewObject<UTextBlock>(DynamicLearnButton);
    if (!DynamicLearnButton || !DynamicLearnButtonLabel)
    {
        return;
    }

    DynamicLearnButton->SetVisibility(ESlateVisibility::Visible);
    DynamicLearnButton->SetRenderOpacity(1.f);
    DynamicLearnButtonLabel->SetText(FText::FromString(TEXT("Learn")));
    DynamicLearnButtonLabel->SetJustification(ETextJustify::Center);
    DynamicLearnButton->AddChild(DynamicLearnButtonLabel);
    DynamicLearnButton->OnClicked.AddDynamic(this, &URoundSkillpointMenuWidget::HandleLearnButtonClicked);

    CanvasPanel_Skills1->AddChild(DynamicLearnButton);
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(DynamicLearnButton->Slot))
    {
        CanvasSlot->SetAutoSize(false);
        CanvasSlot->SetSize(FVector2D(160.f, 32.f));
        CanvasSlot->SetPosition(FVector2D(20.f, 20.f));
    }
}

void URoundSkillpointMenuWidget::HandleLearnButtonClicked()
{
    LearnSkill();
}

FEventReply URoundSkillpointMenuWidget::HandleLearnImageMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        LearnSkill();
        return UWidgetBlueprintLibrary::Handled();
    }

    if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        UnlearnSkill();
        return UWidgetBlueprintLibrary::Handled();
    }

    return UWidgetBlueprintLibrary::Unhandled();
}
