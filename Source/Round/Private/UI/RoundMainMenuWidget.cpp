#include "UI/RoundMainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableText.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ListView.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/RoundCharacterRegistryLookup.h"
#include "Data/RoundGameDataRegistryLookup.h"
#include "Game/RoundGameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SaveGames/RoundSaveGame.h"
#include "UI/RoundCharacterSelectionOptionWidget.h"
#include "UI/RoundMenuListEntryWidgets.h"
#include "UI/RoundSessionListItem.h"
#include "UI/RoundRoomSessionButton.h"
#include "UI/RoundSkillMenuWidget.h"

namespace
{
constexpr TCHAR SaveSlotName[] = TEXT("Saves");
const FName RoomNameSetting(TEXT("RoundRoomName"));
int32 CountTierSelections(const int64 PackedSelections)
{
    int32 Count = 0;
    for (int32 TierIndex = 0; TierIndex < 4; ++TierIndex)
    {
        if (((PackedSelections >> (TierIndex * 2)) & 0x3) != 0)
        {
            ++Count;
        }
    }
    return Count;
}
void SetPanelVisible(UCanvasPanel* Panel, const bool bVisible)
{
    if (Panel)
    {
        Panel->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void SetRoomPanelVisible(UVerticalBox* Panel, const bool bVisible)
{
    if (!Panel)
    {
        return;
    }

    UWidget* Container = Panel;
    if (UPanelWidget* Card = Container->GetParent())
    {
        Container = Card;
        if (UPanelWidget* SizeBox = Container->GetParent())
        {
            Container = SizeBox;
        }
    }
    Container->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
}

URoundMainMenuWidget::URoundMainMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URoundMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!TeamEntryClass)
    {
        TeamEntryClass = Cast<UClass>(RoundGameDataRegistryLookup::ResolveTeamEntryWidgetClass().Get());
    }
    if (!CharacterSelectionOptionClass)
    {
        CharacterSelectionOptionClass = Cast<UClass>(RoundGameDataRegistryLookup::ResolveCharacterSelectionOptionWidgetClass().Get());
    }
    if (!SkillMenuClass)
    {
        SkillMenuClass = Cast<UClass>(RoundGameDataRegistryLookup::ResolveSkillMenuWidgetClass().Get());
    }

    if (AvailableChampionIds.Num() == 0)
    {
        RoundCharacterRegistry::GetCharacterIds(AvailableChampionIds, false);
    }

    if (Button)
    {
        Button->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleRefreshSessionsClicked);
    }
    if (Button_0)
    {
        Button_0->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleCreateSessionClicked);
    }
    if (Button_1)
    {
        Button_1->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleJoinSessionClicked);
    }
    if (Button_EditTeams)
    {
        Button_EditTeams->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleOpenTeamsClicked);
    }
    if (Button_EditTeams_1)
    {
        Button_EditTeams_1->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleOpenEditTeamsClicked);
    }
    if (Button_Back)
    {
        Button_Back->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleBackClicked);
    }
    if (Button_EditorBack)
    {
        Button_EditorBack->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleOpenTeamsClicked);
    }
    if (Button_SaveTeam)
    {
        Button_SaveTeam->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleSaveTeamClicked);
    }
    if (Button_Delete_Team)
    {
        Button_Delete_Team->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleDeleteSelectedTeamClicked);
    }
    if (Button_Delete_Team_1)
    {
        Button_Delete_Team_1->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleDeleteEditedTeamClicked);
    }
    if (Button_Delete_Character)
    {
        Button_Delete_Character->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleDeleteCharacterClicked);
    }
    if (Button_Add_Character)
    {
        Button_Add_Character->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleAddCharacterClicked);
    }
    if (Button_Add_Team)
    {
        Button_Add_Team->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleAddTeamClicked);
    }
    if (Button_Add_Team_1)
    {
        Button_Add_Team_1->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleAddTeamAndEditClicked);
    }
    if (ComboBoxString_1)
    {
        ComboBoxString_1->OnSelectionChanged.AddDynamic(this, &URoundMainMenuWidget::HandleFormatSelectionChanged);
    }
    if (ComboBoxString_0)
    {
        ComboBoxString_0->OnSelectionChanged.AddDynamic(this, &URoundMainMenuWidget::HandleSelectedTeamChanged);
    }
    if (EditableText_TeamName)
    {
        EditableText_TeamName->OnTextChanged.AddDynamic(this, &URoundMainMenuWidget::HandleTeamNameChanged);
    }
    if (ListView_0)
    {
        ListView_0->OnItemClicked().AddUObject(this, &URoundMainMenuWidget::HandleSessionItemClicked);
    }
    if (Button_ShowCreateRoom) Button_ShowCreateRoom->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleShowCreateRoomClicked);
    if (Button_ShowBrowseRooms) Button_ShowBrowseRooms->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleShowBrowseRoomsClicked);
    if (Button_RoomBack) Button_RoomBack->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleBackToRoomHomeClicked);
    if (Button_BrowseBack) Button_BrowseBack->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleBackToRoomHomeClicked);
    if (Button_Quit) Button_Quit->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleQuitClicked);
    if (Button_StartGame) Button_StartGame->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleStartGameClicked);
    if (Button_JoinDirect) Button_JoinDirect->OnClicked.AddDynamic(this, &URoundMainMenuWidget::HandleJoinDirectClicked);
    if (EditableText_RoomName) EditableText_RoomName->OnTextChanged.AddDynamic(this, &URoundMainMenuWidget::HandleRoomNameChanged);
    if (EditableText_DirectAddress) EditableText_DirectAddress->OnTextChanged.AddDynamic(this, &URoundMainMenuWidget::HandleDirectAddressChanged);
    if (ComboBox_RoomTeam) ComboBox_RoomTeam->OnSelectionChanged.AddDynamic(this, &URoundMainMenuWidget::HandleSelectedTeamChanged);

    LoadOptions();
    RefreshRoomControls();
    if (URoundGameInstance* GameInstance = GetRoundGameInstance(); GameInstance && GameInstance->bJoinedRoom)
    {
        ShowRoomMenuView(ERoundRoomMenuView::Lobby);
    }
    else
    {
        ShowRoomMenuView(ERoundRoomMenuView::Home);
    }
    EnsureSkillMenu();
}

void URoundMainMenuWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (CurrentRoomView != ERoundRoomMenuView::Lobby)
    {
        return;
    }

    LobbyRefreshAccumulator += InDeltaTime;
    if (LobbyRefreshAccumulator >= 0.5f)
    {
        LobbyRefreshAccumulator = 0.f;
        RefreshLobbyPlayers();
    }
}

void URoundMainMenuWidget::TeamIndexSelected(const int32 InSelectedTeamIndex)
{
    SelectedTeamIndex = InSelectedTeamIndex;
}

void URoundMainMenuWidget::SetSelectedCharacterIndex(const int32 InSelectedCharacterIndex)
{
    SelectedCharacterIndex = InSelectedCharacterIndex;
    LoadSelectedCharacter();
    EnsureSkillMenu();
}

void URoundMainMenuWidget::LoadTeams()
{
    if (!VerticalBox_Teams)
    {
        RefreshTeamSelectionOptions();
        return;
    }

    VerticalBox_Teams->ClearChildren();

    for (int32 Index = 0; Index < Teams.Num(); ++Index)
    {
        TSubclassOf<URoundTeamsListEntryWidget> EntryClass;
        if (TeamEntryClass)
        {
            EntryClass = TeamEntryClass;
        }
        else
        {
            EntryClass = URoundTeamsListEntryWidget::StaticClass();
        }

        URoundTeamsListEntryWidget* Entry = CreateWidget<URoundTeamsListEntryWidget>(this, EntryClass);
        if (!Entry)
        {
            continue;
        }

        Entry->MainMenu = this;
        Entry->Index = Index;
        Entry->SetTeamName(Teams[Index].Name);
        VerticalBox_Teams->AddChild(Entry);
    }

    RefreshTeamSelectionOptions();
}

void URoundMainMenuWidget::SelectTeamToEdit()
{
    if (!Teams.IsValidIndex(SelectedTeamIndex))
    {
        return;
    }

    EditTeam = Teams[SelectedTeamIndex];
    SelectedCharacterIndex = EditTeam.Characters.IsEmpty() ? INDEX_NONE : 0;

    if (EditableText_TeamName)
    {
        EditableText_TeamName->SetText(FText::FromString(EditTeam.Name));
    }
    if (ComboBoxString_1)
    {
        ComboBoxString_1->SetSelectedOption(FormatToString(EditTeam.Format));
    }

    LoadEditTeamCharacters();
    LoadSelectedCharacter();
    EnsureSkillMenu();
    ShowTeamEditorPanel();
}

bool URoundMainMenuWidget::ValidateTeam(FText& OutError) const
{
    if (EditTeam.Name.TrimStartAndEnd().IsEmpty())
    {
        OutError = FText::FromString(TEXT("Teamname fehlt."));
        return false;
    }

    FRoundFormatDefinition FormatDefinition;
    const URoundGameInstance* GameInstance = GetRoundGameInstance();
    const bool bHasFormat = GameInstance && GameInstance->TryGetFormatDefinition(EditTeam.Format, FormatDefinition);

    if (EditTeam.Characters.Num() == 0)
    {
        OutError = FText::FromString(TEXT("Das Team hat keine Charaktere."));
        return false;
    }

    if (bHasFormat && EditTeam.Characters.Num() > FormatDefinition.Characters)
    {
        OutError = FText::FromString(TEXT("Zu viele Charaktere fuer das gewaehlte Format."));
        return false;
    }

    for (const FRoundCharacterEntry& Character : EditTeam.Characters)
    {
        if (Character.Character.TrimStartAndEnd().IsEmpty())
        {
            OutError = FText::FromString(TEXT("Mindestens ein Charakterslot ist leer."));
            return false;
        }

    }

    OutError = FText::GetEmpty();
    return true;
}

void URoundMainMenuWidget::FindSessions()
{
    const IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
    if (!SessionInterface.IsValid())
    {
        SetMenuStatus(FText::FromString(TEXT("Session-Interface nicht verfuegbar.")));
        return;
    }

    SessionSearch = MakeShared<FOnlineSessionSearch>();
    SessionSearch->MaxSearchResults = 100;
    SessionSearch->bIsLanQuery = UsesLanSessions();
    SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

    if (FindSessionsCompleteHandle.IsValid())
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
    }

    FindSessionsCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
        FOnFindSessionsCompleteDelegate::CreateUObject(this, &URoundMainMenuWidget::HandleFindSessionsComplete));

    const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
    const int32 ControllerId = LocalPlayer ? LocalPlayer->GetControllerId() : 0;
    if (!SessionInterface->FindSessions(ControllerId, SessionSearch.ToSharedRef()))
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
        FindSessionsCompleteHandle.Reset();
        SetMenuStatus(FText::FromString(TEXT("Session-Suche konnte nicht gestartet werden.")));
        return;
    }

    SetMenuStatus(FText::FromString(TEXT("Suche nach Sessions...")));
}

bool URoundMainMenuWidget::TryGetCurrentFormatDefinition(FRoundFormatDefinition& OutFormat) const
{
    const URoundGameInstance* GameInstance = GetRoundGameInstance();
    return GameInstance && GameInstance->TryGetFormatDefinition(EditTeam.Format, OutFormat);
}

