#include "Components/RoundAttributeSetComponent.h"
#include "Net/UnrealNetwork.h"

URoundAttributeSetComponent::URoundAttributeSetComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void URoundAttributeSetComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(URoundAttributeSetComponent, BaseStats);
    DOREPLIFETIME(URoundAttributeSetComponent, CurrentHealth);
    DOREPLIFETIME(URoundAttributeSetComponent, CurrentInitiative);
}

void URoundAttributeSetComponent::InitializeFromStats(const FRoundStatBlock& InStats)
{
    BaseStats = InStats;
    CurrentHealth = BaseStats.MaxHealth;
    CurrentInitiative = 0;
    OnHealthChanged.Broadcast(CurrentHealth, 0);
    OnInitiativeChanged.Broadcast(CurrentInitiative);
}

int32 URoundAttributeSetComponent::ApplyDamage(int32 RawDamage)
{
    const int32 MitigatedDamage = FMath::Max(0, RawDamage - BaseStats.Defense);
    const int32 PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth - MitigatedDamage, 0, BaseStats.MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, CurrentHealth - PreviousHealth);
    return MitigatedDamage;
}

int32 URoundAttributeSetComponent::ApplyFinalDamage(int32 FinalDamage)
{
    const int32 DamageToApply = FMath::Max(0, FinalDamage);
    const int32 PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth - DamageToApply, 0, BaseStats.MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, CurrentHealth - PreviousHealth);
    return DamageToApply;
}

void URoundAttributeSetComponent::Heal(int32 Amount)
{
    const int32 PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth + FMath::Max(0, Amount), 0, BaseStats.MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, CurrentHealth - PreviousHealth);
}

bool URoundAttributeSetComponent::SpendInitiative(int32 Amount)
{
    if (CurrentInitiative < Amount)
    {
        return false;
    }

    CurrentInitiative -= FMath::Max(0, Amount);
    OnInitiativeChanged.Broadcast(CurrentInitiative);
    return true;
}

void URoundAttributeSetComponent::GainInitiative(int32 Amount)
{
    CurrentInitiative = FMath::Max(0, CurrentInitiative + FMath::Max(0, Amount));
    OnInitiativeChanged.Broadcast(CurrentInitiative);
}

void URoundAttributeSetComponent::PassInitiative(int32 Amount)
{
    CurrentInitiative = FMath::Max(0, CurrentInitiative - FMath::Max(0, Amount));
    OnInitiativeChanged.Broadcast(CurrentInitiative);
}

void URoundAttributeSetComponent::SetCurrentHealthValue(const int32 NewHealth)
{
    const int32 PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(NewHealth, 0, BaseStats.MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, CurrentHealth - PreviousHealth);
}

void URoundAttributeSetComponent::SetCurrentInitiativeValue(const int32 NewInitiative)
{
    CurrentInitiative = FMath::Max(0, NewInitiative);
    OnInitiativeChanged.Broadcast(CurrentInitiative);
}

bool URoundAttributeSetComponent::IsAlive() const
{
    return CurrentHealth > 0;
}

void URoundAttributeSetComponent::OnRep_BaseStats()
{
    OnHealthChanged.Broadcast(CurrentHealth, 0);
    OnInitiativeChanged.Broadcast(CurrentInitiative);
}

void URoundAttributeSetComponent::OnRep_CurrentHealth()
{
    OnHealthChanged.Broadcast(CurrentHealth, 0);
}

void URoundAttributeSetComponent::OnRep_CurrentInitiative()
{
    OnInitiativeChanged.Broadcast(CurrentInitiative);
}
