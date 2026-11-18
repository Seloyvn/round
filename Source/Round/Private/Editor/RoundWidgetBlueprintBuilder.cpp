#include "Editor/RoundWidgetBlueprintBuilder.h"

#if WITH_EDITOR
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/ComboBoxKey.h"
#include "Components/CheckBox.h"
#include "Components/EditableText.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ListView.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/RoundStartPositionWidget.h"
#include "UI/RoundDebugWidget.h"
#include "WidgetBlueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/UnrealType.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

namespace
{
const FLinearColor Background(0.025f, 0.035f, 0.06f, 1.f);
const FLinearColor Card(0.055f, 0.075f, 0.12f, 1.f);
const FLinearColor Primary(0.12f, 0.25f, 0.48f, 1.f);
const FLinearColor Secondary(0.12f, 0.14f, 0.20f, 1.f);
const FLinearColor Danger(0.24f, 0.10f, 0.13f, 1.f);
const FLinearColor MainText(0.88f, 0.92f, 1.f, 1.f);
const FLinearColor MutedText(0.62f, 0.68f, 0.80f, 1.f);

template <typename T>
T* Make(UWidgetTree* Tree, const TCHAR* Name)
{
    T* Widget = Tree->ConstructWidget<T>(T::StaticClass(), FName(Name));
    if (FBoolProperty* VariableProperty = FindFProperty<FBoolProperty>(Widget->GetClass(), TEXT("bIsVariable")))
    {
        VariableProperty->SetPropertyValue_InContainer(Widget, true);
    }
    return Widget;
}

void ClearWidgetTree(UWidgetTree* Tree)
{
    TArray<UWidget*> ExistingWidgets;
    Tree->GetAllWidgets(ExistingWidgets);
    Tree->RootWidget = nullptr;
    for (UWidget* Widget : ExistingWidgets)
    {
        if (!Widget) continue;
        Widget->RemoveFromParent();
        const FName ObsoleteName = MakeUniqueObjectName(Tree, Widget->GetClass(), TEXT("ObsoleteWidget"));
        Widget->Rename(*ObsoleteName.ToString(), Tree, REN_DontCreateRedirectors | REN_NonTransactional);
    }
}

void FillCanvas(UCanvasPanelSlot* Slot, const int32 ZOrder = 0)
{
    Slot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    Slot->SetOffsets(FMargin(0.f));
    Slot->SetZOrder(ZOrder);
}

UTextBlock* AddText(UWidgetTree* Tree, UVerticalBox* Box, const TCHAR* Name, const TCHAR* Value, int32 Size, FLinearColor Color = MainText)
{
    UTextBlock* Text = Make<UTextBlock>(Tree, Name);
    Text->SetText(FText::FromString(Value));
    Text->SetColorAndOpacity(FSlateColor(Color));
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = Size;
    Text->SetFont(Font);
    Box->AddChildToVerticalBox(Text)->SetPadding(FMargin(0.f, 5.f));
    return Text;
}

UButton* AddButton(UWidgetTree* Tree, UVerticalBox* Box, const TCHAR* Name, const TCHAR* Label, FLinearColor Color = Primary)
{
    UButton* Button = Make<UButton>(Tree, Name);
    Button->SetBackgroundColor(Color);
    UTextBlock* Text = Make<UTextBlock>(Tree, *FString::Printf(TEXT("%s_Label"), Name));
    Text->SetText(FText::FromString(Label));
    Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    FSlateFontInfo Font = Text->GetFont();
    Font.Size = 21;
    Text->SetFont(Font);
    Text->SetJustification(ETextJustify::Center);
    Button->AddChild(Text);
    Box->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.f, 6.f));
    return Button;
}

UVerticalBox* AddCenteredCard(UWidgetTree* Tree, UCanvasPanel* Canvas, const TCHAR* Name, float Width, float Height)
{
    USizeBox* Size = Make<USizeBox>(Tree, *FString::Printf(TEXT("%s_Size"), Name));
    Size->SetWidthOverride(Width);
    Size->SetHeightOverride(Height);
    UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Size);
    Slot->SetAnchors(FAnchors(0.5f));
    Slot->SetAlignment(FVector2D(0.5f));
    Slot->SetPosition(FVector2D::ZeroVector);
    Slot->SetSize(FVector2D(Width, Height));

    UBorder* Border = Make<UBorder>(Tree, *FString::Printf(TEXT("%s_Card"), Name));
    Border->SetBrushColor(Card);
    Border->SetPadding(FMargin(42.f, 32.f));
    Size->AddChild(Border);
    UVerticalBox* Box = Make<UVerticalBox>(Tree, Name);
    Border->SetContent(Box);
    return Box;
}