int32 URoundMainMenuWidget::GetRemainingTeamSkillPoints() const
{
    FRoundFormatDefinition FormatDefinition;
    int32 TotalBudget = 0;
    if (TryGetCurrentFormatDefinition(FormatDefinition))
    {
        TotalBudget = FormatDefinition.Skillpoints;
    }

    FRoundCharacterEntry SelectedCharacter;
    if (!TryGetEditedCharacter(SelectedCharacterIndex, SelectedCharacter))
    {
        return TotalBudget;
    }

    int32 UsedPoints = CountTierSelections(SelectedCharacter.CharacterSkillSelections);
    for (const FRoundSkillEntry& Skill : SelectedCharacter.Skills)
    {
        UsedPoints += CountTierSelections(Skill.Points);
    }

    const int32 RemainingPoints = FMath::Max(0, TotalBudget - UsedPoints);
    UE_LOG(LogTemp, Log,
        TEXT("Team skill budget - CharacterIndex=%d Character=%s Budget=%d UsedFromMasks=%d StoredSkill=%lld StoredCharacter=%lld PassiveMask=%lld Skills=%d Remaining=%d"),
        SelectedCharacterIndex, *SelectedCharacter.Character, TotalBudget, UsedPoints,
        SelectedCharacter.Skillpoints, SelectedCharacter.CharacterSkillpoints,
        SelectedCharacter.CharacterSkillSelections, SelectedCharacter.Skills.Num(), RemainingPoints);
    return RemainingPoints;
}

bool URoundMainMenuWidget::TryGetEditedCharacter(const int32 CharacterIndex, FRoundCharacterEntry& OutCharacter) const
{
    if (!EditTeam.Characters.IsValidIndex(CharacterIndex))
    {
        return false;
    }

    OutCharacter = EditTeam.Characters[CharacterIndex];
    return true;
}

bool URoundMainMenuWidget::UpdateEditedCharacter(const int32 CharacterIndex, const FRoundCharacterEntry& InCharacter, const bool bRefreshSelection)
{
    if (!EditTeam.Characters.IsValidIndex(CharacterIndex))
    {
        return false;
    }

    EditTeam.Characters[CharacterIndex] = InCharacter;
    RefreshEditedTeamViews();

    if (bRefreshSelection)
    {
        SelectedCharacterIndex = CharacterIndex;
        LoadSelectedCharacter();
    }

    return true;
}

void URoundMainMenuWidget::RefreshEditedTeamViews()
{
    RefreshTeamCounter();
    LoadEditTeamCharacters();
}

void URoundMainMenuWidget::HandleRefreshSessionsClicked()
{
    FindSessions();
}

void URoundMainMenuWidget::HandleCreateSessionClicked()
{
    const IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
    if (!SessionInterface.IsValid())
    {
        SetMenuStatus(FText::FromString(TEXT("Session-Interface nicht verfuegbar.")));
        return;
    }

    if (CreateSessionCompleteHandle.IsValid())
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
    }

    CreateSessionCompleteHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
        FOnCreateSessionCompleteDelegate::CreateUObject(this, &URoundMainMenuWidget::HandleCreateSessionComplete));

    FOnlineSessionSettings Settings;
    Settings.NumPublicConnections = 2;
    Settings.bShouldAdvertise = true;
    Settings.bAllowJoinInProgress = true;
    Settings.bIsLANMatch = UsesLanSessions();
    Settings.bUsesPresence = true;
    Settings.bAllowJoinViaPresence = true;
    Settings.bUseLobbiesIfAvailable = true;
    Settings.Set(SEARCH_PRESENCE, true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
    const URoundGameInstance* GameInstance = GetRoundGameInstance();
    const FString RoomName = GameInstance && !GameInstance->RoomName.IsEmpty() ? GameInstance->RoomName : TEXT("Test Room");
    Settings.Set(RoomNameSetting, RoomName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

    const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
    const int32 ControllerId = LocalPlayer ? LocalPlayer->GetControllerId() : 0;
    if (!SessionInterface->CreateSession(ControllerId, NAME_GameSession, Settings))
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
        CreateSessionCompleteHandle.Reset();
        SetMenuStatus(FText::FromString(TEXT("Session konnte nicht erstellt werden.")));
        return;
    }

    SetMenuStatus(FText::FromString(TEXT("Erstelle Session...")));
}

void URoundMainMenuWidget::HandleJoinSessionClicked()
{
    const IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
    if (!SessionInterface.IsValid())
    {
        SetMenuStatus(FText::FromString(TEXT("Session-Interface nicht verfuegbar.")));
        return;
    }

    if (!SelectedSessionItem || !FoundSessions.IsValidIndex(SelectedSessionItem->SessionIndex))
    {
        SetMenuStatus(FText::FromString(TEXT("Keine Session ausgewaehlt.")));
        return;
    }

    if (JoinSessionCompleteHandle.IsValid())
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
    }

    JoinSessionCompleteHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
        FOnJoinSessionCompleteDelegate::CreateUObject(this, &URoundMainMenuWidget::HandleJoinSessionComplete));

    const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
    const int32 ControllerId = LocalPlayer ? LocalPlayer->GetControllerId() : 0;
    if (!SessionInterface->JoinSession(ControllerId, NAME_GameSession, FoundSessions[SelectedSessionItem->SessionIndex]))
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
        JoinSessionCompleteHandle.Reset();
        SetMenuStatus(FText::FromString(TEXT("Join konnte nicht gestartet werden.")));
        return;
    }

    SetMenuStatus(FText::FromString(FString::Printf(TEXT("Trete Session %s bei..."), *SelectedSessionItem->SessionName)));
}

