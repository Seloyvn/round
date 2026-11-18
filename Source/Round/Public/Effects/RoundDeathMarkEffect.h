#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundDeathMarkEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundDeathMarkEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
    int32 ComboGainOnTrigger = 1;

    double DamageMultiplierFromCauser = 1.15;
    int32 DefenseIgnoreFromCauser = 0;
    int32 MoveDurationExtension = 0;
    int32 ExecuteCooldownReductionOnMaxCombo = 0;
    int32 DeathMarkCooldownReductionOnExpiry = 0;
    int32 TransferRangeOnKill = 0;
    double LowHealthThreshold = 0.0;
    double LowHealthDamageMultiplierAdd = 0.0;
    bool bComboCanTriggerFromAllies = false;
    bool bPreventInvisibility = false;
    bool bGrantComboEachCauserTurn = false;
    bool bTransferred = false;

    virtual bool IsDebuff() const override { return true; }
    virtual int32 ModifyIncomingDamage(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;
    virtual void OnDamageTaken(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 AppliedDamage, ERoundDamageType DamageType) override;
    virtual void OnOwnerMoved(class ARoundCharacter* TargetCharacter, class ARoundGridTile* PreviousTile, class ARoundGridTile* NewTile) override;
    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount) override;
    virtual bool PreventsEffectApplication(const ARoundEffectActor* IncomingEffect) const override;

private:
    bool bInitialComboGranted = false;
    bool bMoveExtensionUsed = false;
    int32 LastComboGrantTurnSequence = -1;

    void ReduceAbilityCooldown(FName AbilityId, int32 ReductionAmount) const;
    void TransferToNearestEnemy(class ARoundCharacter* DeadTarget);
};
