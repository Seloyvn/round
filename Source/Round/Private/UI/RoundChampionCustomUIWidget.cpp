#include "UI/RoundChampionCustomUIWidget.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundCharacter1.h"
#include "Actors/RoundCharacter2.h"
#include "Actors/RoundCharacter3.h"
#include "Actors/RoundCharacter4.h"
#include "Actors/RoundCharacter5.h"
#include "Actors/RoundCharacter6.h"
#include "Actors/RoundCharacterBladecaller.h"
#include "Actors/RoundCharacterCleric.h"
#include "Actors/RoundCharacterChronomancer.h"
#include "Actors/RoundCharacterDragoon.h"
#include "Actors/RoundCharacterDruid.h"
#include "Actors/RoundCharacterGambler.h"
#include "Actors/RoundCharacterGeomancer.h"
#include "Actors/RoundGridTile.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ListView.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundAbilityData.h"
#include "Data/RoundCharacterData.h"
#include "Data/RoundCharacterProgressionData.h"
#include "Data/RoundGameDataRegistryLookup.h"
#include "Data/RoundSkillProgressionData.h"
#include "Engine/DataTable.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Game/RoundPlayerPawn.h"
#include "Input/Events.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "UI/RoundGameUIWidget.h"
#include "UI/RoundHoverInfoWidget.h"
#include "UI/RoundSkillHoverListEntryWidget.h"
#include "Effects/RoundBlessingEffect.h"
#include "UObject/UnrealType.h"
#include "Types/SlateEnums.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundCustomUI, Log, All);

namespace
{
template <typename TWidgetType>
TWidgetType* FindChampionWidgetByPartialName(const UWidgetTree* Tree, const FString& Needle)
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
            if (TypedWidget->GetName().Contains(Needle, ESearchCase::IgnoreCase))
            {
                return TypedWidget;
            }
        }
    }

    return nullptr;
}

int32 CountPackedSelections(const int64 PackedValue)
{
    int32 Count = 0;
    for (int32 TierIndex = 0; TierIndex < 4; ++TierIndex)
    {
        const int32 Selection = static_cast<int32>((PackedValue >> (TierIndex * 2)) & 0x3);
        if (Selection == 0)
        {
            break;
        }

        ++Count;
    }

    return Count;
}

FString FormatRoundHoverNumericValue(const double Value)
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

FString FormatRoundHoverSkillpointDescription(const FRoundSkillpointDetails& Level)
{
    FString Result = Level.Description;
    for (const TPair<FString, double>& Pair : Level.Values)
    {
        const FString BraceToken = FString::Printf(TEXT("{%s}"), *Pair.Key);
        const FString NegativeBraceToken = FString::Printf(TEXT("{-%s}"), *Pair.Key);
        const FString PercentToken = FString::Printf(TEXT("%%%s%%"), *Pair.Key);
        const FString FormattedValue = FormatRoundHoverNumericValue(Pair.Value);
        const FString InvertedFormattedValue = FormatRoundHoverNumericValue(-Pair.Value);
        Result.ReplaceInline(*BraceToken, *FormattedValue, ESearchCase::IgnoreCase);
        Result.ReplaceInline(*NegativeBraceToken, *InvertedFormattedValue, ESearchCase::IgnoreCase);
        Result.ReplaceInline(*PercentToken, *FormattedValue, ESearchCase::IgnoreCase);
    }

    return Result;
}

bool ResolveCharacterPassiveLevels(const ARoundCharacter* Character, TArray<FRoundSkillpointDetails>& OutLevels)
{
    OutLevels.Reset();
    if (!Character || !Character->CharacterData)
    {
        return false;
    }

    const URoundCharacterData* CharacterData = Character->CharacterData;
    if (CharacterData->CharacterProgression)
    {
        for (const FRoundSkillpointTier& Tier : CharacterData->CharacterProgression->Tiers)
        {
            OutLevels.Append(Tier.Choices);
        }
        return OutLevels.Num() > 0;
    }

    if (!CharacterData->SkillpointsTable)
    {
        return false;
    }

    UDataTable* SkillpointTable = CharacterData->SkillpointsTable;
    if (SkillpointTable->GetRowStruct() == FRoundSkillpointDetails::StaticStruct())
    {
        TArray<FRoundSkillpointDetails*> Rows;
        SkillpointTable->GetAllRows(TEXT("RoundChampionCustomUI.ResolveCharacterPassiveLevels"), Rows);
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
        SkillpointTable->GetAllRows(TEXT("RoundChampionCustomUI.ResolveCharacterPassiveLevels"), Rows);
        for (const FRoundSkillpointsDetails* Row : Rows)
        {
            if (Row)
            {
                OutLevels.Append(Row->Skillpoints);
            }
        }
    }

    return OutLevels.Num() > 0;
}

FText BuildSelectedSkillpointDetailsText(const int64 PackedSelections, const TArray<FRoundSkillpointDetails>& Levels)
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

        FString Description = FormatRoundHoverSkillpointDescription(Levels[RowIndex]);
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

void AssignListEntryClass(UListView* ListView, UClass* EntryClass)
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
    }
}

