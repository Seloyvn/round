#include "UI/RoundSkillMenuWidget.h"

#include "Data/RoundAbilityData.h"
#include "Data/RoundCharacterData.h"
#include "Data/RoundCharacterRegistryLookup.h"
#include "Data/RoundCharacterProgressionData.h"
#include "Data/RoundGameDataRegistryLookup.h"
#include "Data/RoundSkillProgressionData.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxKey.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "UI/RoundCharacterSkillpointMenuWidget.h"
#include "Engine/AssetManager.h"
#include "UI/RoundMainMenuWidget.h"
#include "UI/RoundSkillmenuSelectorPointWidget.h"
#include "UI/RoundSkillpointMenuWidget.h"
#include "UI/RoundStartPositionWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundSkillMenu, Log, All);

namespace
{
template <typename WidgetType>
WidgetType* FindWidgetByCandidateNames(UWidgetTree* WidgetTree, std::initializer_list<const TCHAR*> Names)
{
    if (!WidgetTree)
    {
        return nullptr;
    }

    for (const TCHAR* Name : Names)
    {
        if (WidgetType* Found = Cast<WidgetType>(WidgetTree->FindWidget(FName(Name))))
        {
            return Found;
        }
    }

    return nullptr;
}

template <typename WidgetType>
WidgetType* FindFirstWidgetOfClass(UWidgetTree* WidgetTree)
{
    if (!WidgetTree)
    {
        return nullptr;
    }

    WidgetType* Result = nullptr;
    WidgetTree->ForEachWidget([&Result](UWidget* Widget)
        {
            if (!Result)
            {
                Result = Cast<WidgetType>(Widget);
            }
        });
    return Result;
}
}

URoundSkillMenuWidget::URoundSkillMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URoundSkillMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!MenuHolder)
    {
        MenuHolder = FindWidgetByCandidateNames<UCanvasPanel>(WidgetTree, {TEXT("MenuHolder"), TEXT("CanvasPanel_0"), TEXT("CanvasPanel")});
        if (!MenuHolder)
        {
            MenuHolder = FindFirstWidgetOfClass<UCanvasPanel>(WidgetTree);
        }
    }
    if (!SkillmenuSelector)
    {
        SkillmenuSelector = FindWidgetByCandidateNames<UVerticalBox>(WidgetTree, {TEXT("SkillmenuSelector"), TEXT("VerticalBox_0"), TEXT("VerticalBox")});
        if (!SkillmenuSelector)
        {
            SkillmenuSelector = FindFirstWidgetOfClass<UVerticalBox>(WidgetTree);
        }
    }

    if (!SkillpointMenuClass)
    {
        SkillpointMenuClass = Cast<UClass>(RoundGameDataRegistryLookup::ResolveSkillpointMenuWidgetClass().Get());
    }
    if (!SkillSelectorPointClass)
    {
        SkillSelectorPointClass = Cast<UClass>(RoundGameDataRegistryLookup::ResolveSkillSelectorPointWidgetClass().Get());
    }
    UE_LOG(LogRoundSkillMenu, Log, TEXT("SkillMenu NativeConstruct - MainMenu=%s MenuHolder=%s Selector=%s StartPosition=%s SharedMenu=%s"),
        *GetNameSafe(MainMenu),
        *GetNameSafe(MenuHolder),
        *GetNameSafe(SkillmenuSelector),
        *GetNameSafe(UI_StartPosition_0),
        *GetNameSafe(GetSharedSkillpointMenu()));

    if (Button_0)
    {
        Button_0->OnClicked.AddDynamic(this, &URoundSkillMenuWidget::HandleBackClicked);
    }
    if (Button_SkillpointsTab)
    {
        Button_SkillpointsTab->OnClicked.AddDynamic(this, &URoundSkillMenuWidget::HandleSkillpointsTabClicked);
    }
    if (Button_StartPositionTab)
    {
        Button_StartPositionTab->OnClicked.AddDynamic(this, &URoundSkillMenuWidget::HandleStartPositionTabClicked);
    }
    if (ContentSwitcher)
    {
        ContentSwitcher->SetActiveWidgetIndex(0);
    }
    if (ComboBoxKey_0)
    {
        ComboBoxKey_0->OnGenerateItemWidget.BindDynamic(this, &URoundSkillMenuWidget::HandleGenerateCharacterClassWidget);
        ComboBoxKey_0->OnGenerateContentWidget.BindDynamic(this, &URoundSkillMenuWidget::HandleGenerateCharacterClassWidget);
        ComboBoxKey_0->OnSelectionChanged.AddDynamic(this, &URoundSkillMenuWidget::HandleCharacterClassChanged);
    }
    if (UI_StartPosition_0)
    {
        UI_StartPosition_0->SetMainMenuWidget(MainMenu);
    }

    LoadSkillpoints();
}