UCanvasPanel* AddScreen(UWidgetTree* Tree, UCanvasPanel* Root, const TCHAR* Name, bool Visible)
{
    UCanvasPanel* Canvas = Make<UCanvasPanel>(Tree, Name);
    Canvas->SetVisibility(Visible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    FillCanvas(Root->AddChildToCanvas(Canvas));
    UBorder* Backdrop = Make<UBorder>(Tree, *FString::Printf(TEXT("%s_Background"), Name));
    Backdrop->SetBrushColor(Background);
    FillCanvas(Canvas->AddChildToCanvas(Backdrop), -1);
    return Canvas;
}

bool SaveBlueprint(UWidgetBlueprint* Blueprint)
{
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    Blueprint->MarkPackageDirty();
    const FString Filename = FPackageName::LongPackageNameToFilename(
        Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename, SaveArgs);
}

bool RebuildMainMenu()
{
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/TopDown/UI/UI_MainMenu.UI_MainMenu"));
    if (!Blueprint || !Blueprint->WidgetTree) return false;
    UWidgetTree* Tree = Blueprint->WidgetTree;
    Tree->Modify();
    ClearWidgetTree(Tree);

    UCanvasPanel* Root = Make<UCanvasPanel>(Tree, TEXT("RootCanvas"));
    Tree->RootWidget = Root;

    UCanvasPanel* Main = AddScreen(Tree, Root, TEXT("CanvasPanel_Main"), true);
    UVerticalBox* Home = AddCenteredCard(Tree, Main, TEXT("RoomHomePanel"), 720.f, 600.f);
    AddText(Tree, Home, TEXT("Text_Title"), TEXT("ROUND"), 48);
    AddText(Tree, Home, TEXT("Text_Subtitle"), TEXT("ONLINE"), 18, FLinearColor(0.35f, 0.62f, 1.f));
    AddButton(Tree, Home, TEXT("Button_ShowCreateRoom"), TEXT("Raum erstellen"));
    AddButton(Tree, Home, TEXT("Button_ShowBrowseRooms"), TEXT("Raum beitreten"));
    AddButton(Tree, Home, TEXT("Button_EditTeams"), TEXT("Teams verwalten"), Secondary);
    AddButton(Tree, Home, TEXT("Button_Quit"), TEXT("Spiel beenden"), Danger);

    UVerticalBox* Create = AddCenteredCard(Tree, Main, TEXT("RoomCreatePanel"), 720.f, 580.f);
    AddText(Tree, Create, TEXT("Text_CreateTitle"), TEXT("RAUM ERSTELLEN"), 34);
    AddText(Tree, Create, TEXT("Text_RoomNameLabel"), TEXT("Raumname"), 17, MutedText);
    UEditableText* RoomName = Make<UEditableText>(Tree, TEXT("EditableText_RoomName"));
    RoomName->SetHintText(FText::FromString(TEXT("Mein Raum")));
    Create->AddChildToVerticalBox(RoomName)->SetPadding(FMargin(0.f, 4.f, 0.f, 16.f));
    AddButton(Tree, Create, TEXT("Button_0"), TEXT("Raum öffnen"));
    AddButton(Tree, Create, TEXT("Button_RoomBack"), TEXT("Zurück"), Secondary);

    UVerticalBox* Browse = AddCenteredCard(Tree, Main, TEXT("RoomBrowsePanel"), 820.f, 780.f);
    AddText(Tree, Browse, TEXT("Text_BrowseTitle"), TEXT("VERFÜGBARE RÄUME"), 34);
    UListView* Sessions = Make<UListView>(Tree, TEXT("ListView_0"));
    Sessions->SetScrollbarVisibility(ESlateVisibility::Visible);
    if (UWidgetBlueprint* SessionEntryBlueprint = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/TopDown/Blueprints/UI_SessionsUserWidget.UI_SessionsUserWidget")))
    {
        if (FClassProperty* EntryClassProperty = FindFProperty<FClassProperty>(Sessions->GetClass(), TEXT("EntryWidgetClass")))
        {
            EntryClassProperty->SetPropertyValue_InContainer(Sessions, SessionEntryBlueprint->GeneratedClass);
        }
    }
    UVerticalBoxSlot* SessionSlot = Browse->AddChildToVerticalBox(Sessions);
    SessionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    SessionSlot->SetPadding(FMargin(0.f, 8.f));
    AddButton(Tree, Browse, TEXT("Button_1"), TEXT("Ausgewähltem Raum beitreten"));
    AddButton(Tree, Browse, TEXT("Button"), TEXT("Liste aktualisieren"), Secondary);
    AddText(Tree, Browse, TEXT("Text_AddressLabel"), TEXT("Direkte Adresse (optional)"), 16, MutedText);
    UEditableText* Address = Make<UEditableText>(Tree, TEXT("EditableText_DirectAddress"));
    Address->SetHintText(FText::FromString(TEXT("IP:Port")));
    Browse->AddChildToVerticalBox(Address)->SetPadding(FMargin(0.f, 4.f));
    AddButton(Tree, Browse, TEXT("Button_JoinDirect"), TEXT("Adresse beitreten"), Secondary);
    AddButton(Tree, Browse, TEXT("Button_BrowseBack"), TEXT("Zurück"), Secondary);

    UVerticalBox* Lobby = AddCenteredCard(Tree, Main, TEXT("RoomLobbyPanel"), 760.f, 650.f);
    AddText(Tree, Lobby, TEXT("Text_LobbyTitle"), TEXT("RAUM"), 34);
    AddText(Tree, Lobby, TEXT("Text_LobbyPlayerCount"), TEXT("SPIELER  1 / 2"), 18, FLinearColor(0.35f, 0.62f, 1.f));
    AddText(Tree, Lobby, TEXT("Text_LobbyPlayers"), TEXT("●  Spieler 1\n○  Warte auf Spieler ..."), 21);
    AddText(Tree, Lobby, TEXT("Text_TeamLabel"), TEXT("Team"), 17, MutedText);
    UComboBoxString* TeamCombo = Make<UComboBoxString>(Tree, TEXT("ComboBox_RoomTeam"));
    Lobby->AddChildToVerticalBox(TeamCombo)->SetPadding(FMargin(0.f, 4.f, 0.f, 16.f));
    AddButton(Tree, Lobby, TEXT("Button_StartGame"), TEXT("Spiel starten"));
    AddText(Tree, Lobby, TEXT("Text_RoomStatus"), TEXT("Warte auf einen zweiten Spieler."), 16, MutedText);

    UCanvasPanel* Teams = AddScreen(Tree, Root, TEXT("CanvasPanel_Teams"), false);
    UVerticalBox* TeamsCard = AddCenteredCard(Tree, Teams, TEXT("TeamsCard"), 920.f, 760.f);
    AddText(Tree, TeamsCard, TEXT("Text_TeamsTitle"), TEXT("TEAMS"), 38);
    AddText(Tree, TeamsCard, TEXT("Text_TeamsHint"), TEXT("Wähle ein Team oder erstelle ein neues."), 17, MutedText);
    UComboBoxString* TeamSelection = Make<UComboBoxString>(Tree, TEXT("ComboBoxString_0"));
    TeamsCard->AddChildToVerticalBox(TeamSelection)->SetPadding(FMargin(0.f, 5.f, 0.f, 12.f));
    UVerticalBox* TeamRows = Make<UVerticalBox>(Tree, TEXT("VerticalBox_Teams"));
    UVerticalBoxSlot* TeamRowsSlot = TeamsCard->AddChildToVerticalBox(TeamRows);
    TeamRowsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    AddButton(Tree, TeamsCard, TEXT("Button_EditTeams_1"), TEXT("Team bearbeiten"));
    AddButton(Tree, TeamsCard, TEXT("Button_Add_Team"), TEXT("Neues Team"), Secondary);
    AddButton(Tree, TeamsCard, TEXT("Button_Delete_Team"), TEXT("Team löschen"), Danger);
    AddButton(Tree, TeamsCard, TEXT("Button_Back"), TEXT("Zurück"), Secondary);

    UCanvasPanel* Editor = AddScreen(Tree, Root, TEXT("CanvasPanel_Team"), false);
    UVerticalBox* EditorCard = AddCenteredCard(Tree, Editor, TEXT("TeamEditorCard"), 1500.f, 900.f);
    AddText(Tree, EditorCard, TEXT("Text_EditorTitle"), TEXT("TEAM BEARBEITEN"), 36);
    UEditableText* TeamName = Make<UEditableText>(Tree, TEXT("EditableText_TeamName"));
    TeamName->SetHintText(FText::FromString(TEXT("Teamname")));
    EditorCard->AddChildToVerticalBox(TeamName)->SetPadding(FMargin(0.f, 3.f, 0.f, 10.f));
    UComboBoxString* Format = Make<UComboBoxString>(Tree, TEXT("ComboBoxString_1"));
    Format->SetVisibility(ESlateVisibility::Collapsed);
    EditorCard->AddChildToVerticalBox(Format);
    UHorizontalBox* Columns = Make<UHorizontalBox>(Tree, TEXT("TeamEditorColumns"));
    UVerticalBoxSlot* ColumnsSlot = EditorCard->AddChildToVerticalBox(Columns);
    ColumnsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    ColumnsSlot->SetPadding(FMargin(0.f, 8.f));
    UVerticalBox* Roster = Make<UVerticalBox>(Tree, TEXT("RosterColumn"));
    UHorizontalBoxSlot* RosterSlot = Columns->AddChildToHorizontalBox(Roster);
    RosterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    RosterSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
    AddText(Tree, Roster, TEXT("Text_RosterTitle"), TEXT("CHARAKTERE"), 21, MutedText);
    UTextBlock* Counter = AddText(Tree, Roster, TEXT("TeamCharacterCounter"), TEXT("0 / 5"), 18);
    Counter->SetJustification(ETextJustify::Right);
    UVerticalBox* Characters = Make<UVerticalBox>(Tree, TEXT("VerticalBox_Characters"));
    UVerticalBoxSlot* CharacterSlot = Roster->AddChildToVerticalBox(Characters);
    CharacterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    AddButton(Tree, Roster, TEXT("Button_Add_Character"), TEXT("Charakter hinzufügen"), Secondary);
    AddButton(Tree, Roster, TEXT("Button_Delete_Character"), TEXT("Ausgewählten Charakter löschen"), Danger);
    UBorder* SkillCard = Make<UBorder>(Tree, TEXT("Border_0"));
    SkillCard->SetBrushColor(FLinearColor(0.035f, 0.05f, 0.085f, 1.f));
    SkillCard->SetPadding(FMargin(18.f));
    UHorizontalBoxSlot* SkillSlot = Columns->AddChildToHorizontalBox(SkillCard);
    SkillSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    SkillSlot->SetPadding(FMargin(12.f, 0.f, 0.f, 0.f));
    AddText(Tree, EditorCard, TEXT("TextBlock_4"), TEXT(""), 16, MutedText);
    AddButton(Tree, EditorCard, TEXT("Button_SaveTeam"), TEXT("Team speichern"));
    AddButton(Tree, EditorCard, TEXT("Button_Add_Team_1"), TEXT("Als neues Team speichern"), Secondary);
    AddButton(Tree, EditorCard, TEXT("Button_Delete_Team_1"), TEXT("Team löschen"), Danger);
    AddButton(Tree, EditorCard, TEXT("Button_EditorBack"), TEXT("Zurück zur Teamliste"), Secondary);

    return SaveBlueprint(Blueprint);
}

bool RebuildListEntry(const TCHAR* Path)
{
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, Path);
    if (!Blueprint || !Blueprint->WidgetTree) return false;
    UWidgetTree* Tree = Blueprint->WidgetTree;
    Tree->Modify();
    ClearWidgetTree(Tree);
    UBorder* Border = Make<UBorder>(Tree, TEXT("EntryCard"));
    Border->SetBrushColor(FLinearColor(0.10f, 0.14f, 0.23f, 1.f));
    Border->SetPadding(FMargin(18.f, 11.f));
    UTextBlock* Label = Make<UTextBlock>(Tree, TEXT("TextBlock_338"));
    Label->SetColorAndOpacity(FSlateColor(MainText));
    FSlateFontInfo Font = Label->GetFont();
    Font.Size = 20;
    Label->SetFont(Font);
    Border->SetContent(Label);
    Tree->RootWidget = Border;
    return SaveBlueprint(Blueprint);
}

bool RebuildCharacterEntry()
{
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/TopDown/UI/BP_CharacterSelectionOption.BP_CharacterSelectionOption"));
    if (!Blueprint || !Blueprint->WidgetTree) return false;
    UWidgetTree* Tree = Blueprint->WidgetTree;
    Tree->Modify();
    ClearWidgetTree(Tree);
    UBorder* Border = Make<UBorder>(Tree, TEXT("CharacterEntryCard"));
    Border->SetBrushColor(FLinearColor(0.10f, 0.14f, 0.23f, 1.f));
    Border->SetPadding(FMargin(16.f, 9.f));
    UTextBlock* Label = Make<UTextBlock>(Tree, TEXT("TextBlock_66"));
    Label->SetColorAndOpacity(FSlateColor(MainText));
    FSlateFontInfo Font = Label->GetFont();
    Font.Size = 19;
    Label->SetFont(Font);
    Border->SetContent(Label);
    Tree->RootWidget = Border;
    return SaveBlueprint(Blueprint);
}

bool RebuildSelectorEntry(const TCHAR* Path)
{
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, Path);
    if (!Blueprint || !Blueprint->WidgetTree) return false;
    UWidgetTree* Tree = Blueprint->WidgetTree;
    Tree->Modify();
    ClearWidgetTree(Tree);
    UButton* Button = Make<UButton>(Tree, TEXT("Button_42"));
    Button->SetBackgroundColor(Secondary);
    UTextBlock* Label = Make<UTextBlock>(Tree, TEXT("TextBlock_40"));
    Label->SetColorAndOpacity(FSlateColor(MainText));
    FSlateFontInfo Font = Label->GetFont();
    Font.Size = 18;
    Label->SetFont(Font);
    Label->SetJustification(ETextJustify::Center);
    Button->AddChild(Label);
    Tree->RootWidget = Button;
    return SaveBlueprint(Blueprint);
}

