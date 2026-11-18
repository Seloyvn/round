#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEngineerDeviceActor.h"
#include "RoundShieldEmitterDeviceActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundShieldEmitterDeviceActor : public ARoundEngineerDeviceActor
{
    GENERATED_BODY()

public:
    ARoundShieldEmitterDeviceActor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|ShieldEmitter")
    int32 ShieldAmount = 90;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|ShieldEmitter")
    int32 ShieldRadius = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|ShieldEmitter")
    int32 ShieldDurationInitiative = 60;

    int32 HealAmount = 0;
    int32 InitiativeGain = 0;
    bool bCleanseOneDebuff = false;
    double SoloShieldMultiplier = 0.0;

    virtual FRoundActionResult TriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext) override;
};
