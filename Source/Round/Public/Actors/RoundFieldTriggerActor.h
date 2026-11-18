#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoundFieldTriggerActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundFieldTriggerActor : public AActor
{
    GENERATED_BODY()

public:
    ARoundFieldTriggerActor();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    TObjectPtr<class ARoundCharacter> CauserCharacter = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    TArray<TObjectPtr<class ARoundGridTile>> TriggerTiles;

    UFUNCTION(BlueprintCallable, Category = "Trigger")
    virtual void InitializeTrigger(class ARoundCharacter* InCauserCharacter, const TArray<class ARoundGridTile*>& InTriggerTiles);

    UFUNCTION(BlueprintCallable, Category = "Trigger")
    virtual void NotifyCharacterEntered(class ARoundCharacter* EnteringCharacter, class ARoundGridTile* EnteredTile, class ARoundGridTile* PreviousTile);

    UFUNCTION(BlueprintCallable, Category = "Trigger")
    virtual void RemoveTrigger();

protected:
    UFUNCTION(BlueprintNativeEvent, Category = "Trigger")
    bool CanTriggerCharacter(class ARoundCharacter* EnteringCharacter, class ARoundGridTile* EnteredTile, class ARoundGridTile* PreviousTile) const;

    UFUNCTION(BlueprintNativeEvent, Category = "Trigger")
    void ExecuteTrigger(class ARoundCharacter* EnteringCharacter, class ARoundGridTile* EnteredTile, class ARoundGridTile* PreviousTile);
};