void URoundSkillMenuWidget::SetMainMenuWidget(URoundMainMenuWidget* InMainMenu)
{
    MainMenu = InMainMenu;
    if (UI_StartPosition_0)
    {
        UI_StartPosition_0->SetMainMenuWidget(MainMenu);
    }
    LoadSkillpoints();
}

void URoundSkillMenuWidget::SelectCharacter(const int32 CharacterIndex)
{
    if (!MainMenu)
    {
        return;
    }

    MainMenu->SetSelectedCharacterIndex(CharacterIndex);
    SelectedSkillIndex = 0;
    RefreshCharacterClassCombo();
    RefreshSkillSelector();
    RefreshSkillpointCounter();
    RefreshSkillpointMenu();
    if (UI_StartPosition_0)
    {
        UI_StartPosition_0->Refresh();
    }
    BP_OnSkillMenuRefreshed();
}

void URoundSkillMenuWidget::SetSkillpointMenu(UWidget* Content)
{
    if (!IsValid(MenuHolder) || !IsValid(Content))
    {
        UE_LOG(LogRoundSkillMenu, Warning, TEXT("SetSkillpointMenu skipped - MenuHolder=%s Content=%s"),
            *GetNameSafe(MenuHolder), *GetNameSafe(Content));
        return;
    }

    MenuHolder->SetVisibility(ESlateVisibility::Visible);
    Content->SetVisibility(ESlateVisibility::Visible);
    Content->SetRenderOpacity(1.f);
    if (Content->GetParent() != MenuHolder)
    {
        Content->RemoveFromParent();
        MenuHolder->ClearChildren();
        MenuHolder->AddChild(Content);
    }
    CurrentContentWidget = Content;
    if (ContentSwitcher)
    {
        ContentSwitcher->SetActiveWidgetIndex(0);
    }
    UE_LOG(LogRoundSkillMenu, Log, TEXT("SetSkillpointMenu added content - Content=%s ParentChildren=%d"),
        *GetNameSafe(Content), MenuHolder->GetChildrenCount());

    if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Content->Slot))
    {
        PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        PanelSlot->SetOffsets(FMargin(0.f));
        UE_LOG(LogRoundSkillMenu, Log, TEXT("SetSkillpointMenu canvas slot applied for %s"), *GetNameSafe(Content));
    }
    else
    {
        UE_LOG(LogRoundSkillMenu, Warning, TEXT("SetSkillpointMenu no canvas slot for %s, slot class=%s"),
            *GetNameSafe(Content), Content->Slot ? *Content->Slot->GetClass()->GetName() : TEXT("None"));
    }
}

void URoundSkillMenuWidget::LoadSkillpoints()
{
    RefreshCharacterClassCombo();
    RefreshSkillSelector();
    RefreshSkillpointCounter();
    RefreshSkillpointMenu();
    if (UI_StartPosition_0)
    {
        UI_StartPosition_0->Refresh();
    }
    BP_OnSkillMenuRefreshed();
}