bool RebuildSkillpoint()
{
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/TopDown/Blueprints/UI_Skillpoint.UI_Skillpoint"));
    if (!Blueprint || !Blueprint->WidgetTree) return false;
    UWidgetTree* Tree = Blueprint->WidgetTree;
    Tree->Modify();
    ClearWidgetTree(Tree);
    USizeBox* Root = Make<USizeBox>(Tree, TEXT("SkillpointSize"));
    Root->SetWidthOverride(34.f);
    Root->SetHeightOverride(34.f);
    UImage* Image = Make<UImage>(Tree, TEXT("Image_80"));
    Image->SetColorAndOpacity(FLinearColor(0.30f, 0.37f, 0.50f, 1.f));
    Root->AddChild(Image);
    Tree->RootWidget = Root;
    return SaveBlueprint(Blueprint);
}

bool RebuildSkillpointMenu(const TCHAR* Path)
{
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, Path);
    if (!Blueprint || !Blueprint->WidgetTree) return false;
    UWidgetTree* Tree = Blueprint->WidgetTree;
    Tree->Modify();
    ClearWidgetTree(Tree);
    UCanvasPanel* Root = Make<UCanvasPanel>(Tree, TEXT("SkillpointMenuRoot"));
    Root->SetClipping(EWidgetClipping::ClipToBounds);
    UBorder* BackgroundBorder = Make<UBorder>(Tree, TEXT("SkillpointMenuBackground"));
    BackgroundBorder->SetBrushColor(FLinearColor(0.035f, 0.05f, 0.085f, 1.f));
    FillCanvas(Root->AddChildToCanvas(BackgroundBorder), -1);
    UTextBlock* Title = Make<UTextBlock>(Tree, TEXT("TextBlock_0"));
    Title->SetColorAndOpacity(FSlateColor(MainText));
    FSlateFontInfo Font = Title->GetFont();
    Font.Size = 21;
    Title->SetFont(Font);
    UCanvasPanelSlot* TitleSlot = Root->AddChildToCanvas(Title);
    TitleSlot->SetPosition(FVector2D(18.f, 14.f));
    TitleSlot->SetSize(FVector2D(460.f, 34.f));
    UCanvasPanel* Points = Make<UCanvasPanel>(Tree, TEXT("CanvasPanel_Skills1"));
    UCanvasPanelSlot* PointsSlot = Root->AddChildToCanvas(Points);
    PointsSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
    PointsSlot->SetOffsets(FMargin(0.f, 48.f, 0.f, 0.f));
    Tree->RootWidget = Root;
    return SaveBlueprint(Blueprint);
}

