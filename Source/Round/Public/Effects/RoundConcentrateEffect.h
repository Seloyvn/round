#pragma once
#include "Actors/RoundEffectActor.h"
#include "RoundConcentrateEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundConcentrateEffect : public ARoundEffectActor
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 DefenseReductionOnHit = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 DefenseReductionDuration = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double CurrentInitiativeLossFraction = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bGrantFreeStepAfterHit = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bBasicAttackConsumesNoAmmo = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AbilityCooldownReductionOnConsume = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double SplashDamageFraction = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bPersistOnKill = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bDumpMagazineAfterBasic = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RemainingEmpoweredHits = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double SecondHitBonusFraction = 0.5;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bExpireOnMove = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bExpireOnDamage = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bBlockAmmoRegeneration = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double ChargeBonusPerInterval = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) double MaxChargeBonus = 0.0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ChargeInterval = 10;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 PendingAmmoReload = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 AmmoReloadDelay = 10;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bHitPendingResolution = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 LastEmpoweredDamage = 0;

    virtual bool IsBuff() const override { return true; }
    virtual int32 ModifyOutgoingDamage(class ARoundCharacter*, class ARoundCharacter*, int32, ERoundDamageType, int32&) const override;
    virtual void OnInitiativeAdvanced(class ARoundCharacter*, int32) override;
    virtual void OnOwnerMoved(class ARoundCharacter*, class ARoundGridTile*, class ARoundGridTile*) override;
    virtual void OnDamageTaken(class ARoundCharacter*, class ARoundCharacter*, int32, ERoundDamageType) override;
    void ResolveEmpoweredHit(class ARoundCharacter2*, class ARoundCharacter*, class ARoundAbilityActor*, bool, bool);
private:
    double InitialAmount = 0.0;
    int32 ChargeElapsed = 0;
    int32 AmmoReloadElapsed = 0;
};
