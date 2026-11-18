#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundFieldObjectActor.h"
#include "Interfaces/RoundDeviceInterface.h"
#include "RoundDeviceFieldObjectActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundDeviceFieldObjectActor : public ARoundFieldObjectActor, public IRoundDeviceInterface
{
    GENERATED_BODY()

public:
    virtual void OnInitiativeAdvanced(int32 InitiativeAmount) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Device")
    bool bCanTriggerManually = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Device")
    bool bCanTriggerOnEnter = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Device")
    bool bCanTriggerOnLeave = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Device")
    bool bCanTriggerOnProximity = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Device")
    bool bCanTriggerOnDestroyed = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Device")
    int32 TriggerIntervalInitiative = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Device")
    int32 TriggerProgressInitiative = 0;

    virtual bool CanTriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext) const override;
    virtual FRoundActionResult TriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext) override;
    virtual class ARoundCharacter* GetDeviceOwnerCharacter_Implementation() const override;
    virtual class ARoundGridTile* GetDeviceTile_Implementation() const override;
    virtual bool BlocksCharactersOnTile_Implementation() const override;
    virtual TArray<class ARoundGridTile*> GetDeviceVisionFields_Implementation() const override;
    virtual bool IsDeviceVisibleToTeam_Implementation(ERoundTeam ViewingTeam, bool bTileInVision) const override;
};
