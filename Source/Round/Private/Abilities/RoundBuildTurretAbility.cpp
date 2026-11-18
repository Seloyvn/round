#include "Abilities/RoundBuildTurretAbility.h"

#include "Actors/RoundCharacterEngineer.h"
#include "Actors/RoundTurretDeviceActor.h"

ARoundBuildTurretAbility::ARoundBuildTurretAbility()
{
    DeviceClass = ARoundTurretDeviceActor::StaticClass();
}

TSubclassOf<ARoundEngineerDeviceActor> ARoundBuildTurretAbility::ResolveDeviceClass() const
{
    const ARoundCharacterEngineer* Engineer = Cast<ARoundCharacterEngineer>(OwningCharacter);
    return Engineer ? Engineer->GetTurretDeviceClass() : Super::ResolveDeviceClass();
}

void ARoundBuildTurretAbility::ConfigureSpawnedDevice(ARoundEngineerDeviceActor* SpawnedDevice) const
{
    ARoundTurretDeviceActor* Turret = Cast<ARoundTurretDeviceActor>(SpawnedDevice);
    if (!Turret)
    {
        return;
    }

    const int32 ConfiguredRange = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("TurretRange")));
    const int32 ConfiguredInterval = GetAbilityDurationInitiativeFromKey(TEXT("TriggerIntervalInitiative"));
    Turret->TurretRange = ConfiguredRange > 0 ? ConfiguredRange : Turret->TurretRange;
    Turret->TurretDamage = FMath::Max(1, GetAbilityDamage());
    Turret->TriggerIntervalInitiative = ConfiguredInterval > 0 ? ConfiguredInterval : Turret->TriggerIntervalInitiative;
    Turret->TurretDamageType = GetAbilityDamageType();
    Turret->DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    Turret->InitiativeLossOnHit = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit"))));
    Turret->SecondaryTargetDamageMultiplier = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("SecondaryTargetDamageMultiplier")));
    Turret->LowHealthThreshold = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("LowHealthThreshold")));
    Turret->LowHealthDamageMultiplier = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("LowHealthDamageMultiplier")));
}
