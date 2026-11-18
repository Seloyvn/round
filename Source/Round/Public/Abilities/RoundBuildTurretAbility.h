#pragma once

#include "CoreMinimal.h"
#include "Abilities/RoundBuildDeviceAbility.h"
#include "RoundBuildTurretAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundBuildTurretAbility : public ARoundBuildDeviceAbility
{
    GENERATED_BODY()

public:
    ARoundBuildTurretAbility();

protected:
    virtual TSubclassOf<class ARoundEngineerDeviceActor> ResolveDeviceClass() const override;
    virtual void ConfigureSpawnedDevice(class ARoundEngineerDeviceActor* SpawnedDevice) const override;
};
