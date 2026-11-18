#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEffectActor.h"
#include "RoundDragoonThreatEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundDragoonThreatEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool IsBuff() const override;
    virtual bool IsRemovable() const override;
    virtual void OnTurnStarted(class ARoundCharacter* TargetCharacter) override;
    virtual void OnOtherCharacterMoved(class ARoundCharacter* TargetCharacter, class ARoundCharacter* MovedCharacter, class ARoundGridTile* PreviousTile, class ARoundGridTile* NewTile) override;
    virtual int32 ModifyIncomingDamage(class ARoundCharacter* TargetCharacter, class ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Round|Dragoon")
    int32 TriggerDamage = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Round|Dragoon")
    ERoundDamageType TriggerDamageType = ERoundDamageType::Physical;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Round|Dragoon")
    int32 TriggerDefenseIgnore = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Round|Dragoon") int32 ThreatRangeAdd = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Round|Dragoon") int32 InitiativeLossOnTrigger = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Round|Dragoon") double IncomingDamageMultiplier = 1.0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Round|Dragoon") int32 MaxTriggers = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Dragoon") int32 TriggerCount = 0;
};
