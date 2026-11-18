#include "Abilities/RoundBuildMineAbility.h"

#include "Actors/RoundCharacterEngineer.h"
#include "Actors/RoundMineDeviceActor.h"

ARoundBuildMineAbility::ARoundBuildMineAbility()
{
    DeviceClass = ARoundMineDeviceActor::StaticClass();
}

TSubclassOf<ARoundEngineerDeviceActor> ARoundBuildMineAbility::ResolveDeviceClass() const
{
    const ARoundCharacterEngineer* Engineer = Cast<ARoundCharacterEngineer>(OwningCharacter);
    return Engineer ? Engineer->GetMineDeviceClass() : Super::ResolveDeviceClass();
}

void ARoundBuildMineAbility::ConfigureSpawnedDevice(ARoundEngineerDeviceActor* SpawnedDevice) const
{
    ARoundMineDeviceActor* Mine = Cast<ARoundMineDeviceActor>(SpawnedDevice);
    if (!Mine)
    {
        return;
    }

    const int32 ConfiguredRadius = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AOERange")));
    Mine->ExplosionDamage = FMath::Max(1, GetAbilityDamage());
    Mine->ExplosionRadius = ConfiguredRadius > 0 ? ConfiguredRadius : Mine->ExplosionRadius;
    Mine->ExplosionDamageType = GetAbilityDamageType();
    Mine->SetArmDelay(GetAbilityDurationInitiativeFromKey(TEXT("ArmDelayInitiative")));
    Mine->DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    Mine->InitiativeLossOnHit = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeLossOnHit"))));
    Mine->RootDurationInitiative = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RootDurationInitiative"))));
    Mine->bPreserveOnManualTrigger = GetAbilityValueFromKey(TEXT("PreserveOnManualTrigger")) > 0.0;
}
