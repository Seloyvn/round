#include "UI/RoundRoomSessionButton.h"

#include "UI/RoundMainMenuWidget.h"

URoundRoomSessionButton::URoundRoomSessionButton(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    OnClicked.AddDynamic(this, &URoundRoomSessionButton::HandleClicked);
}

void URoundRoomSessionButton::Configure(URoundMainMenuWidget* InMainMenu, const int32 InSessionIndex)
{
    MainMenu = InMainMenu;
    SessionIndex = InSessionIndex;
}

void URoundRoomSessionButton::HandleClicked()
{
    if (MainMenu)
    {
        MainMenu->JoinSessionByIndex(SessionIndex);
    }
}
