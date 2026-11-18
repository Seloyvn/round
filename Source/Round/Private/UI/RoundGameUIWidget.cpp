#include "UI/RoundGameUIWidget.h"

#include "Components/Button.h"
#include "Components/ListView.h"
#include "Components/Overlay.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundCharacter.h"
#include "Actors/RoundEffectActor.h"
#include "Actors/RoundGridTile.h"
#include "Actors/RoundObjectiveActor.h"
#include "Actors/RoundObjectiveRewardActor.h"
#include "Actors/RoundTerrainActor.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundAbilityData.h"
#include "Data/RoundCharacterData.h"
#include "Data/RoundGameDataRegistryLookup.h"
#include "Data/RoundSkillProgressionData.h"
#include "Data/RoundTerrainData.h"
#include "Effects/RoundAuraEffect.h"
#include "Game/RoundGameState.h"
#include "Game/RoundPlayerPawn.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Blueprint/WidgetTree.h"
#include "UI/RoundAuraEntryWidget.h"
#include "UI/RoundAuraListItemData.h"
#include "UI/RoundChampionCustomUIWidget.h"
#include "UI/RoundInitiativeBarWidget.h"
#include "UI/RoundInitiativeIconWidget.h"
#include "UI/RoundSkillHoverInfoWidget.h"
#include "UI/RoundSkillHoverListEntryWidget.h"

namespace
{
double FindNumericValueCaseInsensitive(const TMap<FString, double>& Values, const TArray<FString>& CandidateKeys)
{
    for (const FString& CandidateKey : CandidateKeys)
    {
        for (const TPair<FString, double>& Pair : Values)
        {
            if (Pair.Key.Equals(CandidateKey, ESearchCase::IgnoreCase))
            {
                return Pair.Value;
            }
        }
    }

    return 0.0;
}

bool IsPuppetCharacterForUI(const ARoundCharacter* Character)
{
    if (!Character)
    {
        return false;
    }

    const FString RowName = Character->CharacterRowName.ToString();
    return RowName.Contains(TEXT("Puppet"), ESearchCase::IgnoreCase);
}

FText BuildBaseStatsText(const ARoundCharacter* Character)
{
    const URoundAttributeSetComponent* Attributes = Character ? Character->Attributes : nullptr;
    if (!Attributes)
    {
        return FText::GetEmpty();
    }

    const FRoundStatBlock& Stats = Attributes->BaseStats;
    const FString StatsString = FString::Printf(
        TEXT("ATK %d\nDEF %d\nRange %d\nMoveCost %d\nAttackCost %d\nTurnDelay %d"),
        Stats.Attack,
        Stats.Defense,
        Stats.AttackRange,
        Stats.MoveCost,
        Stats.AttackCost,
        Stats.TurnDelay);

    return FText::FromString(StatsString);
}

FText BuildAbilityTooltipText(const ARoundAbilityActor* Ability)
{
    if (!Ability)
    {
        return FText::GetEmpty();
    }

    const FString AbilityName = (Ability->AbilityData && !Ability->AbilityData->DisplayName.IsEmpty())
        ? Ability->AbilityData->DisplayName.ToString()
        : Ability->RowName.ToString();
    const FString Description = Ability->AbilityData ? Ability->AbilityData->Description.ToString() : FString();
    const FString Tooltip = FString::Printf(
        TEXT("%s\n%s\nCost: %d  Range: %d  Cooldown: %d  Current CD: %d"),
        *AbilityName,
        *Description,
        Ability->GetAbilityCost(),
        Ability->GetAbilityRange(),
        Ability->GetAbilityCooldown(),
        Ability->CurrentCooldown);

    return FText::FromString(Tooltip);
}

template <typename TWidgetType>
TWidgetType* FindWidgetByPartialName(const UWidgetTree* Tree, const FString& Needle)
{
    if (!Tree)
    {
        return nullptr;
    }

    TArray<UWidget*> Widgets;
    Tree->GetAllWidgets(Widgets);
    for (UWidget* Widget : Widgets)
    {
        if (TWidgetType* TypedWidget = Cast<TWidgetType>(Widget))
        {
            const FString WidgetName = TypedWidget->GetName();
            if (WidgetName.Contains(Needle, ESearchCase::IgnoreCase))
            {
                return TypedWidget;
            }
        }
    }

    return nullptr;
}

FString FormatSkillHoverNumericValue(const double Value)
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

FString FormatPercentValue(const double Fraction)
{
    return FString::FromInt(FMath::RoundToInt(Fraction * 100.0));
}

FString FormatSkillHoverSkillpointDescription(const FRoundSkillpointDetails& Level)
{
    FString Result = Level.Description;
    for (const TPair<FString, double>& Pair : Level.Values)
    {
        const FString BraceToken = FString::Printf(TEXT("{%s}"), *Pair.Key);
        const FString NegativeBraceToken = FString::Printf(TEXT("{-%s}"), *Pair.Key);
        const FString PercentToken = FString::Printf(TEXT("%%%s%%"), *Pair.Key);
        const FString FormattedValue = FormatSkillHoverNumericValue(Pair.Value);
        const FString InvertedFormattedValue = FormatSkillHoverNumericValue(-Pair.Value);
        Result.ReplaceInline(*BraceToken, *FormattedValue, ESearchCase::IgnoreCase);
        Result.ReplaceInline(*NegativeBraceToken, *InvertedFormattedValue, ESearchCase::IgnoreCase);
        Result.ReplaceInline(*PercentToken, *FormattedValue, ESearchCase::IgnoreCase);
    }

    return Result;
}

FText BuildSelectedAbilitySkillDetailsText(const int64 PackedSelections, const TArray<FRoundSkillpointDetails>& Levels)
{
    TArray<FString> Lines;
    for (int32 TierIndex = 0; TierIndex < 4; ++TierIndex)
    {
        const int32 Selection = static_cast<int32>((PackedSelections >> (TierIndex * 2)) & 0x3);
        if (Selection <= 0)
        {
            break;
        }

        const int32 RowIndex = (TierIndex * 3) + (Selection - 1);
        if (!Levels.IsValidIndex(RowIndex))
        {
            continue;
        }

        FString Description = FormatSkillHoverSkillpointDescription(Levels[RowIndex]);
        if (Description.IsEmpty())
        {
            Description = Levels[RowIndex].Title.ToString();
        }
        if (!Description.IsEmpty())
        {
            Lines.Add(Description);
        }
    }

    if (Lines.Num() == 0)
    {
        return FText::FromString(TEXT("Keine Punkte geskillt."));
    }

    return FText::FromString(FString::Join(Lines, TEXT("\n")));
}

void AssignSkillListEntryClass(UListView* ListView, UClass* EntryClass)
{
    if (!ListView || !EntryClass)
    {
        return;
    }

    if (ListView->GetEntryWidgetClass() == EntryClass)
    {
        return;
    }

    if (FClassProperty* EntryClassProperty = FindFProperty<FClassProperty>(UListViewBase::StaticClass(), TEXT("EntryWidgetClass")))
    {
        EntryClassProperty->SetPropertyValue_InContainer(ListView, EntryClass);
        ListView->RegenerateAllEntries();
    }
}

FText BuildTerrainNameText(const ARoundGridTile* Tile)
{
    if (!Tile)
    {
        return FText::GetEmpty();
    }

    const ARoundTerrainActor* TerrainActor = Cast<ARoundTerrainActor>(Tile->TerrainObject);
    const ERoundTerrainType TerrainType = TerrainActor ? TerrainActor->TerrainType : Tile->TerrainType;
    if (TerrainActor)
    {
        const FText DisplayName = TerrainActor->GetTerrainDisplayNameText();
        if (!DisplayName.IsEmpty())
        {
            return DisplayName;
        }
    }
    else if (const ARoundGameState* RoundGameState = Tile->GetWorld() ? Tile->GetWorld()->GetGameState<ARoundGameState>() : nullptr)
    {
        if (const URoundTerrainData* TerrainData = RoundGameState->GetTerrainData(TerrainType))
        {
            if (!TerrainData->DisplayName.IsEmpty())
            {
                return TerrainData->DisplayName;
            }
        }
    }

    switch (TerrainType)
    {
    case ERoundTerrainType::Ground:
        return FText::FromString(TEXT("Ground"));
    case ERoundTerrainType::Meadow:
        return FText::FromString(TEXT("Wiese"));
    case ERoundTerrainType::Street:
        return FText::FromString(TEXT("Street"));
    case ERoundTerrainType::Rubble:
        return FText::FromString(TEXT("Rubble"));
    case ERoundTerrainType::Forest:
        return FText::FromString(TEXT("Forest"));
    case ERoundTerrainType::Thorns:
        return FText::FromString(TEXT("Thorns"));
    case ERoundTerrainType::Grove:
        return FText::FromString(TEXT("Grove"));
    case ERoundTerrainType::Fire:
        return FText::FromString(TEXT("Fire"));
    case ERoundTerrainType::Rocks:
        return FText::FromString(TEXT("Rocks"));
    case ERoundTerrainType::Wall:
        return FText::FromString(TEXT("Wall"));
    case ERoundTerrainType::Mountain:
        return FText::FromString(TEXT("Mountain"));
    default:
        return FText::FromString(TEXT("Field"));
    }
}

FText BuildTerrainDescriptionText(const ARoundGridTile* Tile)
{
    if (!Tile)
    {
        return FText::GetEmpty();
    }

    const ARoundTerrainActor* TerrainActor = Cast<ARoundTerrainActor>(Tile->TerrainObject);
    const ERoundTerrainType TerrainType = TerrainActor ? TerrainActor->TerrainType : Tile->TerrainType;
    if (TerrainActor)
    {
        const FText Description = TerrainActor->GetTerrainDescriptionText();
        if (!Description.IsEmpty())
        {
            return Description;
        }
    }
    else if (const ARoundGameState* RoundGameState = Tile->GetWorld() ? Tile->GetWorld()->GetGameState<ARoundGameState>() : nullptr)
    {
        if (const URoundTerrainData* TerrainData = RoundGameState->GetTerrainData(TerrainType))
        {
            if (!TerrainData->Description.IsEmpty())
            {
                return TerrainData->Description;
            }
        }
    }

    switch (TerrainType)
    {
    case ERoundTerrainType::Ground:
        return FText::FromString(TEXT("Normales Feld."));
    case ERoundTerrainType::Meadow:
        return FText::FromString(TEXT("Wiese: Begehbarer Untergrund ohne besonderen Effekt."));
    case ERoundTerrainType::Street:
        return FText::FromString(TEXT("Strasse: Bewegung ist hier guenstiger."));
    case ERoundTerrainType::Rubble:
        return FText::FromString(TEXT("Geruempel: Bewegung kostet hier mehr."));
    case ERoundTerrainType::Forest:
        return FText::FromString(TEXT("Wald: Reduziert Fernkampfschaden gegen Ziele im Wald."));
    case ERoundTerrainType::Thorns:
        return FText::FromString(TEXT("Dornen: Verursacht Schaden beim Betreten und erhoeht die Bewegungskosten."));
    case ERoundTerrainType::Grove:
        return FText::FromString(TEXT("Grove: Reduziert den erlittenen Schaden fuer alle auf dem Feld."));
    case ERoundTerrainType::Fire:
        return FText::FromString(TEXT("Feuer: Verursacht beim Zugstart 100 Feuerschaden."));
    case ERoundTerrainType::Rocks:
        return FText::FromString(TEXT("Felsen: Unbegehbar."));
    case ERoundTerrainType::Wall:
        return FText::FromString(TEXT("Wand: Unbegehbar."));
    case ERoundTerrainType::Mountain:
        return FText::FromString(TEXT("Gebirge: Unbegehbar und sichtblockierend."));
    default:
        break;
    }

    if (TerrainActor)
    {
        const bool bMoveable = TerrainActor->IsMoveable();
        const int32 MoveCost = TerrainActor->GetMoveCost(Tile->MoveCost);
        return FText::FromString(FString::Printf(
            TEXT("Feld: MoveCost %d, %s."),
            MoveCost,
            bMoveable ? TEXT("begehbar") : TEXT("unbegehbar")));
    }

    return FText::GetEmpty();
}

}

