#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RoundGameMode.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ARoundGameMode();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round")
    TSubclassOf<class ARoundCharacter> DefaultRoundCharacterClass;

    UFUNCTION(BlueprintCallable, Category = "Round")
    virtual void StartMatchFlow();

    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
    virtual void BeginPlay() override;

private:
    void TryAssignPlayerToTeam(APlayerController* NewPlayer);
    void RetryAssignPlayerToTeam(APlayerController* NewPlayer, int32 RemainingAttempts);
    int32 ResolvePlayerSlot(APlayerController* NewPlayer) const;
};