bool RebuildSkillMenu()
{
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/TopDown/Blueprints/UI_SkillMenu.UI_SkillMenu"));
    if (!Blueprint || !Blueprint->WidgetTree) return false;
    UWidgetTree* Tree = Blueprint->WidgetTree;
    Tree->Modify();
    ClearWidgetTree(Tree);
    UBorder* Root = Make<UBorder>(Tree, TEXT("SkillMenuCard"));
    Root->SetBrushColor(FLinearColor(0.035f, 0.05f, 0.085f, 1.f));
    Root->SetPadding(FMargin(18.f));
    UHorizontalBox* Columns = Make<UHorizontalBox>(Tree, TEXT("SkillMenuColumns"));
    Root->SetContent(Columns);
    USizeBox* NavigationSize = Make<USizeBox>(Tree, TEXT("SkillNavigationSize"));
    NavigationSize->SetWidthOverride(230.f);
    UHorizontalBoxSlot* NavigationSlot = Columns->AddChildToHorizontalBox(NavigationSize);
    NavigationSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
    UVerticalBox* Navigation = Make<UVerticalBox>(Tree, TEXT("SkillNavigation"));
    NavigationSize->AddChild(Navigation);
    AddText(Tree, Navigation, TEXT("Text_CharacterLabel"), TEXT("CHARAKTER"), 16, MutedText);
    UComboBoxKey* CharacterCombo = Make<UComboBoxKey>(Tree, TEXT("ComboBoxKey_0"));
    Navigation->AddChildToVerticalBox(CharacterCombo)->SetPadding(FMargin(0.f, 3.f, 0.f, 10.f));
    AddText(Tree, Navigation, TEXT("SkillpointCounter"), TEXT("0 SKILLPUNKTE"), 17, FLinearColor(0.35f, 0.62f, 1.f));
    UVerticalBox* Selector = Make<UVerticalBox>(Tree, TEXT("SkillmenuSelector"));
    UVerticalBoxSlot* SelectorSlot = Navigation->AddChildToVerticalBox(Selector);
    SelectorSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    SelectorSlot->SetPadding(FMargin(0.f, 6.f));
    UBorder* ContentCard = Make<UBorder>(Tree, TEXT("SkillContentCard"));
    ContentCard->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.06f, 1.f));
    ContentCard->SetPadding(FMargin(12.f));
    ContentCard->SetClipping(EWidgetClipping::ClipToBounds);
    UHorizontalBoxSlot* ContentSlot = Columns->AddChildToHorizontalBox(ContentCard);
    ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UVerticalBox* ContentColumn = Make<UVerticalBox>(Tree, TEXT("SkillContentColumn"));
    ContentCard->SetContent(ContentColumn);
    UHorizontalBox* Tabs = Make<UHorizontalBox>(Tree, TEXT("ContentTabs"));
    ContentColumn->AddChildToVerticalBox(Tabs)->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
    UButton* SkillTab = Make<UButton>(Tree, TEXT("Button_SkillpointsTab"));
    SkillTab->SetBackgroundColor(Primary);
    UTextBlock* SkillTabLabel = Make<UTextBlock>(Tree, TEXT("Button_SkillpointsTab_Label"));
    SkillTabLabel->SetText(FText::FromString(TEXT("Skillpunkte")));
    SkillTabLabel->SetColorAndOpacity(FSlateColor(MainText));
    SkillTabLabel->SetJustification(ETextJustify::Center);
    SkillTab->AddChild(SkillTabLabel);
    UHorizontalBoxSlot* SkillTabSlot = Tabs->AddChildToHorizontalBox(SkillTab);
    SkillTabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    SkillTabSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
    UButton* StartTab = Make<UButton>(Tree, TEXT("Button_StartPositionTab"));
    StartTab->SetBackgroundColor(Secondary);
    UTextBlock* StartTabLabel = Make<UTextBlock>(Tree, TEXT("Button_StartPositionTab_Label"));
    StartTabLabel->SetText(FText::FromString(TEXT("Startposition")));
    StartTabLabel->SetColorAndOpacity(FSlateColor(MainText));
    StartTabLabel->SetJustification(ETextJustify::Center);
    StartTab->AddChild(StartTabLabel);
    UHorizontalBoxSlot* StartTabSlot = Tabs->AddChildToHorizontalBox(StartTab);
    StartTabSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    StartTabSlot->SetPadding(FMargin(4.f, 0.f, 0.f, 0.f));
    UWidgetSwitcher* Switcher = Make<UWidgetSwitcher>(Tree, TEXT("ContentSwitcher"));
    UVerticalBoxSlot* SwitcherSlot = ContentColumn->AddChildToVerticalBox(Switcher);
    SwitcherSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    UCanvasPanel* Holder = Make<UCanvasPanel>(Tree, TEXT("MenuHolder"));
    Holder->SetClipping(EWidgetClipping::ClipToBounds);
    Switcher->AddChild(Holder);
    UVerticalBox* StartColumn = Make<UVerticalBox>(Tree, TEXT("StartPositionColumn"));
    Switcher->AddChild(StartColumn);
    AddText(Tree, StartColumn, TEXT("Text_StartPositionLabel"), TEXT("STARTPOSITION"), 16, MutedText);
    UWidgetBlueprint* StartPositionBlueprint = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/TopDown/UI/UI_StartPosition.UI_StartPosition"));
    if (StartPositionBlueprint && StartPositionBlueprint->GeneratedClass)
    {
        const TSubclassOf<URoundStartPositionWidget> StartPositionClass = Cast<UClass>(StartPositionBlueprint->GeneratedClass);
        URoundStartPositionWidget* StartPosition = Tree->ConstructWidget<URoundStartPositionWidget>(
            StartPositionClass, TEXT("UI_StartPosition_0"));
        if (FBoolProperty* VariableProperty = FindFProperty<FBoolProperty>(StartPosition->GetClass(), TEXT("bIsVariable")))
        {
            VariableProperty->SetPropertyValue_InContainer(StartPosition, true);
        }
        UVerticalBoxSlot* StartSlot = StartColumn->AddChildToVerticalBox(StartPosition);
        StartSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        StartSlot->SetPadding(FMargin(0.f, 4.f));
    }
    Tree->RootWidget = Root;
    return SaveBlueprint(Blueprint);
}