void URoundGameUIWidget::NativeConstruct()
{
    Super::NativeConstruct();
    EnsureInteractWidgets();
    BindButtons();
    RefreshFromPlayer();
}

void URoundGameUIWidget::EnsureInteractWidgets()
{
    if (Button_Interact && Interact)
    {
        return;
    }

    UVerticalBox* ParentBox = VerticalBox_Buttons.Get();
    if (!ParentBox)
    {
        if (UWidget* ExactWidget = GetWidgetFromName(TEXT("VerticalBox_Buttons")))
        {
            ParentBox = Cast<UVerticalBox>(ExactWidget);
        }

        if (!ParentBox && WidgetTree)
        {
            ParentBox = FindWidgetByPartialName<UVerticalBox>(WidgetTree, TEXT("Buttons"));
        }
    }

    if (!ParentBox || !WidgetTree)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureInteractWidgets failed - ParentBox=%s WidgetTree=%s"),
            *GetNameSafe(ParentBox),
            WidgetTree ? TEXT("Yes") : TEXT("No"));
        return;
    }

    if (!Button_Interact)
    {
        Button_Interact = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Button_Interact_Runtime"));
    }

    if (!Interact)
    {
        Interact = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Interact_Runtime"));
    }

    if (!Button_Interact || !Interact)
    {
        UE_LOG(LogTemp, Warning, TEXT("EnsureInteractWidgets failed to construct runtime interact widgets."));
        return;
    }

    Interact->SetText(FText::FromString(TEXT("Channel")));
    Interact->SetJustification(ETextJustify::Center);

    if (Button_Interact->GetChildrenCount() == 0)
    {
        Button_Interact->AddChild(Interact);
    }

    if (Button_Interact->GetParent() != ParentBox)
    {
        ParentBox->AddChildToVerticalBox(Button_Interact);
    }

    UE_LOG(LogTemp, Warning, TEXT("EnsureInteractWidgets created runtime fallback interact button under %s"), *GetNameSafe(ParentBox));
}

