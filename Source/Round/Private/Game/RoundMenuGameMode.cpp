#include "Game/RoundMenuGameMode.h"

#include "Game/RoundMenuController.h"

ARoundMenuGameMode::ARoundMenuGameMode()
{
    PlayerControllerClass = ARoundMenuController::StaticClass();
}
