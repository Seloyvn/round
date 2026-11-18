#include "Actors/RoundDeviceFieldObjectActor.h"

void ARoundDeviceFieldObjectActor::OnInitiativeAdvanced(int32 InitiativeAmount)
{
    Super::OnInitiativeAdvanced(InitiativeAmount);

    if (!HasAuthority() || IsActorBeingDestroyed() || InitiativeAmount <= 0 || TriggerIntervalInitiative <= 0 || !CurrentTile)
    {
        return;
    }

    TriggerProgressInitiative += InitiativeAmount;
    while (TriggerProgressInitiative >= TriggerIntervalInitiative)
    {
        TriggerProgressInitiative -= TriggerIntervalInitiative;

        FRoundDeviceTriggerContext TriggerContext;
        TriggerContext.TriggerType = ERoundDeviceTriggerType::Timed;
        TriggerContext.TriggerTile = CurrentTile;
        TriggerContext.TriggeringCharacter = OwningCharacter;
        TriggerContext.InitiativeAmount = TriggerIntervalInitiative;

        if (IRoundDeviceInterface::Execute_CanTriggerDevice(this, TriggerContext))
        {
            IRoundDeviceInterface::Execute_TriggerDevice(this, TriggerContext);
        }
    }
}

bool ARoundDeviceFieldObjectActor::CanTriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext) const
{
    switch (TriggerContext.TriggerType)
    {
    case ERoundDeviceTriggerType::Manual:
        return bCanTriggerManually;
    case ERoundDeviceTriggerType::Timed:
        return TriggerIntervalInitiative > 0;
    case ERoundDeviceTriggerType::OnEnter:
        return bCanTriggerOnEnter;
    case ERoundDeviceTriggerType::OnLeave:
        return bCanTriggerOnLeave;
    case ERoundDeviceTriggerType::Proximity:
        return bCanTriggerOnProximity;
    case ERoundDeviceTriggerType::OnDestroyed:
        return bCanTriggerOnDestroyed;
    default:
        return false;
    }
}

FRoundActionResult ARoundDeviceFieldObjectActor::TriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext)
{
    FRoundActionResult Result;
    Result.bSuccess = false;
    Result.Reason = TEXT("Device has no trigger implementation.");
    return Result;
}

ARoundCharacter* ARoundDeviceFieldObjectActor::GetDeviceOwnerCharacter_Implementation() const
{
    return OwningCharacter;
}

ARoundGridTile* ARoundDeviceFieldObjectActor::GetDeviceTile_Implementation() const
{
    return CurrentTile;
}

bool ARoundDeviceFieldObjectActor::BlocksCharactersOnTile_Implementation() const
{
    return false;
}

TArray<ARoundGridTile*> ARoundDeviceFieldObjectActor::GetDeviceVisionFields_Implementation() const
{
    TArray<ARoundGridTile*> VisionFields;
    if (CurrentTile)
    {
        VisionFields.Add(CurrentTile);
    }

    return VisionFields;
}

bool ARoundDeviceFieldObjectActor::IsDeviceVisibleToTeam_Implementation(ERoundTeam ViewingTeam, bool bTileInVision) const
{
    if (ViewingTeam == ERoundTeam::Neutral)
    {
        return true;
    }

    return (OwningCharacter && OwningCharacter->Team == ViewingTeam) || bTileInVision;
}
