#pragma once

#include "CoreMinimal.h"
#include "Components/Button.h"
#include "RoundRoomSessionButton.generated.h"

class URoundMainMenuWidget;

UCLASS()
class ROUND_API URoundRoomSessionButton : public UButton
{
    GENERATED_BODY()

public:
    URoundRoomSessionButton(const FObjectInitializer& ObjectInitializer);

    void Configure(URoundMainMenuWidget* InMainMenu, int32 InSessionIndex);

private:
    UFUNCTION()
    void HandleClicked();

    UPROPERTY()
    TObjectPtr<URoundMainMenuWidget> MainMenu = nullptr;

    int32 SessionIndex = INDEX_NONE;
};