void URoundMainMenuWidget::JoinSessionByIndex(const int32 SessionIndex)
{
    if (!SessionItems.IsValidIndex(SessionIndex))
    {
        SetMenuStatus(FText::FromString(TEXT("Dieser Raum ist nicht mehr verfügbar.")));
        return;
    }

    SelectedSessionItem = SessionItems[SessionIndex];
    HandleJoinSessionClicked();
}

void URoundMainMenuWidget::HandleStartGameClicked()
{
    StartHostedGame();
}

void URoundMainMenuWidget::HandleJoinDirectClicked()
{
    JoinDirectAddress();
}

void URoundMainMenuWidget::HandleShowCreateRoomClicked()
{
    ShowRoomMenuView(ERoundRoomMenuView::CreateRoom);
}

void URoundMainMenuWidget::HandleShowBrowseRoomsClicked()
{
    ShowRoomMenuView(ERoundRoomMenuView::BrowseRooms);
    FindSessions();
}

void URoundMainMenuWidget::HandleBackToRoomHomeClicked()
{
    ShowRoomMenuView(ERoundRoomMenuView::Home);
}

void URoundMainMenuWidget::HandleQuitClicked()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void URoundMainMenuWidget::HandleRoomNameChanged(const FText& InText)
{
    if (URoundGameInstance* GameInstance = GetRoundGameInstance())
    {
        GameInstance->RoomName = InText.ToString().TrimStartAndEnd();
    }
}

void URoundMainMenuWidget::HandleDirectAddressChanged(const FText& InText)
{
    DirectConnectAddress = InText.ToString().TrimStartAndEnd();
}

void URoundMainMenuWidget::HandleOpenTeamsClicked()
{
    ShowTeamsPanel();
}

void URoundMainMenuWidget::HandleOpenEditTeamsClicked()
{
    ShowTeamEditorPanel();
}

void URoundMainMenuWidget::HandleBackClicked()
{
    LoadOptions();
    ShowRoomMenuView(ERoundRoomMenuView::Home);
}

void URoundMainMenuWidget::HandleSaveTeamClicked()
{
    FText Error;
    if (!ValidateTeam(Error))
    {
        if (TextBlock_4)
        {
            TextBlock_4->SetText(Error);
        }
        return;
    }

    if (SelectedTeamIndex != INDEX_NONE && Teams.IsValidIndex(SelectedTeamIndex))
    {
        Teams[SelectedTeamIndex] = EditTeam;
    }
    else
    {
        SelectedTeamIndex = Teams.Add(EditTeam);
    }

    const FRoundTeamDefinition& SelectedTeam = Teams[SelectedTeamIndex];
    const URoundGameInstance* GameInstance = GetRoundGameInstance();
    FRoundFormatDefinition FormatDefinition;
    if (!GameInstance || !GameInstance->TryGetFormatDefinition(SelectedTeam.Format, FormatDefinition)
        || SelectedTeam.Characters.Num() != FormatDefinition.Characters)
    {
        SetMenuStatus(FText::FromString(TEXT("Das ausgewählte Team ist für dieses Format nicht vollständig.")));
        return;
    }
    for (const FRoundCharacterEntry& Character : SelectedTeam.Characters)
    {
        if ((Character.Skillpoints + Character.CharacterSkillpoints) != FormatDefinition.Skillpoints)
        {
            SetMenuStatus(FText::FromString(TEXT("Jeder Charakter muss alle Skillpunkte des Formats verteilt haben.")));
            return;
        }
    }

    PushTeamsToGameInstance();
    SaveTeamsToSlot();
    LoadTeams();
    ShowTeamsPanel();
}

void URoundMainMenuWidget::HandleDeleteSelectedTeamClicked()
{
    if (Teams.IsValidIndex(SelectedTeamIndex))
    {
        Teams.RemoveAt(SelectedTeamIndex);
        if (Teams.IsEmpty())
        {
            SelectedTeamIndex = INDEX_NONE;
        }
        else
        {
            SelectedTeamIndex = FMath::Clamp(SelectedTeamIndex, 0, Teams.Num() - 1);
        }
        PushTeamsToGameInstance();
        SaveTeamsToSlot();
        LoadTeams();
    }
}

void URoundMainMenuWidget::HandleDeleteEditedTeamClicked()
{
    HandleDeleteSelectedTeamClicked();
    ShowTeamsPanel();
}

void URoundMainMenuWidget::HandleDeleteCharacterClicked()
{
    if (!EditTeam.Characters.IsValidIndex(SelectedCharacterIndex))
    {
        return;
    }

    EditTeam.Characters.RemoveAt(SelectedCharacterIndex);
    SelectedCharacterIndex = EditTeam.Characters.IsEmpty()
        ? INDEX_NONE
        : FMath::Clamp(SelectedCharacterIndex, 0, EditTeam.Characters.Num() - 1);
    RefreshEditedTeamViews();
    LoadSelectedCharacter();
    EnsureSkillMenu();
}

void URoundMainMenuWidget::HandleAddCharacterClicked()
{
    const FString SelectedChampion = AvailableChampionIds.IsValidIndex(0) ? AvailableChampionIds[0].ToString() : FString();
    if (SelectedChampion.IsEmpty())
    {
        return;
    }

    FRoundCharacterEntry Entry;
    Entry.Character = SelectedChampion;
    EditTeam.Characters.Add(Entry);
    SelectedCharacterIndex = EditTeam.Characters.Num() - 1;

    LoadEditTeamCharacters();
    LoadSelectedCharacter();
}