bool RebuildStartPosition()
{
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/TopDown/UI/UI_StartPosition.UI_StartPosition"));
    if (!Blueprint || !Blueprint->WidgetTree) return false;
    UWidgetTree* Tree = Blueprint->WidgetTree;
    Tree->Modify();
    ClearWidgetTree(Tree);
    USizeBox* Size = Make<USizeBox>(Tree, TEXT("StartPositionSize"));
    Size->SetWidthOverride(340.f);
    Size->SetHeightOverride(275.f);
    UBorder* BackgroundBorder = Make<UBorder>(Tree, TEXT("StartPositionBackground"));
    BackgroundBorder->SetBrushColor(FLinearColor(0.045f, 0.065f, 0.105f, 1.f));
    BackgroundBorder->SetPadding(FMargin(10.f));
    Size->AddChild(BackgroundBorder);
    UCanvasPanel* Canvas = Make<UCanvasPanel>(Tree, TEXT("CanvasPanel_58"));
    Canvas->SetClipping(EWidgetClipping::ClipToBounds);
    BackgroundBorder->SetContent(Canvas);
    Tree->RootWidget = Size;
    return SaveBlueprint(Blueprint);
}

bool RebuildStartField()
{
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/TopDown/UI/UI_StartField.UI_StartField"));
    if (!Blueprint || !Blueprint->WidgetTree) return false;
    UWidgetTree* Tree = Blueprint->WidgetTree;
    Tree->Modify();
    ClearWidgetTree(Tree);
    UButton* Button = Make<UButton>(Tree, TEXT("Button_375"));
    Button->SetBackgroundColor(FLinearColor(0.10f, 0.14f, 0.23f, 1.f));
    UImage* Image = Make<UImage>(Tree, TEXT("Image_41"));
    Image->SetDesiredSizeOverride(FVector2D(34.f, 34.f));
    Image->SetColorAndOpacity(FLinearColor(0.25f, 0.25f, 0.25f, 1.f));
    Button->AddChild(Image);
    Tree->RootWidget = Button;
    return SaveBlueprint(Blueprint);
}

