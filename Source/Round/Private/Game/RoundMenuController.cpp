#include "Game/RoundMenuController.h"

#include "Blueprint/UserWidget.h"
#include "Data/RoundGameDataRegistryLookup.h"
#include "UI/RoundMainMenuWidget.h"

ARoundMenuController::ARoundMenuController()
{
    bShowMouseCursor = true;
}

void ARoundMenuController::BeginPlay()
{
    Super::BeginPlay();
    CreateMainMenu();
}

void ARoundMenuController::CreateMainMenu()
{
    if (UI || !IsLocalController())
    {
        return;
    }

    TSubclassOf<URoundMainMenuWidget> WidgetClass;
    if (MainMenuClass)
    {
        WidgetClass = MainMenuClass;
    }
    else
    {
        WidgetClass = Cast<UClass>(RoundGameDataRegistryLookup::ResolveMainMenuWidgetClass().Get());
        if (!WidgetClass)
        {
            WidgetClass = URoundMainMenuWidget::StaticClass();
        }
    }

    UI = CreateWidget<URoundMainMenuWidget>(this, WidgetClass);
    if (!UI)
    {
        return;
    }

    UI->AddToViewport();

    FInputModeUIOnly InputMode;
    InputMode.SetWidgetToFocus(UI->TakeWidget());
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
    bShowMouseCursor = true;
}