void URoundMainMenuWidget::HandleAddTeamClicked()
{
    FRoundTeamDefinition NewTeam;
    NewTeam.Name = FString::Printf(TEXT("Team %d"), Teams.Num() + 1);
    FRoundFormatDefinition PrimaryFormat;
    if (const URoundGameInstance* GameInstance = GetRoundGameInstance(); GameInstance && GameInstance->TryGetPrimaryFormatDefinition(PrimaryFormat))
    {
        NewTeam.Format = PrimaryFormat.Format;
    }

    SelectedTeamIndex = Teams.Add(NewTeam);
    PushTeamsToGameInstance();
    SaveTeamsToSlot();
    LoadTeams();
}

void URoundMainMenuWidget::HandleAddTeamAndEditClicked()
{
    EditTeam = FRoundTeamDefinition();
    EditTeam.Name = FString::Printf(TEXT("Team %d"), Teams.Num() + 1);
    FRoundFormatDefinition PrimaryFormat;
    if (const URoundGameInstance* GameInstance = GetRoundGameInstance(); GameInstance && GameInstance->TryGetPrimaryFormatDefinition(PrimaryFormat))
    {
        EditTeam.Format = PrimaryFormat.Format;
    }
    SelectedTeamIndex = INDEX_NONE;
    SelectedCharacterIndex = INDEX_NONE;

    if (EditableText_TeamName)
    {
        EditableText_TeamName->SetText(FText::FromString(EditTeam.Name));
    }
    if (ComboBoxString_1)
    {
        ComboBoxString_1->SetSelectedOption(FormatToString(EditTeam.Format));
    }

    LoadEditTeamCharacters();
    LoadSelectedCharacter();
    EnsureSkillMenu();
    ShowTeamEditorPanel();
}

void URoundMainMenuWidget::HandleFormatSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (SelectionType == ESelectInfo::Direct)
    {
        return;
    }

    EditTeam.Format = StringToFormat(SelectedItem);

    if (SelectedTeamIndex != INDEX_NONE && Teams.IsValidIndex(SelectedTeamIndex))
    {
        Teams[SelectedTeamIndex] = EditTeam;
        PushTeamsToGameInstance();
        SaveTeamsToSlot();
    }

    RefreshTeamCounter();
    LoadEditTeamCharacters();
    LoadSelectedCharacter();
    EnsureSkillMenu();
}

void URoundMainMenuWidget::HandleSelectedTeamChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (SelectionType == ESelectInfo::Direct || SelectedItem.IsEmpty())
    {
        return;
    }

    for (int32 Index = 0; Index < Teams.Num(); ++Index)
    {
        if (Teams[Index].Name == SelectedItem)
        {
            SelectedTeamIndex = Index;
            PushTeamsToGameInstance();
            SetMenuStatus(FText::FromString(FString::Printf(TEXT("Startteam: %s"), *SelectedItem)));
            break;
        }
    }
}

void URoundMainMenuWidget::HandleTeamNameChanged(const FText& InText)
{
    EditTeam.Name = InText.ToString();
}

void URoundMainMenuWidget::HandleSessionItemClicked(UObject* Item)
{
    SelectedSessionItem = Cast<URoundSessionListItem>(Item);
    if (SelectedSessionItem)
    {
        SetMenuStatus(FText::FromString(FString::Printf(TEXT("Ausgewaehlt: %s"), *SelectedSessionItem->SessionName)));
    }
}

void URoundMainMenuWidget::HandleFindSessionsComplete(const bool bWasSuccessful)
{
    const IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
    if (SessionInterface.IsValid() && FindSessionsCompleteHandle.IsValid())
    {
        SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteHandle);
    }
    FindSessionsCompleteHandle.Reset();

    SessionItems.Reset();
    FoundSessions.Reset();

    if (bWasSuccessful && SessionSearch.IsValid())
    {
        FoundSessions = SessionSearch->SearchResults;

        for (int32 Index = 0; Index < FoundSessions.Num(); ++Index)
        {
            URoundSessionListItem* Item = NewObject<URoundSessionListItem>(this);
            if (!Item)
            {
                continue;
            }

            Item->SessionIndex = Index;
            FoundSessions[Index].Session.SessionSettings.Get(RoomNameSetting, Item->SessionName);
            if (Item->SessionName.IsEmpty())
            {
                Item->SessionName = FoundSessions[Index].Session.OwningUserName;
            }
            if (Item->SessionName.IsEmpty())
            {
                Item->SessionName = FString::Printf(TEXT("Session %d"), Index + 1);
            }

            SessionItems.Add(Item);
        }
    }

    RefreshSessionList();
    SetMenuStatus(bWasSuccessful
        ? FText::FromString(FString::Printf(TEXT("%d Session(s) gefunden."), SessionItems.Num()))
        : FText::FromString(TEXT("Keine Sessions gefunden.")));
}

void URoundMainMenuWidget::HandleCreateSessionComplete(FName SessionName, const bool bWasSuccessful)
{
    const IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
    if (SessionInterface.IsValid() && CreateSessionCompleteHandle.IsValid())
    {
        SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteHandle);
    }
    CreateSessionCompleteHandle.Reset();

    if (!bWasSuccessful)
    {
        SetMenuStatus(FText::FromString(TEXT("Session-Erstellung fehlgeschlagen.")));
        return;
    }

    URoundGameInstance* GameInstance = GetRoundGameInstance();
    if (GameInstance)
    {
        GameInstance->bHostingRoom = true;
        GameInstance->bJoinedRoom = true;
    }

    SetMenuStatus(FText::FromString(TEXT("Raum erstellt. Team wählen und anschließend Spiel starten.")));
    if (!StartListeningOnCurrentMap())
    {
        SetMenuStatus(FText::FromString(TEXT("Raum erstellt, aber der Listen-Server konnte nicht gestartet werden.")));
    }
    RefreshRoomControls();
}

