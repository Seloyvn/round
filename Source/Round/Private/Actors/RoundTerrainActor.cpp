#include "Actors/RoundTerrainActor.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/RoundTerrainHelpers.h"
#include "Data/RoundTerrainData.h"
#include "Game/RoundGameState.h"
#include "Net/UnrealNetwork.h"

namespace
{
double FindTerrainValueCaseInsensitive(const TMap<FString, double>& Values, const FString& Key, bool& bOutFound)
{
    if (const double* ExactValue = Values.Find(Key))
    {
        bOutFound = true;
        return *ExactValue;
    }

    for (const TPair<FString, double>& Pair : Values)
    {
        if (Pair.Key.Equals(Key, ESearchCase::IgnoreCase))
        {
            bOutFound = true;
            return Pair.Value;
        }
    }

    bOutFound = false;
    return 0.0;
}
}

ARoundTerrainActor::ARoundTerrainActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
    MeshComponent->SetupAttachment(SceneRoot);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARoundTerrainActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundTerrainActor, Field);
    DOREPLIFETIME(ARoundTerrainActor, TerrainType);
    DOREPLIFETIME(ARoundTerrainActor, MaxHealth);
    DOREPLIFETIME(ARoundTerrainActor, CurrentHealth);
    DOREPLIFETIME(ARoundTerrainActor, RemainingInitiative);
    DOREPLIFETIME(ARoundTerrainActor, PeriodicTriggerProgressInitiative);
}

void ARoundTerrainActor::SetField(ARoundGridTile* InField)
{
    Field = InField;
    if (Field)
    {
        SetActorLocation(Field->GetActorLocation() + FVector(0.0f, 0.0f, -2.0f));
        SetActorScale3D(FVector(1.0f, 1.0f, 1.0f));
    }
}

void ARoundTerrainActor::SetTerrainData(URoundTerrainData* InTerrainData)
{
    TerrainData = InTerrainData;
    if (TerrainData)
    {
        TerrainType = TerrainData->TerrainType;
    }

    MaxHealth = GetTerrainMaxHealth();
    CurrentHealth = MaxHealth;
}

bool ARoundTerrainActor::IsMoveable() const
{
    if (TerrainData)
    {
        bool bFound = false;
        const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("Moveable"), bFound);
        if (bFound)
        {
            return Value > 0.0;
        }
    }
    return bTerrainMoveable;
}

bool ARoundTerrainActor::BlocksVision() const
{
    if (TerrainData)
    {
        bool bFound = false;
        const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("BlocksVision"), bFound);
        if (bFound)
        {
            return Value > 0.0;
        }
    }
    return false;
}

int32 ARoundTerrainActor::GetVisionCost() const
{
    if (TerrainData)
    {
        bool bFound = false;
        const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("VisionCost"), bFound);
        if (bFound) return FMath::Max(1, FMath::RoundToInt(Value));
    }
    return 1;
}

int32 ARoundTerrainActor::GetVisionRangeModifier() const
{
    if (TerrainData)
    {
        bool bFound = false;
        const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("VisionRangeModifier"), bFound);
        if (bFound) return FMath::RoundToInt(Value);
    }
    return 0;
}

bool ARoundTerrainActor::StopsMovement() const
{
    if (TerrainData)
    {
        bool bFound = false;
        const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("StopsMovement"), bFound);
        if (bFound) return Value > 0.0;
    }
    return false;
}

int32 ARoundTerrainActor::GetTerrainBaseMoveCost() const
{
    if (TerrainData)
    {
        bool bFound = false;
        const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("BaseMoveCost"), bFound);
        if (bFound)
        {
            return FMath::RoundToInt(Value);
        }

        const double AlternateValue = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("MoveCost"), bFound);
        if (bFound)
        {
            return FMath::RoundToInt(AlternateValue);
        }
    }

    return TerrainBaseMoveCost;
}

FText ARoundTerrainActor::GetTerrainDisplayNameText() const
{
    return TerrainData ? TerrainData->DisplayName : FText::GetEmpty();
}

FText ARoundTerrainActor::GetTerrainDescriptionText() const
{
    return TerrainData ? TerrainData->Description : FText::GetEmpty();
}

float ARoundTerrainActor::GetTerrainDefaultWeight() const
{
    return TerrainData ? TerrainData->DefaultWeight : 0.0f;
}

float ARoundTerrainActor::GetTerrainMoveCostMultiplier() const
{
    if (TerrainData)
    {
        bool bFound = false;
        const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("MoveCostMultiplier"), bFound);
        if (bFound)
        {
            return static_cast<float>(Value);
        }
    }

    return 1.0f;
}

