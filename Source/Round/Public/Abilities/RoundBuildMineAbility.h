#pragma once

#include "CoreMinimal.h"
#include "Abilities/RoundBuildDeviceAbility.h"
#include "RoundBuildMineAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundBuildMineAbility : public ARoundBuildDeviceAbility
{
    GENERATED_BODY()

public:
    ARoundBuildMineAbility();

protected:
    virtual TSubclassOf<class ARoundEngineerDeviceActor> ResolveDeviceClass() const override;
    virtual void ConfigureSpawnedDevice(class ARoundEngineerDeviceActor* SpawnedDevice) const override;
};
