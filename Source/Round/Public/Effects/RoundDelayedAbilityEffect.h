#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEffectActor.h"
#include "RoundDelayedAbilityEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundDelayedAbilityEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delayed Ability")
    TObjectPtr<class ARoundAbilityActor> DelayedAbility = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delayed Ability")
    FVector2D TargetTilePosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delayed Ability")
    bool bHasTargetTile = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Delayed Ability")
    bool bHasTriggered = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delayed Ability")
    bool bAllowTurnsWhileDelayed = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delayed Ability")
    TObjectPtr<class ARoundCharacter> TrackedTargetCharacter = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delayed Ability")
    bool bPreferTrackedTargetCharacter = false;

    virtual bool PreventsAbilityUse(const class ARoundAbilityActor* Ability) const override;
    virtual int32 GetTurnBlockRemainingInitiative() const override;
    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount) override;
};