float ARoundTerrainActor::GetIncomingRangedDamageMultiplier() const
{
    if (TerrainData)
    {
        bool bFound = false;
        const double GenericValue = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("IncomingDamageMultiplier"), bFound);
        if (bFound)
        {
            return static_cast<float>(GenericValue);
        }

        const double GenericAlternateValue = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("DamageMultiplier"), bFound);
        if (bFound)
        {
            return static_cast<float>(GenericAlternateValue);
        }

        const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("IncomingRangedDamageMultiplier"), bFound);
        if (bFound)
        {
            return static_cast<float>(Value);
        }

        const double AlternateValue = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("RangedDamageMultiplier"), bFound);
        if (bFound)
        {
            return static_cast<float>(AlternateValue);
        }
    }

    return 1.0f;
}

int32 ARoundTerrainActor::GetTurnStartDamage() const
{
    if (!TerrainData)
    {
        return 0;
    }

    bool bFound = false;
    const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("TurnStartDamage"), bFound);
    if (bFound)
    {
        return FMath::RoundToInt(Value);
    }

    const double EnterValue = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("OnEnterDamage"), bFound);
    if (bFound)
    {
        return FMath::RoundToInt(EnterValue);
    }

    const double DamageValue = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("Damage"), bFound);
    return bFound ? FMath::RoundToInt(DamageValue) : 0;
}

ERoundDamageType ARoundTerrainActor::GetTurnStartDamageType() const
{
    if (TerrainData)
    {
        bool bFound = false;
        const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("TurnStartDamageType"), bFound);
        if (bFound)
        {
            return static_cast<ERoundDamageType>(FMath::RoundToInt(Value));
        }
    }

    return ERoundDamageType::Fire;
}

int32 ARoundTerrainActor::GetOnEnterDamage() const
{
    if (!TerrainData)
    {
        return 0;
    }

    bool bFound = false;
    const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("OnEnterDamage"), bFound);
    if (bFound)
    {
        return FMath::RoundToInt(Value);
    }

    return 0;
}

ERoundDamageType ARoundTerrainActor::GetOnEnterDamageType() const
{
    if (TerrainData)
    {
        bool bFound = false;
        const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("OnEnterDamageType"), bFound);
        if (bFound)
        {
            return static_cast<ERoundDamageType>(FMath::RoundToInt(Value));
        }
    }

    return ERoundDamageType::Ground;
}

int32 ARoundTerrainActor::GetPeriodicDamage() const
{
    if (!TerrainData)
    {
        return 0;
    }

    bool bFound = false;
    const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("PeriodicDamage"), bFound);
    if (bFound)
    {
        return FMath::RoundToInt(Value);
    }

    const double AlternateValue = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("DamagePerInterval"), bFound);
    return bFound ? FMath::RoundToInt(AlternateValue) : 0;
}

ERoundDamageType ARoundTerrainActor::GetPeriodicDamageType() const
{
    if (TerrainData)
    {
        bool bFound = false;
        const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("PeriodicDamageType"), bFound);
        if (bFound)
        {
            return static_cast<ERoundDamageType>(FMath::RoundToInt(Value));
        }
    }

    return ERoundDamageType::Ground;
}

int32 ARoundTerrainActor::GetPeriodicDamageIntervalInitiative() const
{
    if (!TerrainData)
    {
        return 0;
    }

    bool bFound = false;
    const double Value = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("PeriodicDamageIntervalInitiative"), bFound);
    if (bFound)
    {
        return FMath::Max(0, FMath::RoundToInt(Value));
    }

    const double AlternateValue = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("TriggerIntervalInitiative"), bFound);
    return bFound ? FMath::Max(0, FMath::RoundToInt(AlternateValue)) : 0;
}

bool ARoundTerrainActor::IsDestructible() const
{
    return GetTerrainMaxHealth() > 0;
}

int32 ARoundTerrainActor::GetTerrainMaxHealth() const
{
    if (!TerrainData)
    {
        return 0;
    }

    bool bFound = false;
    const double MaxHealthValue = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("MaxHealth"), bFound);
    if (bFound)
    {
        return FMath::Max(0, FMath::RoundToInt(MaxHealthValue));
    }

    const double HealthValue = FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("Health"), bFound);
    if (bFound)
    {
        return FMath::Max(0, FMath::RoundToInt(HealthValue));
    }

    return 0;
}

