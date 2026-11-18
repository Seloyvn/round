#include "Abilities/RoundBuildDeviceAbility.h"

#include "Actors/RoundCharacterEngineer.h"
#include "Actors/RoundEngineerDeviceActor.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"

bool ARoundBuildDeviceAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    if (!Super::CanTargetTile(TargetTile) || !TargetTile)
    {
        return false;
    }

    return TargetTile->IsMoveable() && !TargetTile->OccupyingFieldObject;
}

int32 ARoundBuildDeviceAbility::GetAbilityRange() const
{
    const ARoundCharacterEngineer* Engineer = Cast<ARoundCharacterEngineer>(OwningCharacter);
    return Super::GetAbilityRange() + (Engineer ? FMath::Max(0, FMath::RoundToInt(Engineer->GetPassiveValueFromKey(TEXT("DevicePlacementRangeAdd")))) : 0);
}

FRoundActionResult ARoundBuildDeviceAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterEngineer* Engineer = Cast<ARoundCharacterEngineer>(OwningCharacter);
    if (!Engineer || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    TSubclassOf<ARoundEngineerDeviceActor> DeviceClassToSpawn = ResolveDeviceClass();
    if (!DeviceClassToSpawn)
    {
        Result.Reason = TEXT("No device class configured.");
        return Result;
    }

    ARoundEngineerDeviceActor* SpawnedDevice = Engineer->SpawnDeviceAtTile(DeviceClassToSpawn, TargetTile);
    if (!SpawnedDevice)
    {
        Result.Reason = TEXT("Failed to build device.");
        return Result;
    }

    SpawnedDevice->InitializeDevice(
        Engineer,
        GetAbilityDurationInitiativeFromKey(TEXT("DurationInitiative")),
        FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("Health")))));
    ConfigureSpawnedDevice(SpawnedDevice);
    const double IntervalReduction = FMath::Clamp(Engineer->GetPassiveValueFromKey(TEXT("DeviceTriggerIntervalReduction")), 0.0, 0.8);
    if (IntervalReduction > 0.0 && SpawnedDevice->TriggerIntervalInitiative > 0)
    {
        SpawnedDevice->TriggerIntervalInitiative = FMath::Max(1, FMath::RoundToInt(SpawnedDevice->TriggerIntervalInitiative * (1.0 - IntervalReduction)));
    }

    if (ARoundGameState* GameState = Engineer->GetRoundGameState())
    {
        GameState->OnBoardStateChanged.Broadcast();
    }

    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Built %s."), *GetNameSafe(SpawnedDevice));
    return Result;
}

TSubclassOf<ARoundEngineerDeviceActor> ARoundBuildDeviceAbility::ResolveDeviceClass() const
{
    return DeviceClass;
}

void ARoundBuildDeviceAbility::ConfigureSpawnedDevice(ARoundEngineerDeviceActor* SpawnedDevice) const
{
}
