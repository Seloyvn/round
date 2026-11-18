#pragma once

#include "Actors/RoundFieldTriggerActor.h"
#include "RoundTypes.h"
#include "RoundAimFieldTriggerActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundAimFieldTriggerActor : public ARoundFieldTriggerActor
{
    GENERATED_BODY()

public:
    ARoundAimFieldTriggerActor();
    virtual void InitializeTrigger(class ARoundCharacter* InCauserCharacter, const TArray<class ARoundGridTile*>& InTriggerTiles) override;
    virtual void RemoveTrigger() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    int32 TriggerDamage = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    ERoundDamageType DamageType = ERoundDamageType::Physical;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    int32 TriggerInitiativeLoss = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    int32 AoEInitiativeLoss = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    int32 PersistTriggerCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    bool bBlockMovementBonusesOnTrigger = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    bool bOwnerDamageBonusOnTriggeredTarget = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    bool bExtraDamageIfTargetMoved = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    double DamageMultiplierVsLowHealth = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    int32 DefenseIgnore = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    double NextOutgoingDamageMultiplier = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    double MarkDamageMultiplier = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    int32 MarkDurationInitiative = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    bool bBonusIfOwnerStayed = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    double StayedDamageMultiplier = 1.0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    int32 ExtendDebuffDurationInitiative = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    int32 AmmoOnKill = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    int32 CooldownReductionOnKill = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    bool bCoverFire = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    bool bApplyTrackingMark = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Trigger")
    TObjectPtr<class ARoundGridTile> OwnerPlacementTile = nullptr;

protected:
    virtual void ExecuteTrigger_Implementation(class ARoundCharacter* EnteringCharacter, class ARoundGridTile* EnteredTile, class ARoundGridTile* PreviousTile) override;
    virtual bool CanTriggerCharacter_Implementation(class ARoundCharacter* EnteringCharacter, class ARoundGridTile* EnteredTile, class ARoundGridTile* PreviousTile) const override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY()
    TObjectPtr<AActor> VisualActor = nullptr;
};