TSubclassOf<URoundHoverInfoWidget> ResolveDefaultHoverInfoWidgetClass()
{
    if (UClass* LoadedClass = RoundGameDataRegistryLookup::ResolveDefaultHoverInfoWidgetClass().Get())
    {
        UE_LOG(LogRoundCustomUI, Log, TEXT("ResolveDefaultHoverInfoWidgetClass loaded %s"), *GetNameSafe(LoadedClass));
        return LoadedClass;
    }

    UE_LOG(LogRoundCustomUI, Warning, TEXT("ResolveDefaultHoverInfoWidgetClass has no registry class, falling back to native class"));
    return URoundHoverInfoWidget::StaticClass();
}

FString GetRoundCardSuitLabel(const ERoundCardSuit Suit)
{
    switch (Suit)
    {
    case ERoundCardSuit::Hearts:
        return TEXT("H");
    case ERoundCardSuit::Spades:
        return TEXT("S");
    case ERoundCardSuit::Clubs:
        return TEXT("C");
    case ERoundCardSuit::Diamonds:
    default:
        return TEXT("D");
    }
}

FString GetRoundCardRankLabel(const int32 RankValue)
{
    switch (RankValue)
    {
    case 11:
        return TEXT("J");
    case 12:
        return TEXT("Q");
    case 13:
        return TEXT("K");
    case 14:
        return TEXT("A");
    default:
        return FString::FromInt(RankValue);
    }
}
}

void URoundChampionCustomUIWidget::SetCharacter(ARoundCharacter* InCharacter)
{
    Character = InCharacter;
    UpdateUI();
}

void URoundChampionCustomUIWidget::UpdateUI()
{
    RefreshSkillHoverList();
}

void URoundChampionCustomUIWidget::SetOwningGameUI(URoundGameUIWidget* InOwningGameUI)
{
    OwningGameUI = InOwningGameUI;
    RefreshSkillHoverList();
}

FText URoundChampionCustomUIWidget::GetHoverDescriptionText() const
{
    return FText::GetEmpty();
}

bool URoundChampionCustomUIWidget::BeginAbilityHandCardSelection(int32 AbilityIndex, ARoundAbilityActor* Ability)
{
    if (!Ability || !Ability->RequiresHandCardSelection())
    {
        return false;
    }

    PendingHandSelectionAbility = Ability;
    PendingHandSelectionAbilityIndex = AbilityIndex;
    PendingHandSelectionMin = FMath::Max(0, Ability->GetMinHandCardSelections());
    PendingHandSelectionMax = FMath::Max(PendingHandSelectionMin, Ability->GetMaxHandCardSelections());
    PendingHandSelectionIndices.Reset();
    bHandCardSelectionActive = true;
    UpdateUI();
    return true;
}

bool URoundChampionCustomUIWidget::IsHandCardSelectionActive() const
{
    return bHandCardSelectionActive;
}

int32 URoundChampionCustomUIWidget::GetPendingHandCardSelectionAbilityIndex() const
{
    return PendingHandSelectionAbilityIndex;
}

int32 URoundChampionCustomUIWidget::GetSelectedHandCardCount() const
{
    return PendingHandSelectionIndices.Num();
}

bool URoundChampionCustomUIWidget::IsHandCardSelected(int32 CardIndex) const
{
    return PendingHandSelectionIndices.Contains(CardIndex);
}

bool URoundChampionCustomUIWidget::IsHandCardSelectable(int32 CardIndex) const
{
    if (!bHandCardSelectionActive || !PendingHandSelectionAbility.IsValid())
    {
        return false;
    }

    return PendingHandSelectionAbility->IsValidHandCardSelectionIndex(CardIndex);
}

void URoundChampionCustomUIWidget::ToggleHandCardSelection(int32 CardIndex)
{
    if (!IsHandCardSelectable(CardIndex))
    {
        return;
    }

    if (PendingHandSelectionIndices.Contains(CardIndex))
    {
        PendingHandSelectionIndices.Remove(CardIndex);
    }
    else
    {
        if (PendingHandSelectionMax > 0 && PendingHandSelectionIndices.Num() >= PendingHandSelectionMax)
        {
            PendingHandSelectionIndices.RemoveAt(0);
        }
        PendingHandSelectionIndices.Add(CardIndex);
    }

    if (PendingHandSelectionMin == PendingHandSelectionMax
        && PendingHandSelectionMax > 0
        && PendingHandSelectionIndices.Num() == PendingHandSelectionMax)
    {
        ConfirmHandCardSelection();
        return;
    }

    UpdateUI();
}

void URoundChampionCustomUIWidget::ConfirmHandCardSelection()
{
    if (!bHandCardSelectionActive || !PendingHandSelectionAbility.IsValid() || !OwningGameUI.IsValid() || PendingHandSelectionAbilityIndex < 0)
    {
        return;
    }

    if (PendingHandSelectionIndices.Num() < PendingHandSelectionMin)
    {
        return;
    }

    if (ARoundPlayerPawn* PlayerPawn = OwningGameUI->Player)
    {
        PlayerPawn->UseAbilityDirectWithHandSelection(PendingHandSelectionAbilityIndex, PendingHandSelectionIndices);
    }

    CancelHandCardSelection();
    if (OwningGameUI.IsValid())
    {
        OwningGameUI->RefreshFromPlayer();
    }
}