void ARoundTerrainActor::SetRemainingInitiative(const int32 DurationInitiative)
{
    RemainingInitiative = FMath::Max(0, DurationInitiative);
}

void ARoundTerrainActor::AdvanceTerrainInitiative(const int32 InitiativeAmount)
{
    if (InitiativeAmount <= 0)
    {
        return;
    }

    const int32 PeriodicInterval = GetPeriodicDamageIntervalInitiative();
    const int32 PeriodicDamage = GetPeriodicDamage();
    if (PeriodicInterval > 0 && PeriodicDamage > 0 && Field && Field->OccupyingCharacter)
    {
        PeriodicTriggerProgressInitiative += InitiativeAmount;
        while (PeriodicTriggerProgressInitiative >= PeriodicInterval && Field && Field->OccupyingCharacter)
        {
            PeriodicTriggerProgressInitiative -= PeriodicInterval;
            bool bDidDie = false;
            Field->OccupyingCharacter->ApplyRoundDamage(PeriodicDamage, GetPeriodicDamageType(), 0, nullptr, bDidDie);
        }
    }

    if (RemainingInitiative <= 0)
    {
        return;
    }

    RemainingInitiative = FMath::Max(0, RemainingInitiative - InitiativeAmount);
    if (RemainingInitiative <= 0 && Field)
    {
        Field->SetTerrainType(ERoundTerrainType::Ground);
    }
}

int32 ARoundTerrainActor::GetMoveCost_Implementation(const int32 BaseCost) const
{
    if (!IsMoveable())
    {
        return MAX_int32;
    }

    const int32 EffectiveTerrainCost = FMath::Max(1, GetTerrainBaseMoveCost());
    if (BaseCost > 0)
    {
        return FMath::Max(EffectiveTerrainCost, FMath::RoundToInt(static_cast<double>(BaseCost) * GetTerrainMoveCostMultiplier()));
    }

    return EffectiveTerrainCost;
}

int32 ARoundTerrainActor::ModifyIncomingDamage_Implementation(ARoundCharacter* Defender, ARoundCharacter* SourceCharacter, const int32 Damage, ERoundDamageType DamageType) const
{
    return Damage;
}

void ARoundTerrainActor::OnCharacterTurnStarted_Implementation(ARoundCharacter* OccupyingCharacter)
{
    if (!OccupyingCharacter || !TerrainData) return;
    const int32 Damage = GetTurnStartDamage();
    if (Damage > 0)
    {
        bool bDidDie = false;
        OccupyingCharacter->ApplyRoundDamage(Damage, GetTurnStartDamageType(), 0, nullptr, bDidDie);
    }
    bool bFound = false;
    const int32 HealAmount = FMath::Max(0, FMath::RoundToInt(FindTerrainValueCaseInsensitive(TerrainData->Values, TEXT("TurnStartHeal"), bFound)));
    if (bFound && HealAmount > 0) OccupyingCharacter->Heal(HealAmount);
}

void ARoundTerrainActor::OnCharacterEntered_Implementation(ARoundCharacter* EnteringCharacter, ARoundGridTile* PreviousTile)
{
    if (!EnteringCharacter || !TerrainData) return;
    const int32 Damage = GetOnEnterDamage();
    if (Damage > 0)
    {
        bool bDidDie = false;
        EnteringCharacter->ApplyRoundDamage(Damage, GetOnEnterDamageType(), 0, nullptr, bDidDie);
    }
}

bool ARoundTerrainActor::CanReceiveBoardDamage_Implementation() const
{
    return IsDestructible() && CurrentHealth > 0;
}

FRoundBoardDamageResult ARoundTerrainActor::ApplyBoardDamage_Implementation(int32 Damage, ERoundDamageType DamageType, int32 DefenseIgnore, ARoundCharacter* SourceCharacter)
{
    FRoundBoardDamageResult Result;
    if (Field && DamageType == ERoundDamageType::Fire && RoundTerrain::IsNatureTerrain(TerrainType))
    {
        Field->SetTerrainType(ERoundTerrainType::Fire);
        return Result;
    }

    if (Damage <= 0 || !CanReceiveBoardDamage_Implementation())
    {
        return Result;
    }

    Result.AppliedDamage = FMath::Clamp(Damage, 0, CurrentHealth);
    CurrentHealth = FMath::Max(0, CurrentHealth - Result.AppliedDamage);
    Result.bDestroyed = CurrentHealth <= 0;

    if (Result.bDestroyed && Field)
    {
        Field->SetTerrainType(ERoundTerrainType::Ground);
    }

    return Result;
}