UWidgetBlueprint* LoadOrCreateDebugBlueprint()
{
    constexpr const TCHAR* AssetPath = TEXT("/Game/TopDown/UI/WBP_DebugMode.WBP_DebugMode");
    if (UWidgetBlueprint* Existing = LoadObject<UWidgetBlueprint>(nullptr, AssetPath)) return Existing;
    UPackage* Package = CreatePackage(TEXT("/Game/TopDown/UI/WBP_DebugMode"));
    UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
        URoundDebugWidget::StaticClass(), Package, TEXT("WBP_DebugMode"), BPTYPE_Normal,
        UWidgetBlueprint::StaticClass(), UWidgetBlueprintGeneratedClass::StaticClass());
    if (Blueprint) FAssetRegistryModule::AssetCreated(Blueprint);
    return Cast<UWidgetBlueprint>(Blueprint);
}

bool RebuildDebugWidget()
{
    UWidgetBlueprint* Blueprint = LoadOrCreateDebugBlueprint();
    if (!Blueprint || !Blueprint->WidgetTree) return false;
    UWidgetTree* Tree = Blueprint->WidgetTree;
    Tree->Modify();
    ClearWidgetTree(Tree);

    UCanvasPanel* Root = Make<UCanvasPanel>(Tree, TEXT("DebugRoot"));
    USizeBox* Size = Make<USizeBox>(Tree, TEXT("DebugPanelSize"));
    Size->SetWidthOverride(390.f);
    UCanvasPanelSlot* SizeSlot = Root->AddChildToCanvas(Size);
    SizeSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 1.f));
    SizeSlot->SetAlignment(FVector2D(1.f, 0.f));
    SizeSlot->SetOffsets(FMargin(-24.f, 24.f, 390.f, -48.f));
    UBorder* Panel = Make<UBorder>(Tree, TEXT("DebugPanel"));
    Panel->SetBrushColor(Card);
    Panel->SetPadding(FMargin(22.f));
    Size->AddChild(Panel);
    UVerticalBox* Column = Make<UVerticalBox>(Tree, TEXT("DebugColumn"));
    Panel->SetContent(Column);
    AddText(Tree, Column, TEXT("Text_DebugTitle"), TEXT("DEBUGMODUS"), 30);
    AddText(Tree, Column, TEXT("Text_DebugHint"), TEXT("F10 öffnet und schließt dieses Fenster."), 15, MutedText);

    auto AddToggle = [Tree, Column](const TCHAR* Name, const TCHAR* Label)
    {
        UHorizontalBox* Row = Make<UHorizontalBox>(Tree, *FString::Printf(TEXT("%s_Row"), Name));
        Column->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.f, 8.f));
        UCheckBox* Check = Make<UCheckBox>(Tree, Name);
        Row->AddChildToHorizontalBox(Check)->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
        UTextBlock* Text = Make<UTextBlock>(Tree, *FString::Printf(TEXT("%s_Label"), Name));
        Text->SetText(FText::FromString(Label));
        Text->SetColorAndOpacity(FSlateColor(MainText));
        Row->AddChildToHorizontalBox(Text);
    };
    AddToggle(TEXT("Check_RevealMap"), TEXT("Gesamte Karte aufdecken"));
    AddToggle(TEXT("Check_TerrainLabels"), TEXT("Terraingebiete einfärben"));
    AddText(Tree, Column, TEXT("Text_TerrainTitle"), TEXT("FELD"), 18, MutedText);
    AddText(Tree, Column, TEXT("Text_TileInfo"), TEXT("Kein Feld ausgewählt"), 16);
    AddText(Tree, Column, TEXT("Text_CharacterTitle"), TEXT("CHARAKTER"), 18, MutedText);
    AddText(Tree, Column, TEXT("Text_CharacterInfo"), TEXT("Kein Charakter ausgewählt"), 16);
    AddButton(Tree, Column, TEXT("Button_FullHeal"), TEXT("Vollständig heilen"));
    AddButton(Tree, Column, TEXT("Button_Damage"), TEXT("Testschaden"), Danger);
    AddButton(Tree, Column, TEXT("Button_InitiativePlus"), TEXT("Initiative erhöhen"), Secondary);
    AddButton(Tree, Column, TEXT("Button_InitiativeMinus"), TEXT("Initiative verringern"), Secondary);
    AddButton(Tree, Column, TEXT("Button_ResetCooldowns"), TEXT("Cooldowns zurücksetzen"));
    AddButton(Tree, Column, TEXT("Button_Close"), TEXT("Schließen"), Secondary);
    Tree->RootWidget = Root;
    return SaveBlueprint(Blueprint);
}
}
#endif