void URoundSkillMenuWidget::RefreshAfterSkillpointChange()
{
    RefreshSkillSelector();
    RefreshSkillpointCounter();
    RefreshSkillpointMenu();
    if (UI_StartPosition_0)
    {
        UI_StartPosition_0->Refresh();
    }
    BP_OnSkillMenuRefreshed();
}

bool URoundSkillMenuWidget::TryGetSelectedCharacter(FRoundCharacterEntry& OutCharacter) const
{
    return MainMenu && MainMenu->TryGetEditedCharacter(GetSelectedCharacterIndex(), OutCharacter);
}

bool URoundSkillMenuWidget::CommitSelectedCharacter(const FRoundCharacterEntry& InCharacter, const bool bRefreshSelection)
{
    return MainMenu && MainMenu->UpdateEditedCharacter(GetSelectedCharacterIndex(), InCharacter, bRefreshSelection);
}

int32 URoundSkillMenuWidget::GetSelectedCharacterIndex() const
{
    return MainMenu ? MainMenu->GetCurrentSelectedCharacterIndex() : INDEX_NONE;
}

void URoundSkillMenuWidget::SetSelectedSkillIndex(const int32 InSelectedSkillIndex)
{
    SelectedSkillIndex = FMath::Max(0, InSelectedSkillIndex);
}

int32 URoundSkillMenuWidget::GetRemainingTeamSkillPoints() const
{
    return MainMenu ? MainMenu->GetRemainingTeamSkillPoints() : 0;
}

int32 URoundSkillMenuWidget::GetCurrentCharacterSkillPoints() const
{
    FRoundCharacterEntry Character;
    if (!TryGetSelectedCharacter(Character))
    {
        return 0;
    }

    return static_cast<int32>(Character.CharacterSkillpoints);
}

int32 URoundSkillMenuWidget::GetCurrentCharacterLearnedSkillCount() const
{
    return GetAvailableAbilities().Num();
}

int32 URoundSkillMenuWidget::GetCurrentCharacterSkillLimit() const
{
    return GetAvailableAbilities().Num();
}

bool URoundSkillMenuWidget::CanLearnAdditionalSkill() const
{
    return false;
}

URoundAbilityData* URoundSkillMenuWidget::FindAbilityDataBySkillName(const FString& InSkillName) const
{
    for (URoundAbilityData* AbilityData : GetAvailableAbilities())
    {
        if (!AbilityData)
        {
            continue;
        }

        const FString AbilityId = AbilityData->AbilityId.IsNone() ? AbilityData->GetPrimaryAssetId().PrimaryAssetName.ToString() : AbilityData->AbilityId.ToString();
        if (AbilityId.Equals(InSkillName, ESearchCase::IgnoreCase))
        {
            return AbilityData;
        }
    }

    return nullptr;
}

void URoundSkillMenuWidget::ShowSkill(const FString& InSkillName, const FRoundSkillDetails& InSkillDetail)
{
    URoundSkillpointMenuWidget* SharedMenu = GetSharedSkillpointMenu();
    if (!SharedMenu)
    {
        UE_LOG(LogRoundSkillMenu, Warning, TEXT("ShowSkill fallback needed - no shared menu for %s"), *InSkillName);

        TSubclassOf<URoundSkillpointMenuWidget> MenuClass = SkillpointMenuClass;
        if (!MenuClass)
        {
            MenuClass = Cast<UClass>(RoundGameDataRegistryLookup::ResolveSkillpointMenuWidgetClass().Get());
        }
        if (!MenuClass)
        {
            MenuClass = URoundSkillpointMenuWidget::StaticClass();
        }

        SharedMenu = CreateWidget<URoundSkillpointMenuWidget>(this, MenuClass);
        if (!SharedMenu)
        {
            UE_LOG(LogRoundSkillMenu, Warning, TEXT("ShowSkill failed to create fallback menu for %s"), *InSkillName);
            return;
        }

        UE_LOG(LogRoundSkillMenu, Log, TEXT("ShowSkill created fallback menu %s from class %s for %s"),
            *GetNameSafe(SharedMenu), *GetNameSafe(MenuClass), *InSkillName);
    }

    SharedMenu->SkillMenu = this;
    SharedMenu->SkillName = InSkillName;
    SharedMenu->SkillDetail = InSkillDetail;
    SetSkillpointMenu(SharedMenu);
    SharedMenu->ReloadPoints();
    UE_LOG(LogRoundSkillMenu, Log, TEXT("ShowSkill using shared menu - Skill=%s Menu=%s"), *InSkillName, *GetNameSafe(SharedMenu));
}

