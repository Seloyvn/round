#include "Game/RoundTopDownGameMode.h"

#include "Game/RoundTopDownController.h"

ARoundTopDownGameMode::ARoundTopDownGameMode()
{
    PlayerControllerClass = ARoundTopDownController::StaticClass();
}