void URoundChampionCustomUIWidget::CancelHandCardSelection()
{
    bHandCardSelectionActive = false;
    PendingHandSelectionAbility = nullptr;
    PendingHandSelectionAbilityIndex = INDEX_NONE;
    PendingHandSelectionMin = 0;
    PendingHandSelectionMax = 0;
    PendingHandSelectionIndices.Reset();
    UpdateUI();
}

TArray<FRoundPlayingCard> URoundChampionCustomUIWidget::GetVisibleHandCards() const
{
    if (const ARoundCharacterGambler* Gambler = Cast<ARoundCharacterGambler>(Character))
    {
        return Gambler->GetHandCards();
    }

    return {};
}

FText URoundChampionCustomUIWidget::GetHandCardDisplayText(int32 CardIndex) const
{
    const TArray<FRoundPlayingCard> Cards = GetVisibleHandCards();
    if (!Cards.IsValidIndex(CardIndex))
    {
        return FText::GetEmpty();
    }

    const FRoundPlayingCard& Card = Cards[CardIndex];
    return FText::FromString(FString::Printf(TEXT("%s%s"), *GetRoundCardSuitLabel(Card.Suit), *GetRoundCardRankLabel(Card.RankValue)));
}

void URoundChampionCustomUIWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
}

void URoundChampionCustomUIWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);
}

void URoundChampionCustomUIWidget::RefreshSkillHoverList()
{
    if (!WidgetTree)
    {
        UE_LOG(LogRoundCustomUI, Warning, TEXT("RefreshSkillHoverList skipped - WidgetTree missing for %s"), *GetNameSafe(this));
        return;
    }

    if (!Character)
    {
        PassiveHoverTooltip = nullptr;
        ApplyPassiveHoverTooltip();
        if (ListView_Skills)
        {
            ListView_Skills->ClearListItems();
        }
        UE_LOG(LogRoundCustomUI, Log, TEXT("RefreshSkillHoverList cleared hover/list - no character"));
        return;
    }

    PassiveHoverTooltip = CreateHoverInfoWidget(
        Character->CharacterData && !Character->CharacterData->DisplayName.IsEmpty()
            ? Character->CharacterData->DisplayName
            : FText::FromString(TEXT("Character")),
        BuildPassiveDescriptionText(),
        BuildPassiveDetailsText());
    if (URoundHoverInfoWidget* PassiveHoverInfoWidget = Cast<URoundHoverInfoWidget>(PassiveHoverTooltip))
    {
        PassiveHoverInfoWidget->SetSkillItems(SkillListItems);
    }
    UE_LOG(LogRoundCustomUI, Log, TEXT("RefreshSkillHoverList PassiveHoverTooltip=%s Character=%s"), *GetNameSafe(PassiveHoverTooltip), *GetNameSafe(Character));
    ApplyPassiveHoverTooltip();

    SkillListItems.Reset();

    TArray<URoundAbilityData*> AbilitiesToShow;
    if (Character->GrantedAbilities.Num() > 0)
    {
        for (ARoundAbilityActor* AbilityActor : Character->GrantedAbilities)
        {
            if (AbilityActor && AbilityActor->AbilityData)
            {
                AbilitiesToShow.Add(AbilityActor->AbilityData);
            }
        }
    }
    else if (Character->CharacterData)
    {
        AbilitiesToShow = Character->CharacterData->Abilities;
    }

    UE_LOG(LogRoundCustomUI, Log, TEXT("RefreshSkillHoverList Character=%s GrantedAbilities=%d CharacterDataAbilities=%d AbilitiesToShow=%d SkillSelections=%d"),
        *GetNameSafe(Character),
        Character->GrantedAbilities.Num(),
        Character->CharacterData ? Character->CharacterData->Abilities.Num() : -1,
        AbilitiesToShow.Num(),
        Character->AbilitySkillSelections.Num());

    for (URoundAbilityData* AbilityData : AbilitiesToShow)
    {
        if (!AbilityData)
        {
            continue;
        }

        const FName AbilityId = !AbilityData->AbilityId.IsNone() ? AbilityData->AbilityId : AbilityData->GetPrimaryAssetId().PrimaryAssetName;
        const FRoundSkillEntry* SkillSelection = Character->AbilitySkillSelections.FindByPredicate([AbilityId](const FRoundSkillEntry& Entry)
            {
                return Entry.Name.Equals(AbilityId.ToString(), ESearchCase::IgnoreCase);
            });

        URoundSkillHoverListItemData* Item = NewObject<URoundSkillHoverListItemData>(this);
        if (!Item)
        {
            continue;
        }

        Item->DisplayName = (AbilityData && !AbilityData->DisplayName.IsEmpty())
            ? AbilityData->DisplayName
            : FText::FromString(AbilityId.ToString());
        Item->Description = (AbilityData && !AbilityData->Description.IsEmpty())
            ? AbilityData->Description
            : FText::FromString(TEXT("Keine Beschreibung vorhanden."));
        TSubclassOf<URoundHoverInfoWidget> HoverWidgetClass = ResolveDefaultHoverInfoWidgetClass();
        if (Character->CharacterData && Character->CharacterData->HoverInfoWidgetClass)
        {
            HoverWidgetClass = Character->CharacterData->HoverInfoWidgetClass;
        }
        Item->HoverWidgetClass = HoverWidgetClass;
        Item->DetailsText = BuildSelectedSkillpointDetailsText(
            SkillSelection ? SkillSelection->Points : 0,
            AbilityData->SkillProgression ? AbilityData->SkillProgression->Levels : TArray<FRoundSkillpointDetails>());
        SkillListItems.Add(Item);
    }
    UE_LOG(LogRoundCustomUI, Log, TEXT("RefreshSkillHoverList applied skill items=%d"), SkillListItems.Num());

    if (URoundHoverInfoWidget* PassiveHoverInfoWidget = Cast<URoundHoverInfoWidget>(PassiveHoverTooltip))
    {
        PassiveHoverInfoWidget->SetSkillItems(SkillListItems);
    }
}

