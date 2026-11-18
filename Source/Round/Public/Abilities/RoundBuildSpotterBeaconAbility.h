#pragma once

#include "CoreMinimal.h"
#include "Abilities/RoundBuildDeviceAbility.h"
#include "RoundBuildSpotterBeaconAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundBuildSpotterBeaconAbility : public ARoundBuildDeviceAbility
{
    GENERATED_BODY()

public:
    ARoundBuildSpotterBeaconAbility();

protected:
    virtual TSubclassOf<class ARoundEngineerDeviceActor> ResolveDeviceClass() const override;
    virtual void ConfigureSpawnedDevice(class ARoundEngineerDeviceActor* SpawnedDevice) const override;
};