void URoundSkillMenuWidget::HandleCharacterClassChanged(const FName SelectedItem, const ESelectInfo::Type SelectionType)
{
    if (!MainMenu || SelectionType == ESelectInfo::Direct)
    {
        return;
    }

    FRoundCharacterEntry Character;
    if (!MainMenu->TryGetEditedCharacter(GetSelectedCharacterIndex(), Character))
    {
        return;
    }

    Character.Character = SelectedItem.ToString();
    Character.Skills.Reset();
    Character.Skillpoints = 0;
    Character.CharacterSkillpoints = 0;
    Character.CharacterSkillSelections = 0;
    MainMenu->UpdateEditedCharacter(GetSelectedCharacterIndex(), Character, true);
    SelectedSkillIndex = 0;
    RefreshCharacterClassCombo();
    RefreshSkillSelector();
    RefreshSkillpointCounter();
    RefreshSkillpointMenu();
    if (UI_StartPosition_0)
    {
        UI_StartPosition_0->Refresh();
    }
}

void URoundSkillMenuWidget::HandleBackClicked()
{
    RemoveFromParent();
}

void URoundSkillMenuWidget::HandleSkillpointsTabClicked()
{
    if (ContentSwitcher)
    {
        ContentSwitcher->SetActiveWidgetIndex(0);
    }
}

void URoundSkillMenuWidget::HandleStartPositionTabClicked()
{
    if (ContentSwitcher)
    {
        ContentSwitcher->SetActiveWidgetIndex(1);
    }
    if (UI_StartPosition_0)
    {
        UI_StartPosition_0->Refresh();
    }
}

UWidget* URoundSkillMenuWidget::HandleGenerateCharacterClassWidget(FName Item)
{
    UTextBlock* Label = NewObject<UTextBlock>(this);
    if (Label)
    {
        Label->SetText(FText::FromString(Item.ToString()));
    }
    return Label;
}

