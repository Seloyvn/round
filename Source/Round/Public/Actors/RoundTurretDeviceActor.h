#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEngineerDeviceActor.h"
#include "RoundTurretDeviceActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundTurretDeviceActor : public ARoundEngineerDeviceActor
{
    GENERATED_BODY()

public:
    ARoundTurretDeviceActor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Turret")
    int32 TurretRange = 4;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Turret")
    int32 TurretDamage = 90;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Turret")
    ERoundDamageType TurretDamageType = ERoundDamageType::Physical;

    int32 DefenseIgnore = 0;
    int32 InitiativeLossOnHit = 0;
    double SecondaryTargetDamageMultiplier = 0.0;
    double LowHealthThreshold = 0.0;
    double LowHealthDamageMultiplier = 0.0;

    virtual FRoundActionResult TriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext) override;
    virtual bool BlocksCharactersOnTile_Implementation() const override;
};