void URoundMainMenuWidget::HandleJoinSessionComplete(FName SessionName, const EOnJoinSessionCompleteResult::Type Result)
{
    const IOnlineSessionPtr SessionInterface = Online::GetSessionInterface(GetWorld());
    if (SessionInterface.IsValid() && JoinSessionCompleteHandle.IsValid())
    {
        SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteHandle);
    }
    JoinSessionCompleteHandle.Reset();

    if (Result != EOnJoinSessionCompleteResult::Success || !SessionInterface.IsValid())
    {
        SetMenuStatus(FText::FromString(TEXT("Join fehlgeschlagen.")));
        return;
    }

    FString ConnectString;
    if (!SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString))
    {
        SetMenuStatus(FText::FromString(TEXT("Konnte Zieladresse der Session nicht aufloesen.")));
        return;
    }

    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        if (URoundGameInstance* GameInstance = GetRoundGameInstance())
        {
            GameInstance->bJoinedRoom = true;
            GameInstance->bHostingRoom = false;
            if (SelectedSessionItem)
            {
                GameInstance->RoomName = SelectedSessionItem->SessionName;
            }
        }
        PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
        SetMenuStatus(FText::FromString(TEXT("Session beigetreten.")));
    }
}

void URoundMainMenuWidget::StartHostedGame()
{
    URoundGameInstance* GameInstance = GetRoundGameInstance();
    if (!GameInstance || !GameInstance->bHostingRoom || !GetWorld() || GetWorld()->GetNetMode() == NM_Client)
    {
        SetMenuStatus(FText::FromString(TEXT("Nur der Host kann das Spiel starten.")));
        return;
    }

    const AGameStateBase* GameState = GetWorld()->GetGameState();
    if (!GameState || GameState->PlayerArray.Num() < 2)
    {
        SetMenuStatus(FText::FromString(TEXT("Zum Starten werden mindestens zwei Spieler benötigt.")));
        RefreshLobbyPlayers();
        return;
    }

    if (!Teams.IsValidIndex(SelectedTeamIndex))
    {
        SetMenuStatus(FText::FromString(TEXT("Bitte zuerst ein Team auswählen.")));
        return;
    }

    const FRoundTeamDefinition& SelectedTeam = Teams[SelectedTeamIndex];
    FRoundFormatDefinition FormatDefinition;
    if (!GameInstance->TryGetFormatDefinition(SelectedTeam.Format, FormatDefinition)
        || SelectedTeam.Characters.Num() != FormatDefinition.Characters)
    {
        SetMenuStatus(FText::FromString(TEXT("Das ausgewählte Team ist für dieses Format nicht vollständig.")));
        return;
    }
    for (const FRoundCharacterEntry& Character : SelectedTeam.Characters)
    {
        if ((Character.Skillpoints + Character.CharacterSkillpoints) != FormatDefinition.Skillpoints)
        {
            SetMenuStatus(FText::FromString(TEXT("Jeder Charakter muss alle Skillpunkte des Formats verteilt haben.")));
            return;
        }
    }

    PushTeamsToGameInstance();
    TSoftObjectPtr<UWorld> MatchMap = RoundGameDataRegistryLookup::ResolveMatchMap();
    const FSoftObjectPath MatchPath = MatchMap.ToSoftObjectPath();
    if (!MatchPath.IsValid())
    {
        SetMenuStatus(FText::FromString(TEXT("Kein Match-Map-Asset in DA_RoundGameDataRegistry gesetzt.")));
        return;
    }

    SetMenuStatus(FText::FromString(TEXT("Starte Spiel...")));
    GetWorld()->ServerTravel(MatchPath.GetLongPackageName() + TEXT("?listen"), true);
}

void URoundMainMenuWidget::JoinDirectAddress()
{
    const FString Address = DirectConnectAddress.TrimStartAndEnd();
    if (Address.IsEmpty())
    {
        SetMenuStatus(FText::FromString(TEXT("Bitte IP-Adresse oder Hostnamen eingeben.")));
        return;
    }

    PushTeamsToGameInstance();
    if (URoundGameInstance* GameInstance = GetRoundGameInstance())
    {
        GameInstance->bJoinedRoom = true;
        GameInstance->bHostingRoom = false;
    }

    if (APlayerController* PlayerController = GetOwningPlayer())
    {
        SetMenuStatus(FText::FromString(FString::Printf(TEXT("Verbinde mit %s..."), *Address)));
        PlayerController->ClientTravel(Address, TRAVEL_Absolute);
    }
}

bool URoundMainMenuWidget::StartListeningOnCurrentMap()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() != NM_Standalone)
    {
        return World && World->GetNetMode() == NM_ListenServer;
    }

    const FString CurrentMapPackage = World->GetOutermost()->GetName();
    if (CurrentMapPackage.IsEmpty())
    {
        return false;
    }

    UGameplayStatics::OpenLevel(this, FName(*CurrentMapPackage), true, TEXT("listen"));
    return true;
}

URoundGameInstance* URoundMainMenuWidget::GetRoundGameInstance() const
{
    return GetWorld() ? GetWorld()->GetGameInstance<URoundGameInstance>() : nullptr;
}

void URoundMainMenuWidget::LoadOptions()
{
    if (URoundGameInstance* GameInstance = GetRoundGameInstance())
    {
        Teams = GameInstance->LoadedTeams;
    }

    if (SelectedTeamIndex == INDEX_NONE && Teams.Num() > 0)
    {
        SelectedTeamIndex = 0;
    }

    LoadFormats();
    LoadTeams();
    RefreshSessionList();
    ShowMainPanel();
}

