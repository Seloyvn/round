#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundAbilityActor.h"
#include "RoundBuildDeviceAbility.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundBuildDeviceAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual bool CanTargetTile(const class ARoundGridTile* TargetTile) const override;
    virtual int32 GetAbilityRange() const override;
    virtual FRoundActionResult ExecuteResolvedAbility(class ARoundGridTile* TargetTile) override;

protected:
    virtual TSubclassOf<class ARoundEngineerDeviceActor> ResolveDeviceClass() const;
    virtual void ConfigureSpawnedDevice(class ARoundEngineerDeviceActor* SpawnedDevice) const;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round|Engineer")
    TSubclassOf<class ARoundEngineerDeviceActor> DeviceClass;
};
