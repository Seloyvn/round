#pragma once

#include "Actors/RoundEffectActor.h"
#include "RoundSharedBreachEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundSharedBreachEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Effect")
    int32 DefenseIgnorePerCharacter = 0;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual int32 ModifyIncomingDamage(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;
    virtual bool IsDebuff() const override { return true; }

private:
    mutable TSet<TWeakObjectPtr<ARoundCharacter>> TriggeredCharacters;
};
