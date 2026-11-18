#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RoundTopDownController.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundTopDownController : public APlayerController
{
    GENERATED_BODY()

public:
    ARoundTopDownController();

protected:
    virtual void BeginPlay() override;
    virtual void PlayerTick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Camera")
    float CameraEdgeThreshold = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Camera")
    float CameraMoveSpeed = 10.0f;

private:
    void UpdateCameraMovement(float DeltaTime);
};
