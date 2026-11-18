#include "Abilities/RoundBuildShieldEmitterAbility.h"

#include "Actors/RoundCharacterEngineer.h"
#include "Actors/RoundShieldEmitterDeviceActor.h"

ARoundBuildShieldEmitterAbility::ARoundBuildShieldEmitterAbility()
{
    DeviceClass = ARoundShieldEmitterDeviceActor::StaticClass();
}

TSubclassOf<ARoundEngineerDeviceActor> ARoundBuildShieldEmitterAbility::ResolveDeviceClass() const
{
    const ARoundCharacterEngineer* Engineer = Cast<ARoundCharacterEngineer>(OwningCharacter);
    return Engineer ? Engineer->GetShieldEmitterDeviceClass() : Super::ResolveDeviceClass();
}

void ARoundBuildShieldEmitterAbility::ConfigureSpawnedDevice(ARoundEngineerDeviceActor* SpawnedDevice) const
{
    ARoundShieldEmitterDeviceActor* Emitter = Cast<ARoundShieldEmitterDeviceActor>(SpawnedDevice);
    if (!Emitter)
    {
        return;
    }

    const int32 ConfiguredShield = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ShieldAmount")));
    const int32 ConfiguredRadius = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AuraRange")));
    const int32 ConfiguredShieldDuration = GetAbilityDurationInitiativeFromKey(TEXT("ShieldDurationInitiative"));
    const int32 ConfiguredInterval = GetAbilityDurationInitiativeFromKey(TEXT("TriggerIntervalInitiative"));
    Emitter->ShieldAmount = ConfiguredShield > 0 ? ConfiguredShield : Emitter->ShieldAmount;
    Emitter->ShieldRadius = ConfiguredRadius > 0 ? ConfiguredRadius : Emitter->ShieldRadius;
    Emitter->ShieldDurationInitiative = ConfiguredShieldDuration > 0 ? ConfiguredShieldDuration : Emitter->ShieldDurationInitiative;
    Emitter->TriggerIntervalInitiative = ConfiguredInterval > 0 ? ConfiguredInterval : Emitter->TriggerIntervalInitiative;
    Emitter->HealAmount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HealAmount"))));
    Emitter->InitiativeGain = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGain"))));
    Emitter->bCleanseOneDebuff = GetAbilityValueFromKey(TEXT("CleanseOneDebuff")) > 0.0;
    Emitter->SoloShieldMultiplier = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("SoloShieldMultiplier")));
}
