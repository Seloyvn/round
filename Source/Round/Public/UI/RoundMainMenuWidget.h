#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "RoundTypes.h"
#include "RoundMainMenuWidget.generated.h"

class UButton;
class UBorder;
class UCanvasPanel;
class UComboBoxString;
class UEditableText;
class UHorizontalBox;
class USizeBox;
class UVerticalBox;
class UListView;
class UTextBlock;
class UVerticalBox;
class URoundCharacterSelectionOptionWidget;
class URoundGameInstance;
class URoundSessionListItem;
class URoundSkillMenuWidget;
class URoundTeamsListEntryWidget;
class ARoundMenuController;

enum class ERoundRoomMenuView : uint8
{
    Home,
    CreateRoom,
    BrowseRooms,
    Lobby
};

UCLASS(Blueprintable)
class ROUND_API URoundMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    URoundMainMenuWidget(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|Menu", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<ARoundMenuController> Controller = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Menu")
    void TeamIndexSelected(int32 InSelectedTeamIndex);

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Menu")
    void SetSelectedCharacterIndex(int32 InSelectedCharacterIndex);

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Menu")
    void LoadTeams();

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Menu")
    void SelectTeamToEdit();

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Menu")
    bool ValidateTeam(FText& OutError) const;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Menu")
    void FindSessions();

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Room")
    void StartHostedGame();

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Room")
    void JoinDirectAddress();

    void JoinSessionByIndex(int32 SessionIndex);

    const FRoundTeamDefinition& GetEditTeamDefinition() const { return EditTeam; }
    int32 GetCurrentSelectedCharacterIndex() const { return SelectedCharacterIndex; }
    bool TryGetCurrentFormatDefinition(FRoundFormatDefinition& OutFormat) const;
    int32 GetRemainingTeamSkillPoints() const;
    bool TryGetEditedCharacter(int32 CharacterIndex, FRoundCharacterEntry& OutCharacter) const;
    bool UpdateEditedCharacter(int32 CharacterIndex, const FRoundCharacterEntry& InCharacter, bool bRefreshSelection = true);
    void RefreshEditedTeamViews();
    const TArray<FName>& GetAvailableChampionIds() const { return AvailableChampionIds; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> CanvasPanel_Main = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> CanvasPanel_Team = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> CanvasPanel_Teams = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UBorder> Border_0 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_0 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_1 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Add_Character = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Add_Team = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Add_Team_1 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Back = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_EditorBack = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Delete_Team = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Delete_Team_1 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_Delete_Character = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_EditTeams = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_EditTeams_1 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_SaveTeam = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UComboBoxString> ComboBoxString_0 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UComboBoxString> ComboBoxString_1 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UEditableText> EditableText_TeamName = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> VerticalBox_Characters = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> VerticalBox_Teams = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TeamCharacterCounter = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_4 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UListView> ListView_0 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> RoomHomePanel = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> RoomCreatePanel = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> RoomBrowsePanel = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UVerticalBox> RoomLobbyPanel = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_ShowCreateRoom = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_ShowBrowseRooms = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_RoomBack = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_BrowseBack = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_Quit = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_StartGame = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> Button_JoinDirect = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UEditableText> EditableText_RoomName = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UEditableText> EditableText_DirectAddress = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> ComboBox_RoomTeam = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_RoomStatus = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_LobbyTitle = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_LobbyPlayerCount = nullptr;
    UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> Text_LobbyPlayers = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|Menu")
    TSubclassOf<URoundTeamsListEntryWidget> TeamEntryClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|Menu")
    TSubclassOf<URoundCharacterSelectionOptionWidget> CharacterSelectionOptionClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|Menu")
    TSubclassOf<URoundSkillMenuWidget> SkillMenuClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|Menu")
    TArray<FName> AvailableChampionIds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|UI|Menu")
    TArray<FRoundTeamDefinition> Teams;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|UI|Menu")
    FRoundTeamDefinition EditTeam;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|UI|Menu")
    int32 SelectedTeamIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|UI|Menu")
    int32 SelectedCharacterIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|UI|Menu")
    TObjectPtr<URoundSessionListItem> SelectedSessionItem = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|UI|Menu")
    TArray<TObjectPtr<URoundSessionListItem>> SessionItems;

private:
    UFUNCTION()
    void HandleRefreshSessionsClicked();

    UFUNCTION()
    void HandleCreateSessionClicked();

    UFUNCTION()
    void HandleJoinSessionClicked();

    UFUNCTION()
    void HandleStartGameClicked();

    UFUNCTION()
    void HandleJoinDirectClicked();

    UFUNCTION()
    void HandleShowCreateRoomClicked();

    UFUNCTION()
    void HandleShowBrowseRoomsClicked();

    UFUNCTION()
    void HandleBackToRoomHomeClicked();

    UFUNCTION()
    void HandleQuitClicked();

    UFUNCTION()
    void HandleRoomNameChanged(const FText& InText);

    UFUNCTION()
    void HandleDirectAddressChanged(const FText& InText);

    UFUNCTION()
    void HandleOpenTeamsClicked();

    UFUNCTION()
    void HandleOpenEditTeamsClicked();

    UFUNCTION()
    void HandleBackClicked();

    UFUNCTION()
    void HandleSaveTeamClicked();

    UFUNCTION()
    void HandleDeleteSelectedTeamClicked();

    UFUNCTION()
    void HandleDeleteEditedTeamClicked();

    UFUNCTION()
    void HandleDeleteCharacterClicked();

    UFUNCTION()
    void HandleAddCharacterClicked();

    UFUNCTION()
    void HandleAddTeamClicked();

    UFUNCTION()
    void HandleAddTeamAndEditClicked();

    UFUNCTION()
    void HandleFormatSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void HandleSelectedTeamChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void HandleTeamNameChanged(const FText& InText);

    UFUNCTION()
    void HandleSessionItemClicked(UObject* Item);

    URoundGameInstance* GetRoundGameInstance() const;
    void RefreshSessionList();
    void RefreshRoomControls();
    void ShowRoomMenuView(ERoundRoomMenuView View);
    void RefreshLobbyPlayers();
    bool UsesLanSessions() const;
    bool StartListeningOnCurrentMap();
    void RefreshTeamSelectionOptions();
    void SetMenuStatus(const FText& InText);
    void LoadOptions();
    void LoadFormats();
    void LoadEditTeamCharacters();
    void LoadSelectedCharacter();
    void RefreshTeamCounter();
    void EnsureSkillMenu();
    void ShowMainPanel();
    void ShowTeamsPanel();
    void ShowTeamEditorPanel();
    void SaveTeamsToSlot();
    void PushTeamsToGameInstance();
    static FString FormatToString(ERoundFormat Format);
    static ERoundFormat StringToFormat(const FString& InString);
    void HandleFindSessionsComplete(bool bWasSuccessful);
    void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

    TSharedPtr<class FOnlineSessionSearch> SessionSearch;
    TArray<class FOnlineSessionSearchResult> FoundSessions;
    FDelegateHandle FindSessionsCompleteHandle;
    FDelegateHandle CreateSessionCompleteHandle;
    FDelegateHandle JoinSessionCompleteHandle;
    TObjectPtr<URoundSkillMenuWidget> CurrentSkillMenu = nullptr;
    ERoundRoomMenuView CurrentRoomView = ERoundRoomMenuView::Home;
    float LobbyRefreshAccumulator = 0.f;
    FString DirectConnectAddress = TEXT("127.0.0.1:7777");
};