void URoundGameUIWidget::SetPlayerPawn(ARoundPlayerPawn* InPlayer)
{
    Player = InPlayer;
    RefreshFromPlayer();
}

void URoundGameUIWidget::RefreshFromPlayer()
{
    if (Player)
    {
        BindToGameState(Player->GetRoundGameState());
    }

    RefreshCharacterUI();
    RefreshButtons();
    RefreshEffects();
    RefreshAuras();
    RefreshSkills();
    RefreshCustomUI();
    RefreshInitiativeBar();
    BP_OnUIRefreshed();
}

void URoundGameUIWidget::RefreshCharacterUI()
{
    ARoundCharacter* Character = GetDisplayedCharacter();
    const URoundAttributeSetComponent* Attributes = Character ? Character->Attributes : nullptr;
    const bool bOwnTeamCharacter = Character && Player && Character->Team == Player->Team;
    const bool bCanControlCharacter = Character && Player && Player->CanControlCharacter(Character);
    const bool bHasCharacter = Character != nullptr;
    const bool bIsPuppetCharacter = IsPuppetCharacterForUI(Character);

    if (VerticalBox_Actions)
    {
        VerticalBox_Actions->SetVisibility(bHasCharacter ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    }

    if (VerticalBox_Buttons)
    {
        const bool bShowButtons = bOwnTeamCharacter && bCanControlCharacter && !bIsPuppetCharacter;
        VerticalBox_Buttons->SetVisibility(bShowButtons ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
    }

    UpdateProgressAndText(
        TEXT("Health_Bar"),
        TEXT("TextBlock_Health"),
        Attributes ? Attributes->CurrentHealth : 0,
        Attributes ? Attributes->BaseStats.MaxHealth : 0);

    const int32 DisplayedInitiative = Attributes ? Attributes->CurrentInitiative : 0;
    UpdateProgressAndText(
        TEXT("InitativeBar"),
        TEXT("TextBlock_Initative"),
        DisplayedInitiative,
        Attributes ? Attributes->BaseStats.MaxInitiative : 0);

    if (InitativeBar)
    {
        InitativeBar->SetVisibility(bIsPuppetCharacter ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    if (TextBlock_Initative)
    {
        TextBlock_Initative->SetVisibility(bIsPuppetCharacter ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    RefreshInfoPanel();

    if (TextBlock_Info_Objective)
    {
        TextBlock_Info_Objective->SetText(BuildObjectiveInfoText());
        TextBlock_Info_Objective->SetToolTipText(BuildObjectiveInfoTooltipText());
    }

    if (TextBlock_Info_Field)
    {
        TextBlock_Info_Field->SetText(BuildFieldInfoText());
        TextBlock_Info_Field->SetToolTipText(BuildFieldInfoTooltipText());
    }

    RefreshEffects();
    RefreshAuras();
    RefreshSkills();
    RefreshCustomUI();
}

void URoundGameUIWidget::RefreshButtons()
{
    if (BoundGameState.IsValid() && BoundGameState->IsReplayActive())
    {
        UpdateActionButton(TEXT("Button_Move"), TEXT("TextBlock_174"), false, FText::FromString(TEXT("Move")));
        UpdateActionButton(TEXT("Button_Attack"), TEXT("TextBlock_234"), false, FText::FromString(TEXT("Attack")));
        UpdateActionButton(TEXT("Button_EndTurn"), TEXT("TextBlock"), false, FText::FromString(TEXT("End Turn")));

        if (UButton* InteractButton = ResolveButton(TEXT("Button_Interact")))
        {
            InteractButton->SetVisibility(ESlateVisibility::Collapsed);
            InteractButton->SetIsEnabled(false);
            InteractButton->SetToolTipText(FText::GetEmpty());
        }
        if (UTextBlock* InteractLabel = ResolveTextBlock(TEXT("Interact")))
        {
            InteractLabel->SetText(FText::FromString(TEXT("Channel")));
            InteractLabel->SetToolTipText(FText::GetEmpty());
        }

        for (int32 AbilityIndex = 0; AbilityIndex < 7; ++AbilityIndex)
        {
            UpdateAbilityButton(AbilityIndex);
        }
        return;
    }

    ARoundCharacter* Character = GetDisplayedCharacter();
    const bool bHasCharacter = Character != nullptr && Player && Player->CanControlCharacter(Character);

    UpdateActionButton(TEXT("Button_Move"), TEXT("TextBlock_174"), bHasCharacter, FText::FromString(TEXT("Move")));
    UpdateActionButton(TEXT("Button_Attack"), TEXT("TextBlock_234"), bHasCharacter, FText::FromString(TEXT("Attack")));
    UpdateActionButton(TEXT("Button_EndTurn"), TEXT("TextBlock"), bHasCharacter, FText::FromString(TEXT("End Turn")));

    FString InteractReason;
    const bool bCanClaimObjective = bHasCharacter && BoundGameState.IsValid() && BoundGameState->CanCharacterStartObjectiveClaim(Character, InteractReason);
    UButton* InteractButton = ResolveButton(TEXT("Button_Interact"));
    UTextBlock* InteractLabel = ResolveTextBlock(TEXT("Interact"));

    UE_LOG(
        LogTemp,
        Log,
        TEXT("UI RefreshButtons: Character=%s Tile=%s HasCharacter=%d CanControl=%d BoundGameState=%d CanClaim=%d Reason='%s' InteractButton=%s InteractLabel=%s"),
        *GetNameSafe(Character),
        *GetNameSafe(Character ? Character->CurrentTile.Get() : nullptr),
        bHasCharacter ? 1 : 0,
        (Player && Character && Player->CanControlCharacter(Character)) ? 1 : 0,
        BoundGameState.IsValid() ? 1 : 0,
        bCanClaimObjective ? 1 : 0,
        *InteractReason,
        *GetNameSafe(InteractButton),
        *GetNameSafe(InteractLabel));

    if (InteractButton)
    {
        InteractButton->SetVisibility(bCanClaimObjective ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        InteractButton->SetIsEnabled(bCanClaimObjective);
        InteractButton->SetToolTipText(FText::FromString(
            bCanClaimObjective
                ? TEXT("Channel this objective. The action ends the turn and blocks new turns until the claim completes or is interrupted.")
                : InteractReason));
        UE_LOG(LogTemp, Log, TEXT("UI RefreshButtons InteractButton: Visibility=%d Enabled=%d Tooltip='%s'"),
            static_cast<int32>(InteractButton->GetVisibility()),
            InteractButton->GetIsEnabled() ? 1 : 0,
            *InteractButton->GetToolTipText().ToString());
    }
    if (InteractLabel)
    {
        InteractLabel->SetVisibility(bCanClaimObjective ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        InteractLabel->SetText(FText::FromString(TEXT("Channel")));
        InteractLabel->SetToolTipText(FText::FromString(
            bCanClaimObjective
                ? TEXT("Channel this objective. The action ends the turn and blocks new turns until the claim completes or is interrupted.")
                : InteractReason));
        UE_LOG(LogTemp, Log, TEXT("UI RefreshButtons InteractLabel: Text='%s' Tooltip='%s'"),
            *InteractLabel->GetText().ToString(),
            *InteractLabel->GetToolTipText().ToString());
    }

    for (int32 AbilityIndex = 0; AbilityIndex < 7; ++AbilityIndex)
    {
        UpdateAbilityButton(AbilityIndex);
    }
}

ARoundCharacter* URoundGameUIWidget::GetDisplayedCharacter() const
{
    return Player ? Player->SelectedCharacter() : nullptr;
}

ARoundAbilityActor* URoundGameUIWidget::GetDisplayedAbility(const int32 AbilityIndex) const
{
    ARoundCharacter* Character = GetDisplayedCharacter();
    if (!Character || !Character->GrantedAbilities.IsValidIndex(AbilityIndex))
    {
        return nullptr;
    }

    return Character->GrantedAbilities[AbilityIndex];
}

void URoundGameUIWidget::OnMoveClicked()
{
    if (Player)
    {
        Player->Move();
        RefreshButtons();
    }
}

void URoundGameUIWidget::OnAttackClicked()
{
    if (Player)
    {
        Player->Attack();
        RefreshButtons();
    }
}

void URoundGameUIWidget::OnEndTurnClicked()
{
    if (Player)
    {
        Player->EndTurn();
        RefreshFromPlayer();
    }
}

void URoundGameUIWidget::OnInteractClicked()
{
    if (Player)
    {
        Player->ObjectiveClaim();
        RefreshFromPlayer();
    }
}

void URoundGameUIWidget::OnAbilityClicked(const int32 AbilityIndex)
{
    if (!Player)
    {
        return;
    }

    if (ARoundAbilityActor* Ability = GetDisplayedAbility(AbilityIndex))
    {
        if (Ability->AbilityData && !Ability->AbilityData->bRequiresTargetTile && !Ability->AbilityData->bRequiresTargetCharacter)
        {
            if (Ability->RequiresHandCardSelection())
            {
                if (URoundChampionCustomUIWidget* ChampionWidget = Cast<URoundChampionCustomUIWidget>(CurrentCustomUIWidget))
                {
                    ChampionWidget->BeginAbilityHandCardSelection(AbilityIndex, Ability);
                    RefreshCharacterUI();
                    RefreshButtons();
                }
                return;
            }

            Player->UseAbilityDirect(AbilityIndex);
            RefreshFromPlayer();
            return;
        }
    }

    Player->AbilitySelected(AbilityIndex);
    RefreshButtons();
}

void URoundGameUIWidget::HasWon()
{
    if (Player)
    {
        Player->TeamWin();
    }
}

void URoundGameUIWidget::SetHoverInfoOverride(ARoundCharacter* SourceCharacter, const FText& MainText, const FText& SubText)
{
    HoverInfoCharacter = SourceCharacter;
    HoverInfoMainText = MainText;
    HoverInfoSubText = SubText;
    bHasHoverInfoOverride = true;
    RefreshInfoPanel();
}

void URoundGameUIWidget::ClearHoverInfoOverride(ARoundCharacter* SourceCharacter)
{
    if (!bHasHoverInfoOverride)
    {
        return;
    }

    if (SourceCharacter && HoverInfoCharacter.IsValid() && HoverInfoCharacter.Get() != SourceCharacter)
    {
        return;
    }

    bHasHoverInfoOverride = false;
    HoverInfoCharacter = nullptr;
    HoverInfoMainText = FText::GetEmpty();
    HoverInfoSubText = FText::GetEmpty();
    RefreshInfoPanel();
}

void URoundGameUIWidget::UpdateAbilityButton(const int32 AbilityIndex)
{
    UButton* Button = GetAbilityButtonByIndex(AbilityIndex);
    UTextBlock* Label = GetAbilityLabelByIndex(AbilityIndex);
    if (!Button || !Label)
    {
        return;
    }

    ARoundCharacter* Character = GetDisplayedCharacter();
    if (!Character || !Player || !Player->CanControlCharacter(Character))
    {
        Button->SetIsEnabled(false);
        Button->SetVisibility(ESlateVisibility::Collapsed);
        Label->SetText(FText::GetEmpty());
        return;
    }

    if (ARoundAbilityActor* Ability = GetDisplayedAbility(AbilityIndex))
    {
        const ERoundButtonState VisibilityState = Ability->GetAbilityVisibility();
        const bool bEnabled = VisibilityState == ERoundButtonState::Enabled;
        Button->SetIsEnabled(bEnabled);
        Button->SetVisibility(VisibilityState == ERoundButtonState::Hidden ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

        FString AbilityLabel = Ability->AbilityData && !Ability->AbilityData->DisplayName.IsEmpty()
            ? Ability->AbilityData->DisplayName.ToString()
            : Ability->RowName.ToString();
        if (Ability->CurrentCooldown > 0)
        {
            AbilityLabel = FString::Printf(TEXT("%s [%d]"), *AbilityLabel, Ability->CurrentCooldown);
        }

        if (Ability->AbilityData)
        {
            Label->SetText(FText::FromString(AbilityLabel));
        }
        else
        {
            Label->SetText(FText::FromString(AbilityLabel));
        }

        const FText TooltipText = BuildAbilityTooltipText(Ability);
        Button->SetToolTipText(TooltipText);
        Label->SetToolTipText(TooltipText);
    }
    else
    {
        Button->SetIsEnabled(false);
        Button->SetVisibility(ESlateVisibility::Collapsed);
        Label->SetText(FText::GetEmpty());
        Button->SetToolTipText(FText::GetEmpty());
        Label->SetToolTipText(FText::GetEmpty());
    }
}

void URoundGameUIWidget::UpdateActionButton(
    const TCHAR* ButtonName,
    const TCHAR* LabelName,
    const bool bEnabled,
    const FText& LabelText)
{
    if (UButton* Button = ResolveButton(ButtonName))
    {
        Button->SetIsEnabled(bEnabled);
        Button->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (UTextBlock* Label = ResolveTextBlock(LabelName))
    {
        Label->SetVisibility(bEnabled ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        Label->SetText(LabelText);
    }
}

void URoundGameUIWidget::UpdateProgressAndText(
    const TCHAR* ProgressBarName,
    const TCHAR* TextName,
    const int32 CurrentValue,
    const int32 MaxValue) const
{
    if (UProgressBar* ProgressBar = ResolveProgressBar(ProgressBarName))
    {
        ProgressBar->SetPercent(MaxValue > 0 ? static_cast<float>(CurrentValue) / static_cast<float>(MaxValue) : 0.f);
    }

    if (UTextBlock* TextBlockWidget = ResolveTextBlock(TextName))
    {
        TextBlockWidget->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentValue, MaxValue)));
    }
}

void URoundGameUIWidget::BindToGameState(ARoundGameState* RoundGameState)
{
    if (BoundGameState.Get() == RoundGameState || !RoundGameState)
    {
        return;
    }

    if (BoundGameState.IsValid())
    {
        BoundGameState->OnBoardStateChanged.RemoveDynamic(this, &URoundGameUIWidget::HandleBoardStateChanged);
        BoundGameState->OnTurnChanged.RemoveDynamic(this, &URoundGameUIWidget::HandleTurnChanged);
    }

    BoundGameState = RoundGameState;
    RoundGameState->OnBoardStateChanged.AddDynamic(this, &URoundGameUIWidget::HandleBoardStateChanged);
    RoundGameState->OnTurnChanged.AddDynamic(this, &URoundGameUIWidget::HandleTurnChanged);
}

void URoundGameUIWidget::HandleBoardStateChanged()
{
    RefreshFromPlayer();
}

void URoundGameUIWidget::HandleTurnChanged(ARoundCharacter* ActiveCharacter)
{
    RefreshFromPlayer();
}

void URoundGameUIWidget::RefreshInitiativeBar()
{
    if (!BP_InitativeBar)
    {
        return;
    }

    ARoundGameState* RoundGameState = Player ? Player->GetRoundGameState() : nullptr;
    if (!RoundGameState)
    {
        return;
    }

    TArray<ARoundCharacter*> InitiativeCharacters;
    for (ARoundCharacter* Character : RoundGameState->RegisteredCharacters)
    {
        if (Character && Character->IsAlive() && Player->CanSeeCharacter(Character) && !IsPuppetCharacterForUI(Character))
        {
            InitiativeCharacters.Add(Character);
        }
    }

    InitiativeCharacters.Sort([](const ARoundCharacter& A, const ARoundCharacter& B)
    {
        const int32 MaxA = A.Attributes ? A.Attributes->BaseStats.MaxInitiative : 100;
        const int32 MaxB = B.Attributes ? B.Attributes->BaseStats.MaxInitiative : 100;
        const int32 DistanceA = FMath::Max(0, MaxA - A.InitiativeToTurn());
        const int32 DistanceB = FMath::Max(0, MaxB - B.InitiativeToTurn());
        return DistanceA < DistanceB;
    });

    static TSubclassOf<URoundInitiativeIconWidget> InitiativeIconClass = nullptr;
    if (!InitiativeIconClass)
    {
        InitiativeIconClass = Cast<UClass>(RoundGameDataRegistryLookup::ResolveInitiativeIconWidgetClass().Get());
    }

    if (!InitiativeIconClass)
    {
        InitiativeIconClass = URoundInitiativeIconWidget::StaticClass();
    }

    APlayerController* OwningController = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;

    BP_InitativeBar->SyncIcons(InitiativeCharacters, InitiativeIconClass, OwningController);
    BP_InitativeBar->SyncObjectiveMarkers(
        RoundGameState->GetObjectivePreviewEntries(),
        RoundGameState->GetRecordedGlobalInitiativeElapsed());
}

bool URoundGameUIWidget::IsObjectiveClaimButtonIndex(const int32 AbilityIndex) const
{
    return false;
}

int32 URoundGameUIWidget::GetObjectiveClaimButtonIndex() const
{
    return INDEX_NONE;
}

FText URoundGameUIWidget::BuildObjectiveInfoText() const
{
    if (!Player || !BoundGameState.IsValid())
    {
        return FText::GetEmpty();
    }

    ARoundGridTile* SelectedTile = Player->SelectedField;
    if (!SelectedTile)
    {
        return FText::GetEmpty();
    }

    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<ARoundObjectiveActor> It(World); It; ++It)
        {
            ARoundObjectiveActor* Objective = *It;
            if (!Objective || !(Objective->CenterTile == SelectedTile || Objective->ObjectiveTiles.Contains(SelectedTile)))
            {
                continue;
            }
            switch (Objective->ObjectiveType)
            {
            case ERoundObjectiveType::HoldZone:
                return FText::FromString(TEXT("Hold"));
            case ERoundObjectiveType::DestroyNode:
                return FText::FromString(TEXT("Destroy"));
            case ERoundObjectiveType::Channel:
                return FText::FromString(TEXT("Channel"));
            default:
                return FText::FromString(TEXT("Objective"));
            }
        }
    }

    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<ARoundObjectiveRewardActor> It(World); It; ++It)
        {
            ARoundObjectiveRewardActor* RewardActor = *It;
            if (RewardActor && RewardActor->RewardTile == SelectedTile)
            {
                return FText::FromString(FString::Printf(
                    TEXT("Reward on tile: %s. Step onto the tile to collect it."),
                    RewardActor->RewardDisplayName.IsEmpty() ? TEXT("Reward") : *RewardActor->RewardDisplayName));
            }
        }
    }

    return FText::GetEmpty();
}

FText URoundGameUIWidget::BuildObjectiveInfoTooltipText() const
{
    if (!Player || !BoundGameState.IsValid())
    {
        return FText::GetEmpty();
    }

    ARoundGridTile* SelectedTile = Player->SelectedField;
    if (!SelectedTile)
    {
        return FText::GetEmpty();
    }

    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<ARoundObjectiveActor> It(World); It; ++It)
        {
            ARoundObjectiveActor* Objective = *It;
            if (!Objective || !(Objective->CenterTile == SelectedTile || Objective->ObjectiveTiles.Contains(SelectedTile)))
            {
                continue;
            }

            const FString RewardLabel = Objective->RewardDisplayName.IsEmpty() ? TEXT("Reward") : Objective->RewardDisplayName;
            switch (Objective->ObjectiveType)
            {
            case ERoundObjectiveType::HoldZone:
                if (Objective->IsPreviewing())
                {
                    return FText::FromString(FString::Printf(
                        TEXT("Will become a Hold objective in %d initiative. Reward: %s."),
                        FMath::Max(0, Objective->SpawnAtGlobalInitiative - BoundGameState->GetRecordedGlobalInitiativeElapsed()),
                        *RewardLabel));
                }
                return FText::FromString(FString::Printf(
                    TEXT("Keep all enemies out of the 7-tile zone for %d initiative. Progress: %d/%d. Reward: %s."),
                    Objective->RequiredProgressInitiative,
                    Objective->CurrentProgressInitiative,
                    Objective->RequiredProgressInitiative,
                    *RewardLabel));
            case ERoundObjectiveType::DestroyNode:
                if (Objective->IsPreviewing())
                {
                    return FText::FromString(FString::Printf(
                        TEXT("Will become a Destroy objective in %d initiative. Reward: %s."),
                        FMath::Max(0, Objective->SpawnAtGlobalInitiative - BoundGameState->GetRecordedGlobalInitiativeElapsed()),
                        *RewardLabel));
                }
                return FText::FromString(FString::Printf(
                    TEXT("Destroy the node to drop the reward. HP: %d/%d. Reward: %s."),
                    Objective->CurrentHealth,
                    Objective->MaxHealth,
                    *RewardLabel));
            case ERoundObjectiveType::Channel:
                if (Objective->IsPreviewing())
                {
                    return FText::FromString(FString::Printf(
                        TEXT("Will become a Channel objective in %d initiative. Reward: %s."),
                        FMath::Max(0, Objective->SpawnAtGlobalInitiative - BoundGameState->GetRecordedGlobalInitiativeElapsed()),
                        *RewardLabel));
                }
                return FText::FromString(FString::Printf(
                    TEXT("Use Claim Objective on the center tile. The channel ends the turn and blocks new turns until complete. Reward: %s."),
                    *RewardLabel));
            default:
                break;
            }
        }

        for (TActorIterator<ARoundObjectiveRewardActor> It(World); It; ++It)
        {
            ARoundObjectiveRewardActor* RewardActor = *It;
            if (RewardActor && RewardActor->RewardTile == SelectedTile)
            {
                return FText::FromString(FString::Printf(
                    TEXT("Reward on tile. Move onto it to collect: %s."),
                    RewardActor->RewardDisplayName.IsEmpty() ? TEXT("Reward") : *RewardActor->RewardDisplayName));
            }
        }
    }

    return FText::GetEmpty();
}

FText URoundGameUIWidget::BuildFieldInfoText() const
{
    if (!Player)
    {
        return FText::GetEmpty();
    }

    const ARoundGridTile* SelectedTile = Player->SelectedField;
    if (!SelectedTile)
    {
        return FText::GetEmpty();
    }

    return BuildTerrainNameText(SelectedTile);
}

FText URoundGameUIWidget::BuildFieldInfoTooltipText() const
{
    if (!Player)
    {
        return FText::GetEmpty();
    }

    const ARoundGridTile* SelectedTile = Player->SelectedField;
    if (!SelectedTile)
    {
        return FText::GetEmpty();
    }

    const ARoundTerrainActor* TerrainActor = Cast<ARoundTerrainActor>(SelectedTile->TerrainObject);
    const FString Description = BuildTerrainDescriptionText(SelectedTile).ToString();
    if (TerrainActor && TerrainActor->IsDestructible())
    {
        const FString HealthText = FString::Printf(TEXT("HP: %d/%d"), TerrainActor->CurrentHealth, TerrainActor->MaxHealth);
        if (!Description.IsEmpty())
        {
            return FText::FromString(FString::Printf(TEXT("%s\n%s"), *Description, *HealthText));
        }

        return FText::FromString(HealthText);
    }

    return FText::FromString(Description);
}

void URoundGameUIWidget::RefreshEffects()
{
    if (!ListView_0)
    {
        return;
    }

    ListView_0->ClearListItems();

    ARoundCharacter* Character = GetDisplayedCharacter();
    if (!Character)
    {
        return;
    }

    for (ARoundEffectActor* Effect : Character->ActiveEffects)
    {
        if (IsValid(Effect)
            && !Effect->IsA(ARoundAuraEffect::StaticClass())
            && !Effect->DisplayName.IsEmpty())
        {
            ListView_0->AddItem(Effect);
        }
    }
}

void URoundGameUIWidget::RefreshAuras()
{
    if (!ListView_Auras && WidgetTree)
    {
        ListView_Auras = Cast<UListView>(WidgetTree->FindWidget(TEXT("ListView_Auras")));
        if (!ListView_Auras)
        {
            ListView_Auras = FindWidgetByPartialName<UListView>(WidgetTree, TEXT("Auras"));
        }
    }

    if (!ListView_Auras)
    {
        return;
    }

    TSubclassOf<UUserWidget> DesiredAuraEntryClass = AuraListEntryWidgetClass;
    if (!DesiredAuraEntryClass && ListView_Auras->GetEntryWidgetClass()
        && ListView_Auras->GetEntryWidgetClass()->IsChildOf(URoundAuraEntryWidget::StaticClass()))
    {
        DesiredAuraEntryClass = ListView_Auras->GetEntryWidgetClass();
    }
    if (!DesiredAuraEntryClass)
    {
        if (UClass* BlueprintAuraEntryClass = RoundGameDataRegistryLookup::ResolveAuraEntryWidgetClass().Get())
        {
            if (BlueprintAuraEntryClass->IsChildOf(URoundAuraEntryWidget::StaticClass()))
            {
                DesiredAuraEntryClass = BlueprintAuraEntryClass;
            }
        }
    }
    if (!DesiredAuraEntryClass)
    {
        DesiredAuraEntryClass = TSubclassOf<UUserWidget>(URoundAuraEntryWidget::StaticClass());
    }

    AssignSkillListEntryClass(ListView_Auras, DesiredAuraEntryClass.Get());
    AuraListItems.Reset();
    ListView_Auras->ClearListItems();

    ARoundCharacter* Character = GetDisplayedCharacter();
    if (!Character || !BoundGameState.IsValid())
    {
        ListView_Auras->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    if (!Character->GetWorld())
    {
        ListView_Auras->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    for (TActorIterator<ARoundCharacter> It(Character->GetWorld()); It; ++It)
    {
        ARoundCharacter* AuraOwner = *It;
        if (!AuraOwner)
        {
            continue;
        }

        for (ARoundEffectActor* Effect : AuraOwner->ActiveEffects)
        {
            ARoundAuraEffect* AuraEffect = Cast<ARoundAuraEffect>(Effect);
            if (!AuraEffect || !AuraEffect->AffectsCharacter(Character))
            {
                continue;
            }

            URoundAuraListItemData* Item = NewObject<URoundAuraListItemData>(this);
            if (!Item)
            {
                continue;
            }

            Item->DisplayName = FText::FromString(AuraEffect->DisplayName.IsEmpty() ? TEXT("Aura") : AuraEffect->DisplayName);
            double DamageReductionFraction = 0.0;
            if (AuraEffect->TryGetIncomingDamageReductionFraction(DamageReductionFraction))
            {
                Item->Description = FText::FromString(FString::Printf(
                    TEXT("Nimmt %s%% weniger Schaden."),
                    *FormatPercentValue(DamageReductionFraction)));
            }
            else
            {
                Item->Description = FText::FromString(AuraEffect->Description);
            }
            Item->RemainingInitiative = AuraEffect->RemainingInitiative;
            Item->AuraInstanceId = AuraEffect->GetFName().ToString();
            Item->SourceCharacterId = AuraOwner->GetFName().ToString();
            AuraListItems.Add(Item);
        }
    }

    if (AuraListItems.Num() <= 0)
    {
        ListView_Auras->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    TArray<UObject*> Items;
    Items.Reserve(AuraListItems.Num());
    for (URoundAuraListItemData* Item : AuraListItems)
    {
        if (Item)
        {
            Items.Add(Item);
        }
    }

    ListView_Auras->SetVisibility(ESlateVisibility::Visible);
    ListView_Auras->SetListItems(Items);
}

void URoundGameUIWidget::RefreshSkills()
{
    if (!ListView_Skills && WidgetTree)
    {
        ListView_Skills = Cast<UListView>(WidgetTree->FindWidget(TEXT("ListView_Skills")));
        if (!ListView_Skills)
        {
            ListView_Skills = FindWidgetByPartialName<UListView>(WidgetTree, TEXT("Skills"));
        }
    }

    if (!ListView_Skills)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("RefreshSkills ListView=%s CurrentEntryClass=%s"),
        *GetNameSafe(ListView_Skills),
        *GetNameSafe(ListView_Skills->GetEntryWidgetClass()));

    TSubclassOf<UUserWidget> DesiredSkillEntryClass = SkillListEntryWidgetClass;
    if (!DesiredSkillEntryClass && ListView_Skills->GetEntryWidgetClass())
    {
        DesiredSkillEntryClass = ListView_Skills->GetEntryWidgetClass();
    }
    if (!DesiredSkillEntryClass)
    {
        DesiredSkillEntryClass = TSubclassOf<UUserWidget>(URoundSkillHoverListEntryWidget::StaticClass());
    }

    AssignSkillListEntryClass(ListView_Skills, DesiredSkillEntryClass.Get());
    UE_LOG(LogTemp, Log, TEXT("RefreshSkills AssignedEntryClass=%s"),
        *GetNameSafe(ListView_Skills->GetEntryWidgetClass()));
    SkillListItems.Reset();
    ListView_Skills->ClearListItems();

    ARoundCharacter* Character = GetDisplayedCharacter();
    if (!Character)
    {
        ListView_Skills->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    TArray<ARoundAbilityActor*> AbilitiesToShow;
    for (ARoundAbilityActor* Ability : Character->GrantedAbilities)
    {
        if (Ability && Ability->AbilityData)
        {
            AbilitiesToShow.Add(Ability);
        }
    }

    if (AbilitiesToShow.Num() == 0)
    {
        ListView_Skills->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    for (ARoundAbilityActor* Ability : AbilitiesToShow)
    {
        if (!Ability || !Ability->AbilityData)
        {
            continue;
        }

        const FName AbilityId = !Ability->AbilityData->AbilityId.IsNone()
            ? Ability->AbilityData->AbilityId
            : Ability->AbilityData->GetPrimaryAssetId().PrimaryAssetName;
        const FRoundSkillEntry* SkillSelection = Character->AbilitySkillSelections.FindByPredicate([AbilityId](const FRoundSkillEntry& Entry)
        {
            return Entry.Name.Equals(AbilityId.ToString(), ESearchCase::IgnoreCase);
        });

        URoundSkillHoverListItemData* Item = NewObject<URoundSkillHoverListItemData>(this);
        if (!Item)
        {
            continue;
        }

        Item->DisplayName = !Ability->AbilityData->DisplayName.IsEmpty()
            ? Ability->AbilityData->DisplayName
            : FText::FromString(AbilityId.ToString());
        Item->Description = !Ability->AbilityData->Description.IsEmpty()
            ? Ability->AbilityData->Description
            : FText::FromString(TEXT("Keine Beschreibung vorhanden."));
        Item->DetailsText = BuildSelectedAbilitySkillDetailsText(
            SkillSelection ? SkillSelection->Points : 0,
            Ability->AbilityData->SkillProgression ? Ability->AbilityData->SkillProgression->Levels : TArray<FRoundSkillpointDetails>());
        Item->MetaText = FText::FromString(FString::Printf(
            TEXT("Cost: %d   Range: %d   Cooldown: %d   Current CD: %d"),
            Ability->GetAbilityCost(),
            Ability->GetAbilityRange(),
            Ability->GetAbilityCooldown(),
            Ability->CurrentCooldown));
        Item->DamageText = FText::AsNumber(GetDisplayedAbilityDamage(Ability));
        Item->CurrentCooldownText = FText::AsNumber(Ability->CurrentCooldown);
        Item->MaxCooldownText = FText::AsNumber(Ability->GetAbilityCooldown());
        Item->HoverWidgetClass = Ability->AbilityData->SkillHoverWidgetClass;
        if (!Item->HoverWidgetClass)
        {
            Item->HoverWidgetClass = URoundSkillHoverInfoWidget::StaticClass();
        }

        SkillListItems.Add(Item);
    }

    UE_LOG(LogTemp, Log, TEXT("RefreshSkills Character=%s SkillItems=%d GrantedAbilities=%d"),
        *GetNameSafe(Character),
        SkillListItems.Num(),
        Character->GrantedAbilities.Num());

    if (SkillListItems.Num() == 0)
    {
        ListView_Skills->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    ListView_Skills->SetVisibility(ESlateVisibility::Visible);
    TArray<UObject*> Items;
    Items.Reserve(SkillListItems.Num());
    for (URoundSkillHoverListItemData* Item : SkillListItems)
    {
        if (Item)
        {
            Items.Add(Item);
        }
    }
    ListView_Skills->SetListItems(Items);
}

int32 URoundGameUIWidget::GetDisplayedAbilityDamage(const ARoundAbilityActor* Ability) const
{
    return Ability ? Ability->GetAbilityDamage() : 0;
}

void URoundGameUIWidget::RefreshCustomUI()
{
    ARoundCharacter* Character = GetDisplayedCharacter();
    TSubclassOf<UUserWidget> DesiredClass = Character && !IsPuppetCharacterForUI(Character) && Character->CharacterData
        ? Character->CharacterData->CustomUIClass
        : nullptr;

    if (!CustomUIPanel || !DesiredClass)
    {
        if (CurrentCustomUIWidget)
        {
            if (CustomUIPanel)
            {
                CustomUIPanel->RemoveChild(CurrentCustomUIWidget);
            }
            CurrentCustomUIWidget = nullptr;
            CustomUICharacter = nullptr;
        }
        ClearHoverInfoOverride(nullptr);
        return;
    }

    const bool bNeedsRecreate =
        !CurrentCustomUIWidget
        || CurrentCustomUIWidget->GetClass() != DesiredClass
        || CurrentCustomUIWidget->GetParent() != CustomUIPanel;

    if (bNeedsRecreate)
    {
        if (CurrentCustomUIWidget)
        {
            CustomUIPanel->RemoveChild(CurrentCustomUIWidget);
            CurrentCustomUIWidget = nullptr;
        }

        APlayerController* OwningController = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
        CurrentCustomUIWidget = CreateWidget<UUserWidget>(OwningController ? OwningController : GetOwningPlayer(), DesiredClass);
        if (CurrentCustomUIWidget)
        {
            CustomUIPanel->AddChild(CurrentCustomUIWidget);
        }
    }

    if (URoundChampionCustomUIWidget* ChampionWidget = Cast<URoundChampionCustomUIWidget>(CurrentCustomUIWidget))
    {
        if (CustomUICharacter.Get() != Character)
        {
            ChampionWidget->SetCharacter(Character);
            CustomUICharacter = Character;
        }
        else
        {
            ChampionWidget->UpdateUI();
        }

        ChampionWidget->SetOwningGameUI(this);
    }
}

void URoundGameUIWidget::RefreshInfoPanel()
{
    ARoundCharacter* Character = GetDisplayedCharacter();

    if (UTextBlock* InfoText = ResolveTextBlock(TEXT("TextBlock_Info")))
    {
        if (bHasHoverInfoOverride && HoverInfoCharacter.Get() == Character)
        {
            InfoText->SetText(HoverInfoMainText);
        }
        else if (Character)
        {
            InfoText->SetText(BuildBaseStatsText(Character));
        }
        else
        {
            InfoText->SetText(FText::GetEmpty());
        }
    }

    if (UTextBlock* InfoSubText = ResolveTextBlock(TEXT("TextBlock_Info_1")))
    {
        if (bHasHoverInfoOverride && HoverInfoCharacter.Get() == Character)
        {
            InfoSubText->SetText(HoverInfoSubText);
        }
        else
        {
            InfoSubText->SetText(FText::GetEmpty());
        }
    }

}

void URoundGameUIWidget::HandleAbility1Clicked()
{
    OnAbilityClicked(0);
}

void URoundGameUIWidget::HandleAbility2Clicked()
{
    OnAbilityClicked(1);
}

void URoundGameUIWidget::HandleAbility3Clicked()
{
    OnAbilityClicked(2);
}

void URoundGameUIWidget::HandleAbility4Clicked()
{
    OnAbilityClicked(3);
}

void URoundGameUIWidget::HandleAbility5Clicked()
{
    OnAbilityClicked(4);
}

void URoundGameUIWidget::HandleAbility6Clicked()
{
    OnAbilityClicked(5);
}

void URoundGameUIWidget::HandleAbility7Clicked()
{
    OnAbilityClicked(6);
}

void URoundGameUIWidget::BindButtons()
{
    if (bButtonsBound)
    {
        return;
    }

    if (Button_Move)
    {
        Button_Move->OnClicked.AddDynamic(this, &URoundGameUIWidget::OnMoveClicked);
    }

    if (Button_Attack)
    {
        Button_Attack->OnClicked.AddDynamic(this, &URoundGameUIWidget::OnAttackClicked);
    }

    if (Button_EndTurn)
    {
        Button_EndTurn->OnClicked.AddDynamic(this, &URoundGameUIWidget::OnEndTurnClicked);
    }

    if (Button_Interact)
    {
        Button_Interact->OnClicked.AddDynamic(this, &URoundGameUIWidget::OnInteractClicked);
    }

    if (Button_Ability1)
    {
        Button_Ability1->OnClicked.AddDynamic(this, &URoundGameUIWidget::HandleAbility1Clicked);
    }

    if (Button_Ability2)
    {
        Button_Ability2->OnClicked.AddDynamic(this, &URoundGameUIWidget::HandleAbility2Clicked);
    }

    if (Button_Ability3)
    {
        Button_Ability3->OnClicked.AddDynamic(this, &URoundGameUIWidget::HandleAbility3Clicked);
    }

    if (Button_Ability4)
    {
        Button_Ability4->OnClicked.AddDynamic(this, &URoundGameUIWidget::HandleAbility4Clicked);
    }

    if (Button_Ability5)
    {
        Button_Ability5->OnClicked.AddDynamic(this, &URoundGameUIWidget::HandleAbility5Clicked);
    }

    if (Button_Ability6)
    {
        Button_Ability6->OnClicked.AddDynamic(this, &URoundGameUIWidget::HandleAbility6Clicked);
    }

    if (Button_Ability7)
    {
        Button_Ability7->OnClicked.AddDynamic(this, &URoundGameUIWidget::HandleAbility7Clicked);
    }

    bButtonsBound = true;
}

UButton* URoundGameUIWidget::GetAbilityButtonByIndex(const int32 AbilityIndex) const
{
    switch (AbilityIndex)
    {
    case 0: return Button_Ability1;
    case 1: return Button_Ability2;
    case 2: return Button_Ability3;
    case 3: return Button_Ability4;
    case 4: return Button_Ability5;
    case 5: return Button_Ability6;
    case 6: return Button_Ability7;
    default: return nullptr;
    }
}

UTextBlock* URoundGameUIWidget::GetAbilityLabelByIndex(const int32 AbilityIndex) const
{
    switch (AbilityIndex)
    {
    case 0: return TextBlock_Ability1;
    case 1: return TextBlock_Ability2;
    case 2: return TextBlock_Ability3;
    case 3: return TextBlock_Ability4;
    case 4: return TextBlock_Ability5;
    case 5: return TextBlock_Ability6;
    case 6: return TextBlock_Ability7;
    default: return nullptr;
    }
}

UButton* URoundGameUIWidget::ResolveButton(const TCHAR* ButtonName) const
{
    if (FCString::Strcmp(ButtonName, TEXT("Button_Move")) == 0)
    {
        return Button_Move;
    }

    if (FCString::Strcmp(ButtonName, TEXT("Button_Attack")) == 0)
    {
        return Button_Attack;
    }

    if (FCString::Strcmp(ButtonName, TEXT("Button_EndTurn")) == 0)
    {
        return Button_EndTurn;
    }

    if (FCString::Strcmp(ButtonName, TEXT("Button_Interact")) == 0)
    {
        if (Button_Interact.Get())
        {
            return Button_Interact.Get();
        }

        if (UButton* ExactWidget = Cast<UButton>(GetWidgetFromName(ButtonName)))
        {
            return ExactWidget;
        }

        if (UButton* PartialWidget = FindWidgetByPartialName<UButton>(WidgetTree, TEXT("Interact")))
        {
            UE_LOG(LogTemp, Warning, TEXT("ResolveButton matched Button_Interact fallback widget %s"), *GetNameSafe(PartialWidget));
            return PartialWidget;
        }
    }

    if (UWidgetTree* Tree = WidgetTree)
    {
        return Cast<UButton>(Tree->FindWidget(FName(ButtonName)));
    }

    return Cast<UButton>(GetWidgetFromName(ButtonName));
}

UTextBlock* URoundGameUIWidget::ResolveTextBlock(const TCHAR* TextName) const
{
    if (FCString::Strcmp(TextName, TEXT("TextBlock")) == 0)
    {
        return TextBlock;
    }

    if (FCString::Strcmp(TextName, TEXT("TextBlock_234")) == 0)
    {
        return TextBlock_234;
    }

    if (FCString::Strcmp(TextName, TEXT("TextBlock_174")) == 0)
    {
        return TextBlock_174;
    }

    if (FCString::Strcmp(TextName, TEXT("TextBlock_Info")) == 0)
    {
        return TextBlock_Info;
    }

    if (FCString::Strcmp(TextName, TEXT("TextBlock_Info_1")) == 0)
    {
        return TextBlock_Info_1;
    }

    if (FCString::Strcmp(TextName, TEXT("TextBlock_Health")) == 0)
    {
        return TextBlock_Health;
    }

    if (FCString::Strcmp(TextName, TEXT("TextBlock_Initative")) == 0)
    {
        return TextBlock_Initative;
    }

    if (FCString::Strcmp(TextName, TEXT("Interact")) == 0)
    {
        if (Interact.Get())
        {
            return Interact.Get();
        }

        if (UTextBlock* ExactWidget = Cast<UTextBlock>(GetWidgetFromName(TextName)))
        {
            return ExactWidget;
        }

        if (UTextBlock* PartialWidget = FindWidgetByPartialName<UTextBlock>(WidgetTree, TEXT("Interact")))
        {
            UE_LOG(LogTemp, Warning, TEXT("ResolveTextBlock matched Interact fallback widget %s"), *GetNameSafe(PartialWidget));
            return PartialWidget;
        }
    }

    if (UWidgetTree* Tree = WidgetTree)
    {
        return Cast<UTextBlock>(Tree->FindWidget(FName(TextName)));
    }

    return Cast<UTextBlock>(GetWidgetFromName(TextName));
}

UProgressBar* URoundGameUIWidget::ResolveProgressBar(const TCHAR* ProgressBarName) const
{
    if (FCString::Strcmp(ProgressBarName, TEXT("Health_Bar")) == 0)
    {
        return Health_Bar;
    }

    if (FCString::Strcmp(ProgressBarName, TEXT("InitativeBar")) == 0)
    {
        return InitativeBar;
    }

    return Cast<UProgressBar>(GetWidgetFromName(ProgressBarName));
}
