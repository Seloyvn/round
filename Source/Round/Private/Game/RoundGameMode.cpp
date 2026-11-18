#include "Game/RoundGameMode.h"

#include "Actors/RoundTeamActor.h"
#include "Game/RoundPlayerPawn.h"
#include "Game/RoundGameState.h"
#include "Game/RoundGameInstance.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

ARoundGameMode::ARoundGameMode()
{
    GameStateClass = ARoundGameState::StaticClass();
    DefaultPawnClass = ARoundPlayerPawn::StaticClass();
}

void ARoundGameMode::BeginPlay()
{
    Super::BeginPlay();
    StartMatchFlow();
}

void ARoundGameMode::StartMatchFlow()
{
    if (ARoundGameState* RoundGameState = GetGameState<ARoundGameState>())
    {
        RoundGameState->RebuildTurnOrder();
        if (!RoundGameState->CurrentTurnCharacter)
        {
            RoundGameState->SetNextTurn();
        }
    }
}

void ARoundGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    TryAssignPlayerToTeam(NewPlayer);
}

void ARoundGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    Super::HandleStartingNewPlayer_Implementation(NewPlayer);
    TryAssignPlayerToTeam(NewPlayer);
}

void ARoundGameMode::TryAssignPlayerToTeam(APlayerController* NewPlayer)
{
    ARoundGameState* RoundGameState = GetGameState<ARoundGameState>();
    ARoundPlayerPawn* PlayerPawn = NewPlayer ? Cast<ARoundPlayerPawn>(NewPlayer->GetPawn()) : nullptr;
    if (!RoundGameState || !PlayerPawn)
    {
        if (NewPlayer)
        {
            RetryAssignPlayerToTeam(NewPlayer, 30);
        }
        return;
    }

    const int32 PlayerSlot = ResolvePlayerSlot(NewPlayer);
    if (const URoundGameInstance* GameInstance = GetGameInstance<URoundGameInstance>();
        GameInstance && !GameInstance->bHostingRoom && !GameInstance->bJoinedRoom
        && GameInstance->LoadedTeams.IsValidIndex(PlayerSlot))
    {
        PlayerPawn->TeamDefinition = GameInstance->LoadedTeams[PlayerSlot];
    }
    if (PlayerSlot == 0)
    {
        RoundGameState->Player1 = PlayerPawn;
        if (RoundGameState->Teams.IsValidIndex(0))
        {
            PlayerPawn->SetTeamActor(Cast<ARoundTeamActor>(RoundGameState->Teams[0]));
        }
    }
    else if (PlayerSlot == 1)
    {
        RoundGameState->Player2 = PlayerPawn;
        if (RoundGameState->Teams.IsValidIndex(1))
        {
            PlayerPawn->SetTeamActor(Cast<ARoundTeamActor>(RoundGameState->Teams[1]));
        }
    }

    PlayerPawn->ForceNetUpdate();
    RoundGameState->TryCreateTeamsFromPlayerSelections();
    RoundGameState->SetVision();

    TWeakObjectPtr<ARoundGameState> WeakRoundGameState = RoundGameState;
    for (int32 RetryIndex = 1; RetryIndex <= 6; ++RetryIndex)
    {
        const float DelaySeconds = 0.2f * RetryIndex;
        FTimerHandle RefreshTimerHandle;
        GetWorldTimerManager().SetTimer(
            RefreshTimerHandle,
            [WeakRoundGameState]()
            {
                if (WeakRoundGameState.IsValid())
                {
                    WeakRoundGameState->SetVision();
                }
            },
            DelaySeconds,
            false);
    }
}

void ARoundGameMode::RetryAssignPlayerToTeam(APlayerController* NewPlayer, const int32 RemainingAttempts)
{
    if (!NewPlayer || RemainingAttempts <= 0)
    {
        return;
    }

    TWeakObjectPtr<APlayerController> WeakPlayer = NewPlayer;
    GetWorldTimerManager().SetTimerForNextTick([this, WeakPlayer, RemainingAttempts]()
    {
        if (!WeakPlayer.IsValid())
        {
            return;
        }

        ARoundPlayerPawn* PlayerPawn = Cast<ARoundPlayerPawn>(WeakPlayer->GetPawn());
        if (PlayerPawn)
        {
            TryAssignPlayerToTeam(WeakPlayer.Get());
            return;
        }

        RetryAssignPlayerToTeam(WeakPlayer.Get(), RemainingAttempts - 1);
    });
}

int32 ARoundGameMode::ResolvePlayerSlot(APlayerController* NewPlayer) const
{
    if (!NewPlayer)
    {
        return INDEX_NONE;
    }

    TArray<APlayerController*> Controllers;
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (APlayerController* Controller = It->Get())
        {
            Controllers.Add(Controller);
        }
    }

    Controllers.Sort([](const APlayerController& Left, const APlayerController& Right)
    {
        const APlayerState* LeftState = Left.PlayerState;
        const APlayerState* RightState = Right.PlayerState;
        const int32 LeftId = LeftState ? LeftState->GetPlayerId() : MAX_int32;
        const int32 RightId = RightState ? RightState->GetPlayerId() : MAX_int32;
        return LeftId < RightId;
    });

    return Controllers.IndexOfByKey(NewPlayer);
}