FText URoundChampionCustomUIWidget::BuildPassiveDescriptionText() const
{
    if (!Character || !Character->CharacterData)
    {
        return FText::GetEmpty();
    }

    const FText PassiveDescription =
        (Character->CharacterData->CharacterProgression && !Character->CharacterData->CharacterProgression->Description.IsEmpty())
            ? Character->CharacterData->CharacterProgression->Description
            : Character->CharacterData->Description;

    if (!PassiveDescription.IsEmpty())
    {
        return PassiveDescription;
    }

    const FText CustomDescription = GetHoverDescriptionText();
    if (!CustomDescription.IsEmpty())
    {
        return CustomDescription;
    }

    const FText PassiveDetailsText = BuildPassiveDetailsText();
    if (!PassiveDetailsText.IsEmpty())
    {
        return PassiveDetailsText;
    }

    return !Character->CharacterData->DisplayName.IsEmpty()
        ? Character->CharacterData->DisplayName
        : FText::FromString(Character->CharacterData->CharacterId.ToString());
}

FText URoundChampionCustomUIWidget::BuildPassiveDetailsText() const
{
    if (!Character)
    {
        return FText::GetEmpty();
    }

    TArray<FRoundSkillpointDetails> PassiveLevels;
    if (!ResolveCharacterPassiveLevels(Character, PassiveLevels))
    {
        return FText::FromString(TEXT("Keine Punkte geskillt."));
    }

    return BuildSelectedSkillpointDetailsText(Character->CharacterPassiveSelections, PassiveLevels);
}

UUserWidget* URoundChampionCustomUIWidget::CreateHoverInfoWidget(const FText& Title, const FText& Description, const FText& Details) const
{
    TSubclassOf<URoundHoverInfoWidget> TooltipClass = ResolveDefaultHoverInfoWidgetClass();
    if (Character && Character->CharacterData && Character->CharacterData->HoverInfoWidgetClass)
    {
        TooltipClass = Character->CharacterData->HoverInfoWidgetClass;
    }
    if (!TooltipClass)
    {
        UE_LOG(LogRoundCustomUI, Warning, TEXT("CreateHoverInfoWidget no tooltip class for %s"), *GetNameSafe(Character));
        return nullptr;
    }

    APlayerController* OwningController = GetOwningPlayer();
    if (!OwningController && OwningGameUI.IsValid())
    {
        OwningController = OwningGameUI->GetOwningPlayer();
    }

    URoundHoverInfoWidget* TooltipWidget = OwningController
        ? CreateWidget<URoundHoverInfoWidget>(OwningController, TooltipClass)
        : CreateWidget<URoundHoverInfoWidget>(GetWorld(), TooltipClass);
    if (!TooltipWidget)
    {
        UE_LOG(LogRoundCustomUI, Warning, TEXT("CreateHoverInfoWidget failed to create widget - Class=%s Character=%s"), *GetNameSafe(TooltipClass.Get()), *GetNameSafe(Character));
        return nullptr;
    }

    TooltipWidget->SetHoverContent(Title, Description, Details);
    TooltipWidget->SetVisibility(ESlateVisibility::Visible);
    TooltipWidget->SetRenderOpacity(1.f);
    UE_LOG(LogRoundCustomUI, Log, TEXT("CreateHoverInfoWidget created %s from %s for Character=%s OwningController=%s"),
        *GetNameSafe(TooltipWidget),
        *GetNameSafe(TooltipClass.Get()),
        *GetNameSafe(Character),
        *GetNameSafe(OwningController));
    return TooltipWidget;
}

void URoundChampionCustomUIWidget::ApplyPassiveHoverTooltip()
{
    if (!WidgetTree)
    {
        UE_LOG(LogRoundCustomUI, Warning, TEXT("ApplyPassiveHoverTooltip skipped - WidgetTree missing for %s"), *GetNameSafe(this));
        return;
    }

    SetToolTip(PassiveHoverTooltip);

    TArray<UWidget*> Widgets;
    WidgetTree->GetAllWidgets(Widgets);
    for (UWidget* Widget : Widgets)
    {
        if (Widget)
        {
            Widget->SetToolTip(PassiveHoverTooltip);
        }
    }

    UE_LOG(LogRoundCustomUI, Log, TEXT("ApplyPassiveHoverTooltip applied tooltip=%s for widget=%s childCount=%d"),
        *GetNameSafe(PassiveHoverTooltip),
        *GetNameSafe(this),
        Widgets.Num());
}

