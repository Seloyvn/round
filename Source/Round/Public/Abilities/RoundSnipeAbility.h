#pragma once
#include "Actors/RoundAbilityActor.h"
#include "RoundSnipeAbility.generated.h"
UCLASS(Blueprintable)
class ROUND_API ARoundSnipeAbility : public ARoundAbilityActor
{
    GENERATED_BODY()

public:
    virtual FRoundActionResult ExecuteAbility(class ARoundGridTile* TargetTile) override;
    virtual void AdvanceCooldown() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability|Snipe")
    int32 ReadyInitiativeElapsed = 0;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