void URoundSkillMenuWidget::RefreshSkillSelector()
{
    if (!SkillmenuSelector)
    {
        UE_LOG(LogRoundSkillMenu, Warning, TEXT("RefreshSkillSelector skipped - SkillmenuSelector missing"));
        return;
    }

    SkillmenuSelector->ClearChildren();
    CharacterSkillpointMenuWidget = nullptr;

    TSubclassOf<URoundSkillmenuSelectorPointWidget> SelectorClass = SkillSelectorPointClass;
    if (!SelectorClass)
    {
        SelectorClass = URoundSkillmenuSelectorPointWidget::StaticClass();
    }

    TSubclassOf<URoundCharacterSkillpointMenuWidget> CharacterSkillpointMenuClass = URoundCharacterSkillpointMenuWidget::StaticClass();
    URoundCharacterData* SelectedCharacterData = nullptr;
    if (TryGetSelectedCharacterData(SelectedCharacterData) && SelectedCharacterData && SelectedCharacterData->CharacterSkillpointMenuClass)
    {
        if (UClass* CharacterMenuClass = SelectedCharacterData->CharacterSkillpointMenuClass.Get())
        {
            if (CharacterMenuClass->IsChildOf(URoundCharacterSkillpointMenuWidget::StaticClass()))
            {
                CharacterSkillpointMenuClass = CharacterMenuClass;
            }
            else
            {
                UE_LOG(LogRoundSkillMenu, Error, TEXT("CharacterSkillpointMenuClass is not a RoundCharacterSkillpointMenuWidget - Character=%s Class=%s"),
                    *SelectedCharacterData->GetName(),
                    *GetNameSafe(CharacterMenuClass));
            }
        }
    }

    CharacterSkillpointMenuWidget = CreateWidget<URoundCharacterSkillpointMenuWidget>(this, CharacterSkillpointMenuClass);
    if (CharacterSkillpointMenuWidget)
    {
        FString PassiveLabel = TEXT("Character");
        FRoundSkillDetails PassiveDetails;
        PassiveDetails.Name = TEXT("Character");
        PassiveDetails.Description = TEXT("Character upgrades");
        if (SelectedCharacterData)
        {
            if (!SelectedCharacterData->DisplayName.IsEmpty())
            {
                PassiveLabel = SelectedCharacterData->DisplayName.ToString();
            }
            else if (!SelectedCharacterData->CharacterId.IsNone())
            {
                PassiveLabel = SelectedCharacterData->CharacterId.ToString();
            }

            PassiveDetails.Name = PassiveLabel;
            PassiveDetails.SkillpointDescription = nullptr;
        }

        CharacterSkillpointMenuWidget->SkillMenu = this;
        CharacterSkillpointMenuWidget->SkillName = PassiveLabel;
        CharacterSkillpointMenuWidget->SkillDetail = PassiveDetails;

        if (URoundSkillmenuSelectorPointWidget* PassiveSelector = CreateWidget<URoundSkillmenuSelectorPointWidget>(this, SelectorClass))
        {
            PassiveSelector->Skillmenu = this;
            PassiveSelector->SkillpointMenu = CharacterSkillpointMenuWidget;
            PassiveSelector->SkillName = PassiveLabel;
            PassiveSelector->SkillDetail = PassiveDetails;
            PassiveSelector->SelectorIndex = 0;
            SkillmenuSelector->AddChild(PassiveSelector);

            if (SelectedSkillIndex == 0)
            {
                CharacterSkillpointMenuWidget->SkillMenu = this;
                CharacterSkillpointMenuWidget->SkillName = PassiveLabel;
                CharacterSkillpointMenuWidget->SkillDetail = PassiveDetails;
                SetSkillpointMenu(CharacterSkillpointMenuWidget);
                CharacterSkillpointMenuWidget->ReloadPoints();
                PassiveSelector->SetLearned(true);
            }
        }
    }

    const TArray<URoundAbilityData*> Abilities = GetAvailableAbilities();
    UE_LOG(LogRoundSkillMenu, Log, TEXT("RefreshSkillSelector - abilities=%d"), Abilities.Num());
    if (Abilities.Num() == 0 && CharacterSkillpointMenuWidget)
    {
        return;
    }

    if (CharacterSkillpointMenuWidget)
    {
        if (USpacer* Spacer = NewObject<USpacer>(this))
        {
            Spacer->SetSize(FVector2D(1.f, 10.f));
            SkillmenuSelector->AddChild(Spacer);
            if (UVerticalBoxSlot* SpacerSlot = Cast<UVerticalBoxSlot>(Spacer->Slot))
            {
                SpacerSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 4.f));
            }
        }
    }

    SelectedSkillIndex = FMath::Clamp(SelectedSkillIndex, 0, FMath::Max(0, Abilities.Num()));

    for (int32 AbilityIndex = 0; AbilityIndex < Abilities.Num(); ++AbilityIndex)
    {
        URoundAbilityData* AbilityData = Abilities[AbilityIndex];
        if (!AbilityData)
        {
            continue;
        }

        const FString AbilityName = AbilityData->AbilityId.IsNone() ? AbilityData->GetPrimaryAssetId().PrimaryAssetName.ToString() : AbilityData->AbilityId.ToString();
        FRoundSkillDetails SkillDetails;
        SkillDetails.Name = AbilityName;
        SkillDetails.Description = AbilityData->Description.ToString();
        if (SkillDetails.Description.IsEmpty() && AbilityData->SkillProgression)
        {
            SkillDetails.Description = AbilityData->SkillProgression->Description.ToString();
        }
        if (SkillDetails.Description.IsEmpty())
        {
            SkillDetails.Description = FString::Printf(TEXT("%s: Fuer diese Faehigkeit ist noch keine Beschreibung hinterlegt."), *AbilityName);
        }
        SkillDetails.Values = AbilityData->Values;
        SkillDetails.DamageType = AbilityData->DamageType;

        URoundSkillpointMenuWidget* SkillpointMenuWidget = GetSharedSkillpointMenu();
        if (!SkillpointMenuWidget)
        {
            TSubclassOf<URoundSkillpointMenuWidget> MenuClass = SkillpointMenuClass;
            if (!MenuClass)
            {
                MenuClass = Cast<UClass>(RoundGameDataRegistryLookup::ResolveSkillpointMenuWidgetClass().Get());
            }
            if (!MenuClass)
            {
                MenuClass = URoundSkillpointMenuWidget::StaticClass();
            }

            SkillpointMenuWidget = CreateWidget<URoundSkillpointMenuWidget>(this, MenuClass);
            if (!SkillpointMenuWidget)
            {
                UE_LOG(LogRoundSkillMenu, Warning, TEXT("Failed to create SkillpointMenuWidget for ability index %d"), AbilityIndex);
                continue;
            }
            UE_LOG(LogRoundSkillMenu, Log, TEXT("Created SkillpointMenuWidget from class %s for %s"),
                *GetNameSafe(MenuClass), *AbilityName);
        }

        SkillpointMenuWidget->SkillMenu = this;
        SkillpointMenuWidget->SkillName = AbilityName;
        SkillpointMenuWidget->SkillDetail = SkillDetails;

        URoundSkillmenuSelectorPointWidget* Selector = CreateWidget<URoundSkillmenuSelectorPointWidget>(this, SelectorClass);
        if (!Selector)
        {
            UE_LOG(LogRoundSkillMenu, Warning, TEXT("Failed to create Selector for ability %s"), *SkillpointMenuWidget->SkillName);
            continue;
        }

        Selector->Skillmenu = this;
        Selector->SkillpointMenu = SkillpointMenuWidget;
        Selector->SkillName = !AbilityData->DisplayName.IsEmpty() ? AbilityData->DisplayName.ToString() : AbilityName;
        Selector->SkillDetail = SkillDetails;
        const int32 SelectorIndex = AbilityIndex + 1;
        Selector->SelectorIndex = SelectorIndex;
        SkillmenuSelector->AddChild(Selector);
        if (SelectorIndex == SelectedSkillIndex)
        {
            UE_LOG(LogRoundSkillMenu, Log, TEXT("Selecting ability %s at index %d"), *AbilityName, SelectorIndex);
            ShowSkill(AbilityName, SkillDetails);
            Selector->SetLearned(SkillpointMenuWidget->CurrentSkillLearned());
        }
    }

    UE_LOG(LogRoundSkillMenu, Log, TEXT("RefreshSkillSelector done - selector children=%d currentContent=%s"),
        SkillmenuSelector->GetChildrenCount(), *GetNameSafe(CurrentContentWidget));
}

