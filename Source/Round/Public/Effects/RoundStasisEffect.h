#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEffectActor.h"
#include "RoundStasisEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundStasisEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool IsBuff() const override { return true; }
    virtual bool PreventsAbilityUse(const class ARoundAbilityActor* Ability) const override;
    virtual bool PreventsTurnActions() const override;
    virtual int32 GetTurnBlockRemainingInitiative() const override;
    virtual int32 ModifyInitiativeGain(int32 InitiativeAmount) const override;
    virtual int32 ModifyIncomingDamageBeforeMitigation(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const override;
    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Chronomancer") int32 HealOnRelease = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Chronomancer") int32 InitiativeGainOnRelease = 0;
};