void URoundMainMenuWidget::RefreshSessionList()
{
    SelectedSessionItem = nullptr;
    if (ListView_0)
    {
        ListView_0->ClearListItems();
        for (URoundSessionListItem* Item : SessionItems)
        {
            if (Item)
            {
                ListView_0->AddItem(Item);
            }
        }
    }

}

void URoundMainMenuWidget::RefreshRoomControls()
{
    if (ComboBox_RoomTeam)
    {
        ComboBox_RoomTeam->ClearOptions();
        for (const FRoundTeamDefinition& TeamDefinition : Teams)
        {
            ComboBox_RoomTeam->AddOption(TeamDefinition.Name);
        }
        if (Teams.IsValidIndex(SelectedTeamIndex))
        {
            ComboBox_RoomTeam->SetSelectedOption(Teams[SelectedTeamIndex].Name);
        }
    }
    if (EditableText_RoomName)
    {
        if (const URoundGameInstance* GameInstance = GetRoundGameInstance())
        {
            EditableText_RoomName->SetText(FText::FromString(GameInstance->RoomName));
        }
    }
    if (EditableText_DirectAddress)
    {
        EditableText_DirectAddress->SetText(FText::FromString(DirectConnectAddress));
    }
    RefreshLobbyPlayers();
}

void URoundMainMenuWidget::ShowRoomMenuView(const ERoundRoomMenuView View)
{
    CurrentRoomView = View;
    SetRoomPanelVisible(RoomHomePanel, View == ERoundRoomMenuView::Home);
    SetRoomPanelVisible(RoomCreatePanel, View == ERoundRoomMenuView::CreateRoom);
    SetRoomPanelVisible(RoomBrowsePanel, View == ERoundRoomMenuView::BrowseRooms);
    SetRoomPanelVisible(RoomLobbyPanel, View == ERoundRoomMenuView::Lobby);
    if (View == ERoundRoomMenuView::Lobby && Text_LobbyTitle)
    {
        const URoundGameInstance* GameInstance = GetRoundGameInstance();
        Text_LobbyTitle->SetText(FText::FromString(GameInstance ? GameInstance->RoomName.ToUpper() : TEXT("RAUM")));
    }
    RefreshRoomControls();
}

void URoundMainMenuWidget::RefreshLobbyPlayers()
{
    if (CurrentRoomView != ERoundRoomMenuView::Lobby)
    {
        return;
    }

    TArray<APlayerState*> Players;
    if (const UWorld* World = GetWorld())
    {
        if (const AGameStateBase* GameState = World->GetGameState())
        {
            Players = GameState->PlayerArray;
        }
    }

    if (Text_LobbyPlayerCount)
    {
        Text_LobbyPlayerCount->SetText(FText::FromString(FString::Printf(TEXT("SPIELER  %d / 2"), Players.Num())));
    }
    if (Text_LobbyPlayers)
    {
        FString PlayerLines;
        for (int32 Index = 0; Index < Players.Num(); ++Index)
        {
            FString PlayerName = Players[Index] ? Players[Index]->GetPlayerName() : FString();
            if (PlayerName.IsEmpty())
            {
                PlayerName = FString::Printf(TEXT("Spieler %d"), Index + 1);
            }
            PlayerLines += FString::Printf(TEXT("●  %s\n"), *PlayerName);
        }
        for (int32 Index = Players.Num(); Index < 2; ++Index)
        {
            PlayerLines += TEXT("○  Warte auf Spieler ...\n");
        }
        Text_LobbyPlayers->SetText(FText::FromString(PlayerLines.TrimEnd()));
    }

    const URoundGameInstance* GameInstance = GetRoundGameInstance();
    const bool bIsHost = GameInstance && GameInstance->bHostingRoom;
    const bool bEnoughPlayers = Players.Num() >= 2;
    if (Button_StartGame)
    {
        Button_StartGame->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
        Button_StartGame->SetIsEnabled(bIsHost && bEnoughPlayers);
    }
    if (Text_RoomStatus)
    {
        Text_RoomStatus->SetText(FText::FromString(bIsHost
            ? (bEnoughPlayers ? TEXT("Alle Spieler sind bereit.") : TEXT("Das Spiel kann ab zwei Spielern gestartet werden."))
            : TEXT("Warte darauf, dass der Host das Spiel startet.")));
    }
}

bool URoundMainMenuWidget::UsesLanSessions() const
{
    const IOnlineSubsystem* Subsystem = Online::GetSubsystem(GetWorld());
    return !Subsystem || Subsystem->GetSubsystemName() == FName(TEXT("NULL"));
}

void URoundMainMenuWidget::LoadFormats()
{
    if (ComboBoxString_1)
    {
        ComboBoxString_1->ClearOptions();
        ComboBoxString_1->SetVisibility(ESlateVisibility::Collapsed);
        if (const URoundGameInstance* GameInstance = GetRoundGameInstance())
        {
            FRoundFormatDefinition PrimaryFormat;
            if (GameInstance->TryGetPrimaryFormatDefinition(PrimaryFormat))
            {
                ComboBoxString_1->AddOption(FormatToString(PrimaryFormat.Format));
                ComboBoxString_1->SetSelectedOption(FormatToString(PrimaryFormat.Format));
                EditTeam.Format = PrimaryFormat.Format;
            }
        }
    }
}

