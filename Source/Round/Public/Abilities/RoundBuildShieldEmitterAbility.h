#pragma once

#include "CoreMinimal.h"
#include "Abilities/RoundBuildDeviceAbility.h"
#include "RoundBuildShieldEmitterAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundBuildShieldEmitterAbility : public ARoundBuildDeviceAbility
{
    GENERATED_BODY()

public:
    ARoundBuildShieldEmitterAbility();

protected:
    virtual TSubclassOf<class ARoundEngineerDeviceActor> ResolveDeviceClass() const override;
    virtual void ConfigureSpawnedDevice(class ARoundEngineerDeviceActor* SpawnedDevice) const override;
};