void URoundChampionCustomUIWidget::ShowPassiveHoverWidgetAtScreenPosition(const FVector2D& ScreenPosition)
{
    UE_LOG(LogRoundCustomUI, Verbose, TEXT("ShowPassiveHoverWidgetAtScreenPosition legacy path ignored for widget=%s at (%.1f, %.1f)"),
        *GetNameSafe(this),
        ScreenPosition.X,
        ScreenPosition.Y);
}

void URoundChampionCustomUIWidget::HidePassiveHoverWidget()
{
    if (ActiveHoverViewportWidget)
    {
        UE_LOG(LogRoundCustomUI, Log, TEXT("HidePassiveHoverWidget removing %s"), *GetNameSafe(ActiveHoverViewportWidget));
        ActiveHoverViewportWidget->RemoveFromParent();
        ActiveHoverViewportWidget = nullptr;
    }
}

void URoundChampionCustomUIWidget::HandleSkillItemHovered(UObject* Item, const bool bIsHovered)
{
    if (!bIsHovered)
    {
        ApplyPassiveHoverTooltip();
    }
}

void URoundChampion1CustomUIWidget::UpdateUI()
{
    if (!TextBlock)
    {
        return;
    }

    if (const ARoundCharacter1* Character1 = Cast<ARoundCharacter1>(Character))
    {
        TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Character1->GetCurrentChainCount(), Character1->GetChainLimit())));
    }
}

FText URoundChampion1CustomUIWidget::GetHoverDescriptionText() const
{
    return TextBlock ? TextBlock->GetText() : FText::GetEmpty();
}

void URoundChampion2CustomUIWidget::UpdateUI()
{
    if (!TextBlock)
    {
        return;
    }

    if (const ARoundCharacter2* Character2 = Cast<ARoundCharacter2>(Character))
    {
        TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), Character2->Ammo, Character2->GetMaxAmmo())));
    }
}

FText URoundChampion2CustomUIWidget::GetHoverDescriptionText() const
{
    return TextBlock ? TextBlock->GetText() : FText::GetEmpty();
}

void URoundChampion3CustomUIWidget::UpdateUI()
{
    if (!TextBlock)
    {
        return;
    }

    const ARoundCharacter3* Character3 = Cast<ARoundCharacter3>(Character);
    if (!Character3)
    {
        return;
    }

    TextBlock->SetText(FText::FromString(FString::Printf(
        TEXT("%d/%d"),
        Character3->TryGetComboPoints(),
        Character3->GetMaxComboPoints())));
}

FText URoundChampion3CustomUIWidget::GetHoverDescriptionText() const
{
    return TextBlock ? TextBlock->GetText() : FText::GetEmpty();
}