void URoundMainMenuWidget::LoadEditTeamCharacters()
{
    if (!VerticalBox_Characters)
    {
        return;
    }

    VerticalBox_Characters->ClearChildren();

    for (int32 Index = 0; Index < EditTeam.Characters.Num(); ++Index)
    {
        TSubclassOf<URoundCharacterSelectionOptionWidget> EntryClass;
        if (CharacterSelectionOptionClass)
        {
            EntryClass = CharacterSelectionOptionClass;
        }
        else
        {
            EntryClass = URoundCharacterSelectionOptionWidget::StaticClass();
        }

        URoundCharacterSelectionOptionWidget* Entry = CreateWidget<URoundCharacterSelectionOptionWidget>(
            this,
            EntryClass);

        if (!Entry)
        {
            continue;
        }

        Entry->MainMenu = this;
        Entry->SlotIndex = Index;
        Entry->SetChampion(FName(*EditTeam.Characters[Index].Character));
        VerticalBox_Characters->AddChild(Entry);
    }

    RefreshTeamCounter();
}

void URoundMainMenuWidget::LoadSelectedCharacter()
{
}

void URoundMainMenuWidget::RefreshTeamCounter()
{
    if (!TeamCharacterCounter)
    {
        return;
    }

    int32 MaxCharacters = 0;
    if (const URoundGameInstance* GameInstance = GetRoundGameInstance())
    {
        FRoundFormatDefinition FormatDefinition;
        if (GameInstance->TryGetFormatDefinition(EditTeam.Format, FormatDefinition))
        {
            MaxCharacters = FormatDefinition.Characters;
        }
    }

    TeamCharacterCounter->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), EditTeam.Characters.Num(), MaxCharacters)));
}

void URoundMainMenuWidget::EnsureSkillMenu()
{
    if (!Border_0)
    {
        return;
    }

    if (!CurrentSkillMenu)
    {
        TSubclassOf<URoundSkillMenuWidget> WidgetClass = SkillMenuClass;
        if (!WidgetClass)
        {
            WidgetClass = URoundSkillMenuWidget::StaticClass();
        }
        CurrentSkillMenu = CreateWidget<URoundSkillMenuWidget>(this, WidgetClass);
        if (CurrentSkillMenu)
        {
            Border_0->SetContent(CurrentSkillMenu);
        }
    }

    if (CurrentSkillMenu)
    {
        CurrentSkillMenu->SetMainMenuWidget(this);
        CurrentSkillMenu->LoadSkillpoints();
    }
}

void URoundMainMenuWidget::ShowMainPanel()
{
    SetPanelVisible(CanvasPanel_Main, true);
    SetPanelVisible(CanvasPanel_Teams, false);
    SetPanelVisible(CanvasPanel_Team, false);
}

void URoundMainMenuWidget::ShowTeamsPanel()
{
    SetPanelVisible(CanvasPanel_Main, false);
    SetPanelVisible(CanvasPanel_Teams, true);
    SetPanelVisible(CanvasPanel_Team, false);
}

void URoundMainMenuWidget::ShowTeamEditorPanel()
{
    SetPanelVisible(CanvasPanel_Main, false);
    SetPanelVisible(CanvasPanel_Teams, true);
    SetPanelVisible(CanvasPanel_Team, true);
}

void URoundMainMenuWidget::SetMenuStatus(const FText& InText)
{
    if (TextBlock_4)
    {
        TextBlock_4->SetText(InText);
    }
    if (Text_RoomStatus)
    {
        Text_RoomStatus->SetText(InText);
    }
}

void URoundMainMenuWidget::SaveTeamsToSlot()
{
    URoundSaveGame* SaveGame = Cast<URoundSaveGame>(UGameplayStatics::CreateSaveGameObject(URoundSaveGame::StaticClass()));
    if (!SaveGame)
    {
        return;
    }

    SaveGame->Teams = Teams;
    UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0);
}

void URoundMainMenuWidget::PushTeamsToGameInstance()
{
    if (URoundGameInstance* GameInstance = GetRoundGameInstance())
    {
        GameInstance->LoadedTeams = Teams;
        if (Teams.IsValidIndex(SelectedTeamIndex))
        {
            GameInstance->StrTeam = Teams[SelectedTeamIndex];
        }
        else if (Teams.Num() > 0)
        {
            GameInstance->StrTeam = Teams[0];
        }
    }
}

void URoundMainMenuWidget::RefreshTeamSelectionOptions()
{
    if (!ComboBoxString_0)
    {
        PushTeamsToGameInstance();
        return;
    }

    ComboBoxString_0->ClearOptions();

    for (const FRoundTeamDefinition& Team : Teams)
    {
        ComboBoxString_0->AddOption(Team.Name);
    }

    if (Teams.IsValidIndex(SelectedTeamIndex))
    {
        ComboBoxString_0->SetSelectedOption(Teams[SelectedTeamIndex].Name);
    }
    else if (Teams.Num() > 0)
    {
        SelectedTeamIndex = 0;
        ComboBoxString_0->SetSelectedOption(Teams[0].Name);
    }

    PushTeamsToGameInstance();
    RefreshRoomControls();
}

FString URoundMainMenuWidget::FormatToString(const ERoundFormat Format)
{
    switch (Format)
    {
    case ERoundFormat::Format5x16:
        return TEXT("5-16");
    case ERoundFormat::Format5x25:
        return TEXT("5-25");
    case ERoundFormat::Formattest:
        return TEXT("test");
    case ERoundFormat::Format3x16:
    default:
        return TEXT("3-16");
    }
}

ERoundFormat URoundMainMenuWidget::StringToFormat(const FString& InString)
{
    if (InString.Equals(TEXT("5-16"), ESearchCase::IgnoreCase))
    {
        return ERoundFormat::Format5x16;
    }

    if (InString.Equals(TEXT("5-25"), ESearchCase::IgnoreCase))
    {
        return ERoundFormat::Format5x25;
    }

    if (InString.Equals(TEXT("test"), ESearchCase::IgnoreCase))
    {
        return ERoundFormat::Formattest;
    }

    return ERoundFormat::Format3x16;
}
