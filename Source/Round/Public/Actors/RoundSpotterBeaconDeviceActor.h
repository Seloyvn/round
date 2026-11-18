#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEngineerDeviceActor.h"
#include "RoundSpotterBeaconDeviceActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundSpotterBeaconDeviceActor : public ARoundEngineerDeviceActor
{
    GENERATED_BODY()

public:
    ARoundSpotterBeaconDeviceActor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Spotter")
    int32 BeaconRadius = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Spotter")
    int32 DebuffDurationInitiative = 60;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Spotter")
    double DamageTakenMultiplier = 1.25;

    int32 InitiativeLoss = 0;
    int32 MoveCostAdd = 0;
    double OutgoingDamageMultiplier = 0.0;

    virtual FRoundActionResult TriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext) override;
    virtual TArray<class ARoundGridTile*> GetDeviceVisionFields_Implementation() const override;
};
