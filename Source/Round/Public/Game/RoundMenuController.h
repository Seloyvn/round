#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RoundMenuController.generated.h"

class URoundMainMenuWidget;

UCLASS(Blueprintable)
class ROUND_API ARoundMenuController : public APlayerController
{
    GENERATED_BODY()

public:
    ARoundMenuController();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Menu")
    TSubclassOf<URoundMainMenuWidget> MainMenuClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Menu")
    TObjectPtr<URoundMainMenuWidget> UI = nullptr;

protected:
    virtual void BeginPlay() override;

private:
    void CreateMainMenu();
};