bool URoundWidgetBlueprintBuilder::RebuildMenuWidgetBlueprints()
{
#if WITH_EDITOR
    const bool bMain = RebuildMainMenu();
    const bool bTeams = RebuildListEntry(TEXT("/Game/TopDown/Blueprints/UI_TeamsUserWidget.UI_TeamsUserWidget"));
    const bool bSessions = RebuildListEntry(TEXT("/Game/TopDown/Blueprints/UI_SessionsUserWidget.UI_SessionsUserWidget"));
    const bool bCharacters = RebuildCharacterEntry();
    const bool bSkillSelector = RebuildSelectorEntry(TEXT("/Game/TopDown/Blueprints/BPW_SkillmenuSelectorPoint.BPW_SkillmenuSelectorPoint"));
    const bool bCharacterSelector = RebuildSelectorEntry(TEXT("/Game/TopDown/Blueprints/BPW_SkillmenuCharacterSelectorPoint.BPW_SkillmenuCharacterSelectorPoint"));
    const bool bSkillpoint = RebuildSkillpoint();
    const bool bSkillpointMenu = RebuildSkillpointMenu(TEXT("/Game/TopDown/Blueprints/UI_SkillpointMenu.UI_SkillpointMenu"));
    const bool bCharacterSkillpointMenu = RebuildSkillpointMenu(TEXT("/Game/TopDown/UI/UI_CharacterSkillpointMenu.UI_CharacterSkillpointMenu"));
    const bool bStartPosition = RebuildStartPosition();
    const bool bStartField = RebuildStartField();
    const bool bSkillMenu = RebuildSkillMenu();
    return bMain && bTeams && bSessions && bCharacters && bSkillSelector && bCharacterSelector
        && bSkillpoint && bSkillpointMenu && bCharacterSkillpointMenu && bStartPosition && bStartField && bSkillMenu;
#else
    return false;
#endif
}

bool URoundWidgetBlueprintBuilder::RebuildDebugWidgetBlueprint()
{
#if WITH_EDITOR
    return RebuildDebugWidget();
#else
    return false;
#endif
}