void URoundChampion4CustomUIWidget::UpdateUI()
{
    const ARoundCharacter4* Character4 = Cast<ARoundCharacter4>(Character);
    if (!Character4)
    {
        return;
    }

    UE_LOG(LogRoundCustomUI, Log, TEXT("Character4 UI Update - Character=%s ActiveBonus=%d UsedCount=%d HasFire=%d HasIce=%d HasLightning=%d HasRock=%d HasFallback=%d"),
        *GetNameSafe(Character4),
        static_cast<int32>(Character4->GetActiveElementBonus()),
        Character4->UsedElementBonusesThisTurn.Num(),
        Image_Fire ? 1 : 0,
        Image_Ice ? 1 : 0,
        Image_Lightning ? 1 : 0,
        Image_Rock ? 1 : 0,
        Image_236 ? 1 : 0);

    auto GetElementColor = [](const ERoundMageElementBonus Bonus) -> FLinearColor
    {
        switch (Bonus)
        {
        case ERoundMageElementBonus::FirePower:
            return FLinearColor(1.0f, 0.35f, 0.1f, 1.0f);
        case ERoundMageElementBonus::IceControl:
            return FLinearColor(0.4f, 0.8f, 1.0f, 1.0f);
        case ERoundMageElementBonus::LightningTempo:
            return FLinearColor(1.0f, 0.9f, 0.2f, 1.0f);
        case ERoundMageElementBonus::RockGuard:
            return FLinearColor(0.55f, 0.45f, 0.3f, 1.0f);
        case ERoundMageElementBonus::None:
        default:
            return FLinearColor(0.8f, 0.8f, 0.8f, 1.0f);
        }
    };

    auto ApplyElementState = [&](UImage* Image, const ERoundMageElementBonus Bonus)
    {
        if (!Image)
        {
            return;
        }

        FLinearColor Tint = GetElementColor(Bonus);
        if (Character4->IsElementBonusActive(Bonus))
        {
            Tint *= 1.35f;
            Tint.A = 1.0f;
        }
        else if (Character4->WasElementBonusUsedThisTurn(Bonus))
        {
            Tint = FLinearColor(0.18f, 0.18f, 0.18f, 1.0f);
        }
        else
        {
            Tint *= 1.0f;
            Tint.A = 0.45f;
        }

        Image->SetBrushTintColor(FSlateColor(Tint));
    };

    ApplyElementState(Image_Fire, ERoundMageElementBonus::FirePower);
    ApplyElementState(Image_Ice, ERoundMageElementBonus::IceControl);
    ApplyElementState(Image_Lightning, ERoundMageElementBonus::LightningTempo);
    ApplyElementState(Image_Rock, ERoundMageElementBonus::RockGuard);

    if (Image_236)
    {
        ERoundMageElementBonus DisplayBonus = ERoundMageElementBonus::None;
        for (const ERoundMageElementBonus Candidate : { ERoundMageElementBonus::FirePower, ERoundMageElementBonus::IceControl, ERoundMageElementBonus::LightningTempo, ERoundMageElementBonus::RockGuard })
        {
            if (Character4->IsElementBonusActive(Candidate))
            {
                DisplayBonus = Candidate;
                break;
            }
        }

        FLinearColor Tint = GetElementColor(DisplayBonus);
        if (DisplayBonus == ERoundMageElementBonus::None)
        {
            Tint = FLinearColor(0.8f, 0.8f, 0.8f, 0.45f);
        }

        Image_236->SetBrushTintColor(FSlateColor(Tint));
    }

    if (TextBlock)
    {
        auto GetStateMarker = [&](const ERoundMageElementBonus Bonus) -> FString
        {
            if (Character4->IsElementBonusActive(Bonus))
            {
                return TEXT("Aktiv");
            }

            if (Character4->WasElementBonusUsedThisTurn(Bonus))
            {
                return TEXT("Verbraucht");
            }

            return TEXT("Bereit");
        };

        TextBlock->SetText(FText::FromString(FString::Printf(
            TEXT("F:%s I:%s L:%s R:%s"),
            *GetStateMarker(ERoundMageElementBonus::FirePower),
            *GetStateMarker(ERoundMageElementBonus::IceControl),
            *GetStateMarker(ERoundMageElementBonus::LightningTempo),
            *GetStateMarker(ERoundMageElementBonus::RockGuard))));
    }
}

FText URoundChampion4CustomUIWidget::GetHoverDescriptionText() const
{
    return TextBlock ? TextBlock->GetText() : FText::GetEmpty();
}

void URoundChampion5CustomUIWidget::UpdateUI()
{
    if (const ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(Character))
    {
        if (Image_236)
        {
            const float BloodAlpha = FMath::Clamp(
                static_cast<float>(Character5->GetCurrentBlood()) / static_cast<float>(FMath::Max(1, Character5->GetBloodCap())),
                0.15f,
                1.0f);
            Image_236->SetBrushTintColor(FSlateColor(FLinearColor(1.0f, 0.0f, 0.0f, BloodAlpha)));
        }

        if (TextBlock_55)
        {
            TextBlock_55->SetText(FText::AsNumber(Character5->GetCurrentBlood()));
        }
    }
}

FText URoundChampion5CustomUIWidget::GetHoverDescriptionText() const
{
    return TextBlock_55 ? TextBlock_55->GetText() : FText::GetEmpty();
}

void URoundChampion6CustomUIWidget::UpdateUI()
{
    const ARoundCharacter6* Character6 = Cast<ARoundCharacter6>(Character);
    if (!Character6)
    {
        return;
    }

    const ARoundCharacter* PuppetCharacter = Character6->TryGetPuppetCharacter();
    const URoundAttributeSetComponent* PuppetAttributes = PuppetCharacter ? PuppetCharacter->Attributes : nullptr;
    const int32 CurrentHealth = PuppetAttributes ? PuppetAttributes->CurrentHealth : 0;
    const int32 MaxHealth = PuppetAttributes ? PuppetAttributes->BaseStats.MaxHealth : 0;
    const FText PuppetHealthText = FText::FromString(FString::Printf(TEXT("%d/%d"), CurrentHealth, MaxHealth));

    if (TextBlock)
    {
        TextBlock->SetText(PuppetHealthText);
    }

    if (TextBlock_55)
    {
        TextBlock_55->SetText(PuppetHealthText);
    }
}

FText URoundChampion6CustomUIWidget::GetHoverDescriptionText() const
{
    if (TextBlock_55 && !TextBlock_55->GetText().IsEmpty())
    {
        return TextBlock_55->GetText();
    }

    return TextBlock ? TextBlock->GetText() : FText::GetEmpty();
}

FReply URoundChampion6CustomUIWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

    const ARoundCharacter6* Character6 = Cast<ARoundCharacter6>(Character);
    if (!Character6)
    {
        return FReply::Unhandled();
    }

    ARoundCharacter* PuppetCharacter = Character6->TryGetPuppetCharacter();
    if (!IsValid(PuppetCharacter) || !IsValid(PuppetCharacter->CurrentTile.Get()))
    {
        return FReply::Unhandled();
    }

    APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController)
    {
        PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    }

    ARoundPlayerPawn* PlayerPawn = PlayerController ? Cast<ARoundPlayerPawn>(PlayerController->GetPawn()) : nullptr;
    if (!PlayerPawn || !PuppetCharacter->HasVision(PlayerPawn->TeamActor))
    {
        return FReply::Unhandled();
    }

    PlayerPawn->SelectField(PuppetCharacter->CurrentTile);
    return FReply::Handled();
}

