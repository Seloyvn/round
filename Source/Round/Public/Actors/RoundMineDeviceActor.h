#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEngineerDeviceActor.h"
#include "RoundMineDeviceActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundMineDeviceActor : public ARoundEngineerDeviceActor
{
    GENERATED_BODY()

public:
    ARoundMineDeviceActor();

    UFUNCTION(BlueprintCallable, Category = "Round|Engineer|Mine")
    void SetArmDelay(int32 InArmDelayInitiative);

    void ApplyCoreMineState(int32 InArmDelayInitiative, int32 InEnemyRevealInitiative);
    int32 GetRemainingArmDelayInitiative() const { return RemainingArmDelayInitiative; }
    int32 GetRemainingEnemyRevealInitiative() const { return RemainingEnemyRevealInitiative; }

    virtual void OnInitiativeAdvanced(int32 InitiativeAmount) override;
    virtual bool CanTriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext) const override;
    virtual FRoundActionResult TriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Mine")
    int32 ExplosionDamage = 140;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Mine")
    int32 ExplosionRadius = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Mine")
    ERoundDamageType ExplosionDamageType = ERoundDamageType::Physical;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Mine")
    int32 ArmDelayInitiative = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Mine")
    int32 EnemyRevealInitiative = 20;

    int32 DefenseIgnore = 0;
    int32 InitiativeLossOnHit = 0;
    int32 RootDurationInitiative = 0;
    bool bPreserveOnManualTrigger = false;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Mine")
    int32 RemainingArmDelayInitiative = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Engineer|Mine")
    int32 RemainingEnemyRevealInitiative = 0;

public:
    virtual bool IsDeviceVisibleToTeam_Implementation(ERoundTeam ViewingTeam, bool bTileInVision) const override;
};