void URoundSkillMenuWidget::RefreshCharacterClassCombo()
{
    if (!ComboBoxKey_0)
    {
        return;
    }

    FRoundCharacterEntry Character;
    ComboBoxKey_0->ClearOptions();

    TArray<FName> AvailableCharacterIds;
    if (MainMenu)
    {
        AvailableCharacterIds = MainMenu->GetAvailableChampionIds();
    }
    if (AvailableCharacterIds.Num() == 0)
    {
        RoundCharacterRegistry::GetCharacterIds(AvailableCharacterIds, false);
    }

    for (const FName CharacterId : AvailableCharacterIds)
    {
        ComboBoxKey_0->AddOption(CharacterId);
    }

    if (!TryGetSelectedCharacter(Character) || Character.Character.IsEmpty())
    {
        return;
    }

    const FName NormalizedId = NormalizeCharacterId(Character.Character);
    ComboBoxKey_0->SetSelectedOption(NormalizedId);
}

void URoundSkillMenuWidget::RefreshSkillpointCounter()
{
    if (SkillpointCounter)
    {
        SkillpointCounter->SetText(FText::AsNumber(GetRemainingTeamSkillPoints()));
    }
}

void URoundSkillMenuWidget::RefreshSkillpointMenu()
{
    if (!MenuHolder)
    {
        return;
    }

    if (!CurrentContentWidget)
    {
        MenuHolder->ClearChildren();
        return;
    }

    if (URoundSkillpointMenuWidget* SkillMenuWidget = Cast<URoundSkillpointMenuWidget>(CurrentContentWidget))
    {
        SkillMenuWidget->ReloadPoints();
    }
}