void URoundChampionClericCustomUIWidget::UpdateUI()
{
    if (!TextBlock)
    {
        return;
    }

    const ARoundCharacterCleric* Cleric = Cast<ARoundCharacterCleric>(Character);
    if (!Cleric || !Cleric->GetWorld())
    {
        TextBlock->SetText(FText::GetEmpty());
        return;
    }

    int32 BlessedAllies = 0;
    int32 TotalBlessingStacks = 0;
    for (TActorIterator<ARoundCharacter> It(Cleric->GetWorld()); It; ++It)
    {
        const ARoundCharacter* Candidate = *It;
        if (!Candidate || Candidate->Team != Cleric->Team || Candidate == Cleric)
        {
            continue;
        }

        for (ARoundEffectActor* Effect : Candidate->ActiveEffects)
        {
            const ARoundBlessingEffect* BlessingEffect = Cast<ARoundBlessingEffect>(Effect);
            if (!BlessingEffect || BlessingEffect->CauserCharacter != Cleric)
            {
                continue;
            }

            ++BlessedAllies;
            TotalBlessingStacks += BlessingEffect->Stacks;
            break;
        }
    }

    TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d | %d"), BlessedAllies, TotalBlessingStacks)));
}

FText URoundChampionClericCustomUIWidget::GetHoverDescriptionText() const
{
    const ARoundCharacterCleric* Cleric = Cast<ARoundCharacterCleric>(Character);
    if (!Cleric)
    {
        return FText::GetEmpty();
    }

    return FText::FromString(FString::Printf(
        TEXT("Blessing heilt bei %d Stacks fuer %d Leben."),
        Cleric->GetBlessingMaxStacks(),
        Cleric->GetBlessingHealAmount()));
}

void URoundChampionBladecallerCustomUIWidget::UpdateUI()
{
    if (!TextBlock)
    {
        return;
    }

    const ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(Character);
    if (!Bladecaller)
    {
        TextBlock->SetText(FText::GetEmpty());
        return;
    }

    TextBlock->SetText(FText::FromString(FString::Printf(
        TEXT("%d"),
        Bladecaller->GetActiveBladeCount())));
}

FText URoundChampionBladecallerCustomUIWidget::GetHoverDescriptionText() const
{
    const ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(Character);
    if (!Bladecaller)
    {
        return FText::GetEmpty();
    }

    return FText::FromString(FString::Printf(
        TEXT("Aktive Klingen: %d. Klingen bestehen %d Initiative. Schaden und weitere Wirkung kommen von der Ability, die die Klinge bewegt."),
        Bladecaller->GetActiveBladeCount(),
        Bladecaller->GetBladeLifetimeInitiative()));
}

void URoundChampionChronomancerCustomUIWidget::UpdateUI()
{
    if (!TextBlock)
    {
        return;
    }

    const ARoundCharacterChronomancer* Chronomancer = Cast<ARoundCharacterChronomancer>(Character);
    if (!Chronomancer)
    {
        TextBlock->SetText(FText::GetEmpty());
        return;
    }

    TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), Chronomancer->GetFragments(), Chronomancer->GetMaxFragments())));
}

FText URoundChampionChronomancerCustomUIWidget::GetHoverDescriptionText() const
{
    const ARoundCharacterChronomancer* Chronomancer = Cast<ARoundCharacterChronomancer>(Character);
    if (!Chronomancer)
    {
        return FText::GetEmpty();
    }

    const FString FragmentText = FString::Printf(TEXT("Fragments: %d / %d"), Chronomancer->GetFragments(), Chronomancer->GetMaxFragments());
    return FText::FromString(FragmentText);
}

void URoundChampionDruidCustomUIWidget::UpdateUI()
{
    if (!TextBlock)
    {
        return;
    }

    const ARoundCharacterDruid* Druid = Cast<ARoundCharacterDruid>(Character);
    if (!Druid)
    {
        TextBlock->SetText(FText::GetEmpty());
        return;
    }

    TextBlock->SetText(FText::FromString(FString::Printf(
        TEXT("%s | %d"),
        Druid->IsBearFormActive() ? TEXT("Bear") : TEXT("Druid"),
        Druid->GetConnectedNatureFields().Num())));
}

FText URoundChampionDruidCustomUIWidget::GetHoverDescriptionText() const
{
    const ARoundCharacterDruid* Druid = Cast<ARoundCharacterDruid>(Character);
    if (!Druid)
    {
        return FText::GetEmpty();
    }

    const FString DruidText = FString::Printf(
        TEXT("Form: %s. Verbundene Naturfelder: %d."),
        Druid->IsBearFormActive() ? TEXT("Bear") : TEXT("Druid"),
        Druid->GetConnectedNatureFields().Num());
    return FText::FromString(DruidText);
}

