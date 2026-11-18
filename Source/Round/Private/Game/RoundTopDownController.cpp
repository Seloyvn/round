#include "Game/RoundTopDownController.h"

#include "GameFramework/Pawn.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/KismetMathLibrary.h"

ARoundTopDownController::ARoundTopDownController()
{
    PrimaryActorTick.bCanEverTick = true;
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableMouseOverEvents = true;
}

void ARoundTopDownController::BeginPlay()
{
    Super::BeginPlay();

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
}

void ARoundTopDownController::PlayerTick(const float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    UpdateCameraMovement(DeltaTime);
}

void ARoundTopDownController::UpdateCameraMovement(const float DeltaTime)
{
    APawn* ControlledPawn = GetPawn();
    if (!IsLocalController() || !ControlledPawn)
    {
        return;
    }

    FVector2D MousePosition = FVector2D::ZeroVector;
    if (!GetMousePosition(MousePosition.X, MousePosition.Y))
    {
        return;
    }

    const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
    if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
    {
        return;
    }

    const double NormalizedX = MousePosition.X / ViewportSize.X;
    const double NormalizedY = MousePosition.Y / ViewportSize.Y;

    double HorizontalDirection = 0.0;
    if (NormalizedX < CameraEdgeThreshold)
    {
        HorizontalDirection = -1.0;
    }
    else if (NormalizedX > (1.0 - CameraEdgeThreshold))
    {
        HorizontalDirection = 1.0;
    }

    double VerticalDirection = 0.0;
    if (NormalizedY < CameraEdgeThreshold)
    {
        VerticalDirection = 1.0;
    }
    else if (NormalizedY > (1.0 - CameraEdgeThreshold))
    {
        VerticalDirection = -1.0;
    }

    if (HorizontalDirection == 0.0 && VerticalDirection == 0.0)
    {
        return;
    }

    const FRotator CurrentControlRotation = GetControlRotation();
    const FVector RightVector = UKismetMathLibrary::GetRightVector(FRotator(0.0f, CurrentControlRotation.Yaw, 0.0f));
    const FVector ForwardVector = UKismetMathLibrary::GetForwardVector(FRotator(0.0f, CurrentControlRotation.Yaw, 0.0f));

    const FVector MoveOffset =
        ((RightVector * HorizontalDirection) + (ForwardVector * VerticalDirection)) *
        CameraMoveSpeed * DeltaTime * 100.0f;

    ControlledPawn->AddActorWorldOffset(MoveOffset, false);
}
