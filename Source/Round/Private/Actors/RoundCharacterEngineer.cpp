#include "Actors/RoundCharacterEngineer.h"

#include "Actors/RoundEngineerDeviceActor.h"
#include "Actors/RoundGridTile.h"
#include "Actors/RoundMineDeviceActor.h"
#include "Actors/RoundShieldEmitterDeviceActor.h"
#include "Actors/RoundSpotterBeaconDeviceActor.h"
#include "Actors/RoundTurretDeviceActor.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Net/UnrealNetwork.h"

namespace
{
template <typename TDevice>
TSubclassOf<TDevice> ResolveConfiguredDeviceClass(const TSubclassOf<TDevice>& ConfiguredClass)
{
    if (ConfiguredClass)
    {
        return ConfiguredClass;
    }

    return TSubclassOf<TDevice>(TDevice::StaticClass());
}
}

TSubclassOf<ARoundTurretDeviceActor> ARoundCharacterEngineer::GetTurretDeviceClass() const
{
    return ResolveConfiguredDeviceClass(TurretDeviceClass);
}

TSubclassOf<ARoundMineDeviceActor> ARoundCharacterEngineer::GetMineDeviceClass() const
{
    return ResolveConfiguredDeviceClass(MineDeviceClass);
}

TSubclassOf<ARoundShieldEmitterDeviceActor> ARoundCharacterEngineer::GetShieldEmitterDeviceClass() const
{
    return ResolveConfiguredDeviceClass(ShieldEmitterDeviceClass);
}

TSubclassOf<ARoundSpotterBeaconDeviceActor> ARoundCharacterEngineer::GetSpotterBeaconDeviceClass() const
{
    return ResolveConfiguredDeviceClass(SpotterBeaconDeviceClass);
}

void ARoundCharacterEngineer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundCharacterEngineer, ActiveDevices);
}

void ARoundCharacterEngineer::Die()
{
    RemoveAllDevices();
    Super::Die();
}

void ARoundCharacterEngineer::BeginTurn()
{
    Super::BeginTurn();
    const int32 RepairAmount = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("DeviceRepairOnTurnStart"))));
    if (RepairAmount > 0)
    {
        for (ARoundEngineerDeviceActor* Device : GetActiveDevices())
        {
            if (Device) Device->RepairDevice(RepairAmount);
        }
    }
}

void ARoundCharacterEngineer::AdvanceGlobalInitiative(int32 Amount)
{
    Super::AdvanceGlobalInitiative(Amount);

    if (Amount <= 0)
    {
        return;
    }

    const TArray<TObjectPtr<ARoundEngineerDeviceActor>> DevicesSnapshot = ActiveDevices;
    for (ARoundEngineerDeviceActor* DeviceActor : DevicesSnapshot)
    {
        if (DeviceActor)
        {
            DeviceActor->OnInitiativeAdvanced(Amount);
        }
    }

    CleanupInvalidDevices();
}

int32 ARoundCharacterEngineer::GetMaxDevices() const
{
    const int32 ConfiguredLimit = FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MaxDevices")));
    return FMath::Max(1, (ConfiguredLimit > 0 ? ConfiguredLimit : 3) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MaxDevicesAdd"))));
}

int32 ARoundCharacterEngineer::GetIncomingBonusDefense(ERoundDamageType DamageType) const
{
    int32 Bonus = Super::GetIncomingBonusDefense(DamageType);
    const int32 PerDevice = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("DefensePerActiveDevice"))));
    const int32 Maximum = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MaxDefenseFromDevices"))));
    if (PerDevice > 0) Bonus += Maximum > 0 ? FMath::Min(Maximum, GetActiveDeviceCount() * PerDevice) : GetActiveDeviceCount() * PerDevice;

    const int32 NetworkRange = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("DeviceNetworkDefenseRange"))));
    if (NetworkRange > 0 && CurrentTile)
    {
        const bool bNearDevice = GetActiveDevices().ContainsByPredicate([this, NetworkRange](const ARoundEngineerDeviceActor* Device)
        {
            return Device && Device->CurrentTile && CurrentTile->GetDistance(Device->CurrentTile) <= NetworkRange;
        });
        if (bNearDevice) Bonus += FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("DefenseNearDevice"))));
    }
    return Bonus;
}

int32 ARoundCharacterEngineer::GetActiveDeviceCount() const
{
    const_cast<ARoundCharacterEngineer*>(this)->CleanupInvalidDevices();
    return ActiveDevices.Num();
}

const TArray<TObjectPtr<ARoundEngineerDeviceActor>>& ARoundCharacterEngineer::GetActiveDevices() const
{
    const_cast<ARoundCharacterEngineer*>(this)->CleanupInvalidDevices();
    return ActiveDevices;
}

ARoundEngineerDeviceActor* ARoundCharacterEngineer::SpawnDeviceAtTile(TSubclassOf<ARoundEngineerDeviceActor> DeviceClass, ARoundGridTile* Tile)
{
    CleanupInvalidDevices();
    if (!HasAuthority() || !GetWorld() || !Tile || !DeviceClass)
    {
        return nullptr;
    }

    while (ActiveDevices.Num() >= GetMaxDevices())
    {
        ARoundEngineerDeviceActor* OldestDevice = ActiveDevices[0];
        ActiveDevices.RemoveAt(0);
        if (OldestDevice)
        {
            OldestDevice->RemoveFromField();
            if (Attributes)
            {
                Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InitiativeRefundOnDeviceReplace")))));
            }
        }
        CleanupInvalidDevices();
    }

    ARoundEngineerDeviceActor* SpawnedDevice = GetWorld()->SpawnActor<ARoundEngineerDeviceActor>(DeviceClass);
    if (!SpawnedDevice)
    {
        return nullptr;
    }

    SpawnedDevice->OwningCharacter = this;
    if (!SpawnedDevice->SetField(Tile))
    {
        SpawnedDevice->Destroy();
        return nullptr;
    }

    RegisterDevice(SpawnedDevice);
    NotifyRoundStateChanged();
    return SpawnedDevice;
}

void ARoundCharacterEngineer::RegisterDevice(ARoundEngineerDeviceActor* DeviceActor)
{
    if (!DeviceActor)
    {
        return;
    }

    CleanupInvalidDevices();
    ActiveDevices.AddUnique(DeviceActor);
    ForceNetUpdate();
}

void ARoundCharacterEngineer::UnregisterDevice(ARoundEngineerDeviceActor* DeviceActor)
{
    if (!DeviceActor)
    {
        return;
    }

    ActiveDevices.Remove(DeviceActor);
    ForceNetUpdate();
}

void ARoundCharacterEngineer::CleanupInvalidDevices()
{
    ActiveDevices.RemoveAll([](const ARoundEngineerDeviceActor* DeviceActor)
    {
        return !IsValid(DeviceActor);
    });
}

void ARoundCharacterEngineer::RemoveAllDevices()
{
    CleanupInvalidDevices();
    const TArray<TObjectPtr<ARoundEngineerDeviceActor>> DevicesSnapshot = ActiveDevices;
    ActiveDevices.Reset();
    for (ARoundEngineerDeviceActor* DeviceActor : DevicesSnapshot)
    {
        if (DeviceActor)
        {
            DeviceActor->RemoveFromField();
        }
    }
}