void URoundChampionDragoonCustomUIWidget::UpdateUI()
{
    if (!TextBlock)
    {
        return;
    }

    const ARoundCharacterDragoon* Dragoon = Cast<ARoundCharacterDragoon>(Character);
    if (!Dragoon)
    {
        TextBlock->SetText(FText::GetEmpty());
        return;
    }

    TextBlock->SetText(FText::FromString(FString::Printf(
        TEXT("%d / %d"),
        Dragoon->GetMomentumStacks(),
        Dragoon->GetMomentumMaxStacks())));
}

FText URoundChampionDragoonCustomUIWidget::GetHoverDescriptionText() const
{
    const ARoundCharacterDragoon* Dragoon = Cast<ARoundCharacterDragoon>(Character);
    if (!Dragoon)
    {
        return FText::GetEmpty();
    }

    return FText::FromString(FString::Printf(
        TEXT("Momentum: %d / %d. Jeder Stack gibt %d Schaden. Trifft Dragoon in seinem Zug keinen Gegner, verliert er alle Stacks."),
        Dragoon->GetMomentumStacks(),
        Dragoon->GetMomentumMaxStacks(),
        Dragoon->GetDamagePerStack()));
}

void URoundChampionGeomancerCustomUIWidget::UpdateUI()
{
    if (!TextBlock)
    {
        return;
    }

    const ARoundCharacterGeomancer* Geomancer = Cast<ARoundCharacterGeomancer>(Character);
    if (!Geomancer)
    {
        TextBlock->SetText(FText::GetEmpty());
        return;
    }

    TextBlock->SetText(FText::FromString(FString::Printf(
        TEXT("%d"),
        Geomancer->GetMarkedEnemyCount())));
}

FText URoundChampionGeomancerCustomUIWidget::GetHoverDescriptionText() const
{
    const ARoundCharacterGeomancer* Geomancer = Cast<ARoundCharacterGeomancer>(Character);
    if (!Geomancer)
    {
        return FText::GetEmpty();
    }

    return FText::FromString(FString::Printf(
        TEXT("Markierte Gegner: %d. Standardangriffe auf markierte Ziele verursachen %d Bonusschaden und erzeugen hinter dem Ziel einen Rock. Dasselbe Ziel kann den Passive-Effekt nur alle %d Initiative ausloesen."),
        Geomancer->GetMarkedEnemyCount(),
        Geomancer->GetMarkedAttackBonusDamage(),
        Geomancer->GetPassivePerTargetCooldownInitiative()));
}

void URoundChampionGamblerCustomUIWidget::UpdateUI()
{
    if (!TextBlock)
    {
        return;
    }

    const ARoundCharacterGambler* Gambler = Cast<ARoundCharacterGambler>(Character);
    if (!Gambler)
    {
        TextBlock->SetText(FText::GetEmpty());
        return;
    }

    FString HandSummary;
    const TArray<FRoundPlayingCard>& HandCards = Gambler->GetHandCards();
    for (int32 CardIndex = 0; CardIndex < HandCards.Num(); ++CardIndex)
    {
        if (!HandSummary.IsEmpty())
        {
            HandSummary += TEXT(" ");
        }

        HandSummary += FString::Printf(TEXT("%s%s"),
            *GetRoundCardSuitLabel(HandCards[CardIndex].Suit),
            *GetRoundCardRankLabel(HandCards[CardIndex].RankValue));
    }

    if (IsHandCardSelectionActive())
    {
        HandSummary = FString::Printf(TEXT("[%d/%d] %s"),
            PendingHandSelectionIndices.Num(),
            PendingHandSelectionMax,
            *HandSummary);
    }

    TextBlock->SetText(FText::FromString(FString::Printf(
        TEXT("D:%d H:%d X:%d | %s"),
        Gambler->GetDeckCount(),
        Gambler->GetHandCount(),
        Gambler->GetDiscardCount(),
        *HandSummary)));
}

FText URoundChampionGamblerCustomUIWidget::GetHoverDescriptionText() const
{
    const ARoundCharacterGambler* Gambler = Cast<ARoundCharacterGambler>(Character);
    if (!Gambler)
    {
        return FText::GetEmpty();
    }

    FString HandSummary;
    const TArray<FRoundPlayingCard>& HandCards = Gambler->GetHandCards();
    for (int32 CardIndex = 0; CardIndex < HandCards.Num(); ++CardIndex)
    {
        if (!HandSummary.IsEmpty())
        {
            HandSummary += TEXT(", ");
        }

        HandSummary += FString::Printf(TEXT("%d:%s%s"),
            CardIndex + 1,
            *GetRoundCardSuitLabel(HandCards[CardIndex].Suit),
            *GetRoundCardRankLabel(HandCards[CardIndex].RankValue));
    }

    return FText::FromString(FString::Printf(
        TEXT("Deck %d | Hand %d/%d | Discard %d\n%s"),
        Gambler->GetDeckCount(),
        Gambler->GetHandCount(),
        Gambler->GetMaxHandCards(),
        Gambler->GetDiscardCount(),
        *HandSummary));
}
