#include "Actors/RoundEngineerDeviceActor.h"

#include "Actors/RoundCharacterEngineer.h"
#include "Actors/RoundGridTile.h"
#include "Net/UnrealNetwork.h"

void ARoundEngineerDeviceActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundEngineerDeviceActor, MaxHealth);
    DOREPLIFETIME(ARoundEngineerDeviceActor, CurrentHealth);
}

void ARoundEngineerDeviceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ARoundCharacterEngineer* EngineerOwner = Cast<ARoundCharacterEngineer>(OwningCharacter))
    {
        EngineerOwner->UnregisterDevice(this);
    }

    Super::EndPlay(EndPlayReason);
}

void ARoundEngineerDeviceActor::OnInitiativeAdvanced(int32 InitiativeAmount)
{
    if (bCoreAuthoritative) return;
    Super::OnInitiativeAdvanced(InitiativeAmount);
}

void ARoundEngineerDeviceActor::ApplyCoreDeviceState(const int32 InLifetimeInitiative,
    const int32 InMaximumHealth, const int32 InCurrentHealth, const int32 InTriggerIntervalInitiative,
    const int32 InTriggerProgressInitiative)
{
    bCoreAuthoritative = true;
    RemainingInitiative = FMath::Max(0, InLifetimeInitiative);
    MaxHealth = FMath::Max(1, InMaximumHealth);
    CurrentHealth = FMath::Clamp(InCurrentHealth, 0, MaxHealth);
    TriggerIntervalInitiative = FMath::Max(0, InTriggerIntervalInitiative);
    TriggerProgressInitiative = FMath::Max(0, InTriggerProgressInitiative);
    ForceNetUpdate();
}

bool ARoundEngineerDeviceActor::CanOccupyTile(const ARoundGridTile* Tile) const
{
    if (!Super::CanOccupyTile(Tile) || !Tile || !Tile->IsMoveable())
    {
        return false;
    }

    if (IRoundDeviceInterface::Execute_BlocksCharactersOnTile(const_cast<ARoundEngineerDeviceActor*>(this)))
    {
        return Tile->OccupyingCharacter == nullptr;
    }

    return true;
}

bool ARoundEngineerDeviceActor::CanTriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext) const
{
    return !bCoreAuthoritative && Super::CanTriggerDevice_Implementation(TriggerContext);
}

void ARoundEngineerDeviceActor::InitializeDevice(ARoundCharacterEngineer* InOwner, int32 InLifetimeInitiative, int32 InHealth)
{
    OwningCharacter = InOwner;
    const double LifetimeMultiplier = 1.0 + (InOwner ? FMath::Max(0.0, InOwner->GetPassiveValueFromKey(TEXT("DeviceLifetimeMultiplierAdd"))) : 0.0);
    const double HealthMultiplier = 1.0 + (InOwner ? FMath::Max(0.0, InOwner->GetPassiveValueFromKey(TEXT("DeviceHealthMultiplierAdd"))) : 0.0);
    SetLifetimeInitiative(FMath::RoundToInt(InLifetimeInitiative * LifetimeMultiplier));
    MaxHealth = FMath::Max(1, FMath::RoundToInt(InHealth * HealthMultiplier));
    CurrentHealth = MaxHealth;
    ForceNetUpdate();
}

int32 ARoundEngineerDeviceActor::RepairDevice(int32 Amount)
{
    const int32 Previous = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth + FMath::Max(0, Amount), 0, MaxHealth);
    if (CurrentHealth != Previous) ForceNetUpdate();
    return CurrentHealth - Previous;
}

int32 ARoundEngineerDeviceActor::GetCurrentHealth() const
{
    return CurrentHealth;
}

int32 ARoundEngineerDeviceActor::ApplyDeviceDamage(int32 DamageAmount)
{
    const int32 AppliedDamage = FMath::Clamp(DamageAmount, 0, CurrentHealth);
    CurrentHealth = FMath::Max(0, CurrentHealth - AppliedDamage);
    if (CurrentHealth <= 0)
    {
        DestroyDevice();
    }
    else
    {
        ForceNetUpdate();
    }

    return AppliedDamage;
}

void ARoundEngineerDeviceActor::DestroyDevice()
{
    if (bCanTriggerOnDestroyed && GetClass()->ImplementsInterface(URoundDeviceInterface::StaticClass()))
    {
        FRoundDeviceTriggerContext TriggerContext;
        TriggerContext.TriggerType = ERoundDeviceTriggerType::OnDestroyed;
        TriggerContext.TriggeringCharacter = OwningCharacter;
        TriggerContext.TriggerTile = CurrentTile;
        if (IRoundDeviceInterface::Execute_CanTriggerDevice(this, TriggerContext))
        {
            IRoundDeviceInterface::Execute_TriggerDevice(this, TriggerContext);
        }
    }

    RemoveFromField();
}

bool ARoundEngineerDeviceActor::CanReceiveBoardDamage_Implementation() const
{
    return CurrentHealth > 0;
}

FRoundBoardDamageResult ARoundEngineerDeviceActor::ApplyBoardDamage_Implementation(int32 Damage, ERoundDamageType DamageType, int32 DefenseIgnore, ARoundCharacter* SourceCharacter)
{
    FRoundBoardDamageResult Result;
    if (Damage <= 0)
    {
        return Result;
    }

    Result.AppliedDamage = ApplyDeviceDamage(Damage);
    Result.bDestroyed = CurrentHealth <= 0 || !IsValid(this);
    return Result;
}
