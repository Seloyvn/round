#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundEffectActor.h"
#include "RoundTemporalAnchorEffect.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundTemporalAnchorEffect : public ARoundEffectActor
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual bool IsBuff() const override { return true; }
    virtual void OnApplied_Implementation(class ARoundCharacter* TargetCharacter) override;
    virtual void OnInitiativeAdvanced(class ARoundCharacter* TargetCharacter, int32 InitiativeAmount) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Chronomancer") int32 HealOnReturn = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Chronomancer") int32 InitiativeGainOnReturn = 0;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Chronomancer") int32 NearbyEnemyInitiativeLoss = 0;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Chronomancer")
    TObjectPtr<class ARoundGridTile> AnchoredTile = nullptr;
};
