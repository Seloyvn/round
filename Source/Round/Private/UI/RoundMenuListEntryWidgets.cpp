#include "UI/RoundMenuListEntryWidgets.h"

#include "Components/TextBlock.h"
#include "UI/RoundMainMenuWidget.h"
#include "UObject/UnrealType.h"

void URoundTeamsListEntryWidget::SetTeamName(const FString& InTeamName)
{
    TeamName = InTeamName;
    RefreshLabel();
}

void URoundTeamsListEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    RefreshLabel();
}

FReply URoundTeamsListEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (MainMenu && Index != INDEX_NONE)
    {
        MainMenu->TeamIndexSelected(Index);
        MainMenu->SelectTeamToEdit();
        return FReply::Handled();
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void URoundTeamsListEntryWidget::RefreshLabel()
{
    if (TextBlock_338)
    {
        TextBlock_338->SetText(FText::FromString(TeamName));
    }
}

void URoundSessionsListEntryWidget::SetSessionLabel(const FString& InSessionLabel)
{
    SessionLabel = InSessionLabel;
    RefreshLabel();
}

void URoundSessionsListEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

    SessionLabel.Reset();
    if (ListItemObject)
    {
        static const FName SessionNamePropertyName(TEXT("SessionName"));
        static const FName NamePropertyName(TEXT("Name"));

        if (const FStrProperty* SessionNameProperty = FindFProperty<FStrProperty>(ListItemObject->GetClass(), SessionNamePropertyName))
        {
            SessionLabel = SessionNameProperty->GetPropertyValue_InContainer(ListItemObject);
        }
        else if (const FStrProperty* NameProperty = FindFProperty<FStrProperty>(ListItemObject->GetClass(), NamePropertyName))
        {
            SessionLabel = NameProperty->GetPropertyValue_InContainer(ListItemObject);
        }
        else
        {
            SessionLabel = ListItemObject->GetName();
        }
    }

    RefreshLabel();
}

void URoundSessionsListEntryWidget::RefreshLabel()
{
    if (TextBlock_338)
    {
        TextBlock_338->SetText(FText::FromString(SessionLabel));
    }
}