bool URoundSkillMenuWidget::TryGetSelectedCharacterData(URoundCharacterData*& OutCharacterData) const
{
    OutCharacterData = nullptr;

    FRoundCharacterEntry Character;
    if (!TryGetSelectedCharacter(Character) || Character.Character.IsEmpty())
    {
        return false;
    }

    const FName NormalizedId = NormalizeCharacterId(Character.Character);
    OutCharacterData = RoundCharacterRegistry::FindCharacterDataById(NormalizedId);

    if (!OutCharacterData)
    {
        UE_LOG(LogRoundSkillMenu, Error, TEXT("TryGetSelectedCharacterData failed - Character=%s Normalized=%s"),
            *Character.Character,
            *NormalizedId.ToString());
    }

    return OutCharacterData != nullptr;
}

TArray<URoundAbilityData*> URoundSkillMenuWidget::GetAvailableAbilities() const
{
    URoundCharacterData* CharacterData = nullptr;
    if (TryGetSelectedCharacterData(CharacterData) && CharacterData && CharacterData->Abilities.Num() > 0)
    {
        TArray<URoundAbilityData*> Abilities;
        for (URoundAbilityData* AbilityData : CharacterData->Abilities)
        {
            if (AbilityData)
            {
                Abilities.Add(AbilityData);
            }
        }

        if (Abilities.Num() > 0)
        {
            return Abilities;
        }
    }
    
    UE_LOG(LogRoundSkillMenu, Error, TEXT("GetAvailableAbilities found no abilities in CharacterData for selected character."));
    return {};
}

URoundSkillpointMenuWidget* URoundSkillMenuWidget::GetSharedSkillpointMenu() const
{
    if (SkillpointMenu_0)
    {
        return SkillpointMenu_0;
    }
    if (SkillpointMenu)
    {
        return SkillpointMenu;
    }
    return nullptr;
}

FName URoundSkillMenuWidget::NormalizeCharacterId(const FString& InCharacterId)
{
    return RoundCharacterRegistry::NormalizeCharacterId(InCharacterId);
}

FSoftObjectPath URoundSkillMenuWidget::BuildCharacterDataPath(const FName CharacterId)
{
    const TSoftObjectPtr<URoundCharacterData> CharacterDataAsset = RoundCharacterRegistry::FindCharacterDataAssetById(CharacterId);
    if (CharacterDataAsset.ToSoftObjectPath().IsValid())
    {
        return CharacterDataAsset.ToSoftObjectPath();
    }

    return FSoftObjectPath();
}
