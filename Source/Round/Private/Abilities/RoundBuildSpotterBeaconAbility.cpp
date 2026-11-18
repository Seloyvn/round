#include "Abilities/RoundBuildSpotterBeaconAbility.h"

#include "Actors/RoundCharacterEngineer.h"
#include "Actors/RoundSpotterBeaconDeviceActor.h"

ARoundBuildSpotterBeaconAbility::ARoundBuildSpotterBeaconAbility()
{
    DeviceClass = ARoundSpotterBeaconDeviceActor::StaticClass();
}

TSubclassOf<ARoundEngineerDeviceActor> ARoundBuildSpotterBeaconAbility::ResolveDeviceClass() const
{
    const ARoundCharacterEngineer* Engineer = Cast<ARoundCharacterEngineer>(OwningCharacter);
    return Engineer ? Engineer->GetSpotterBeaconDeviceClass() : Super::ResolveDeviceClass();
}

void ARoundBuildSpotterBeaconAbility::ConfigureSpawnedDevice(ARoundEngineerDeviceActor* SpawnedDevice) const
{
    ARoundSpotterBeaconDeviceActor* Beacon = Cast<ARoundSpotterBeaconDeviceActor>(SpawnedDevice);
    if (!Beacon)
    {
        return;
    }

    const int32 ConfiguredRadius = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AuraRange")));
    const int32 ConfiguredDebuffDuration = GetAbilityDurationInitiativeFromKey(TEXT("DebuffDurationInitiative"));
    const double ConfiguredMultiplier = GetAbilityValueFromKey(TEXT("DamageTakenMultiplier"));
    const int32 ConfiguredInterval = GetAbilityDurationInitiativeFromKey(TEXT("TriggerIntervalInitiative"));
    Beacon->BeaconRadius = ConfiguredRadius > 0 ? ConfiguredRadius : Beacon->BeaconRadius;
    Beacon->DebuffDurationInitiative = ConfiguredDebuffDuration > 0 ? ConfiguredDebuffDuration : Beacon->DebuffDurationInitiative;
    Beacon->DamageTakenMultiplier = ConfiguredMultiplier > 0.0 ? ConfiguredMultiplier : Beacon->DamageTakenMultiplier;
    Beacon->TriggerIntervalInitiative = ConfiguredInterval > 0 ? ConfiguredInterval : Beacon->TriggerIntervalInitiative;
    Beacon->InitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLoss"))));
    Beacon->MoveCostAdd = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MoveCostAdd"))));
    Beacon->OutgoingDamageMultiplier = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("OutgoingDamageMultiplier")));
}
