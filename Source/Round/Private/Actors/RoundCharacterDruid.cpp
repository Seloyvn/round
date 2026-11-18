#include "Actors/RoundCharacterDruid.h"

#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundGridTile.h"
#include "Abilities/RoundCancelBearFormAbility.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundAbilityData.h"
#include "Data/RoundTerrainHelpers.h"
#include "Game/RoundGameState.h"
#include "Net/UnrealNetwork.h"

namespace
{
URoundAbilityData* BuildBearCancelAbilityData(UObject* Outer)
{
    URoundAbilityData* AbilityData = NewObject<URoundAbilityData>(Outer);
    AbilityData->AbilityId = TEXT("CancelBearForm");
    AbilityData->DisplayName = FText::FromString(TEXT("Cancel Bear Form"));
    AbilityData->Description = FText::FromString(TEXT("Verwandelt den Druiden zurueck in seine normale Form."));
    AbilityData->InitiativeCost = 0;
    AbilityData->Cooldown = 0;
    AbilityData->Range = 0;
    AbilityData->FieldState = ERoundFieldState::Self;
    AbilityData->bRequiresTargetCharacter = false;
    AbilityData->bRequiresTargetTile = false;
    AbilityData->AbilityClass = ARoundCancelBearFormAbility::StaticClass();
    return AbilityData;
}
}

void ARoundCharacterDruid::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundCharacterDruid, bBearFormActive);
    DOREPLIFETIME(ARoundCharacterDruid, StoredDruidHealth);
}

void ARoundCharacterDruid::BeginTurn()
{
    Super::BeginTurn();
    const bool bOnNature = CurrentTile && RoundTerrain::IsNatureTerrain(CurrentTile->TerrainType);
    if (bOnNature)
    {
        Heal(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("HealOnNatureTurnStart")))));
        if (Attributes) Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InitiativeOnNatureTurnStart")))));
    }
    if (bBearFormActive && ActiveBearSourceAbility)
    {
        Heal(FMath::Max(0, FMath::RoundToInt(ActiveBearSourceAbility->GetAbilityValueFromKey(TEXT("BearHealOnTurnStart")))));
    }
}

int32 ARoundCharacterDruid::GetMoveCost() const
{
    int32 Cost = Super::GetMoveCost();
    if (CurrentTile && RoundTerrain::IsNatureTerrain(CurrentTile->TerrainType))
    {
        Cost -= FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MoveCostReductionOnNature"))));
    }
    return FMath::Max(1, Cost);
}

double ARoundCharacterDruid::GetIncomingDamageMultiplier(ERoundDamageType DamageType) const
{
    double Multiplier = Super::GetIncomingDamageMultiplier(DamageType);
    if (CurrentTile && RoundTerrain::IsNatureTerrain(CurrentTile->TerrainType))
    {
        const double NatureMultiplierAdd = GetPassiveValueFromKey(TEXT("IncomingDamageMultiplierAddOnNature"));
        Multiplier *= FMath::Max(0.0, 1.0 + NatureMultiplierAdd);
    }
    return Multiplier;
}

void ARoundCharacterDruid::HandleDamageDealt(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType)
{
    Super::HandleDamageDealt(EnemyCharacter, AppliedDamage, DamageType);
    if (!bBearFormActive || !ActiveBearSourceAbility || AppliedDamage <= 0) return;
    const double Lifesteal = FMath::Max(0.0, ActiveBearSourceAbility->GetAbilityValueFromKey(TEXT("BearLifestealFraction")));
    if (Lifesteal > 0.0) Heal(FMath::RoundToInt(AppliedDamage * Lifesteal));
}

bool ARoundCharacterDruid::IsBearFormActive() const
{
    return bBearFormActive;
}

TArray<ARoundGridTile*> ARoundCharacterDruid::GetConnectedNatureFields() const
{
    TArray<ARoundGridTile*> Result;
    const ARoundGameState* RoundGameState = GetRoundGameState();
    if (!RoundGameState || !CurrentTile)
    {
        return Result;
    }

    TArray<ARoundGridTile*> Seeds;
    if (RoundTerrain::IsNatureTerrain(CurrentTile->TerrainType))
    {
        Seeds.Add(CurrentTile);
    }

    for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(CurrentTile))
    {
        if (Neighbor && RoundTerrain::IsNatureTerrain(Neighbor->TerrainType))
        {
            Seeds.AddUnique(Neighbor);
        }
    }

    TArray<ARoundGridTile*> OpenSet = Seeds;
    while (OpenSet.Num() > 0)
    {
        ARoundGridTile* Field = OpenSet.Pop(false);
        if (!Field || Result.Contains(Field) || !RoundTerrain::IsNatureTerrain(Field->TerrainType))
        {
            continue;
        }

        Result.Add(Field);
        for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(Field))
        {
            if (Neighbor && !Result.Contains(Neighbor) && RoundTerrain::IsNatureTerrain(Neighbor->TerrainType))
            {
                OpenSet.AddUnique(Neighbor);
            }
        }
    }

    return Result;
}

bool ARoundCharacterDruid::EnterBearForm(ARoundAbilityActor* SourceAbility)
{
    if (!HasAuthority() || bBearFormActive || !Attributes)
    {
        return false;
    }

    StoredDruidStats = Attributes->BaseStats;
    StoredDruidHealth = Attributes->CurrentHealth;
    StoredDruidAbilities = GrantedAbilities;
    ActiveBearSourceAbility = SourceAbility;
    bBearFormActive = true;

    ApplyBearStats(SourceAbility);
    if (Attributes && SourceAbility)
    {
        Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(SourceAbility->GetAbilityValueFromKey(TEXT("InitiativeGainOnEnter")))));
    }
    RebuildGrantedAbilitiesForBearForm();
    NotifyRoundStateChanged();
    ForceNetUpdate();
    return true;
}

bool ARoundCharacterDruid::ExitBearForm()
{
    if (!HasAuthority() || !bBearFormActive || !Attributes)
    {
        return false;
    }

    const int32 PreservedInitiative = Attributes->CurrentInitiative;
    Attributes->BaseStats = StoredDruidStats;
    const int32 ExitHeal = ActiveBearSourceAbility ? FMath::Max(0, FMath::RoundToInt(ActiveBearSourceAbility->GetAbilityValueFromKey(TEXT("DruidHealOnExit")))) : 0;
    Attributes->SetCurrentHealthValue(FMath::Clamp(StoredDruidHealth + ExitHeal, 1, StoredDruidStats.MaxHealth));
    Attributes->SetCurrentInitiativeValue(FMath::Clamp(PreservedInitiative, 0, StoredDruidStats.MaxInitiative));

    bBearFormActive = false;
    RestoreGrantedAbilitiesAfterBearForm();
    ActiveBearSourceAbility = nullptr;
    NotifyRoundStateChanged();
    ForceNetUpdate();
    return true;
}

bool ARoundCharacterDruid::TryPreventDeathFromPassive(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType)
{
    if (!bBearFormActive || !Attributes)
    {
        return false;
    }

    return ExitBearForm();
}

void ARoundCharacterDruid::ApplyBearStats(ARoundAbilityActor* SourceAbility)
{
    if (!Attributes)
    {
        return;
    }

    const int32 PreservedInitiative = Attributes->CurrentInitiative;
    FRoundStatBlock BearStats = StoredDruidStats;
    const auto ResolveStat = [SourceAbility](const FString& AbsoluteKey, const FString& AddKey, const int32 FallbackValue) -> int32
    {
        if (!SourceAbility)
        {
            return FallbackValue;
        }

        const int32 AbsoluteValue = FMath::RoundToInt(SourceAbility->GetAbilityValueFromKey(AbsoluteKey));
        const int32 AddValue = FMath::RoundToInt(SourceAbility->GetAbilityValueFromKey(AddKey));
        return AbsoluteValue > 0 ? AbsoluteValue : FMath::Max(0, FallbackValue + AddValue);
    };

    BearStats.MaxHealth = FMath::Max(1, ResolveStat(TEXT("BearMaxHealth"), TEXT("BearMaxHealthAdd"), BearStats.MaxHealth));
    BearStats.Attack = FMath::Max(0, ResolveStat(TEXT("BearAttack"), TEXT("BearAttackAdd"), BearStats.Attack));
    BearStats.Defense = FMath::Max(0, ResolveStat(TEXT("BearDefense"), TEXT("BearDefenseAdd"), BearStats.Defense));
    BearStats.MaxInitiative = FMath::Max(1, ResolveStat(TEXT("BearMaxInitiative"), TEXT("BearMaxInitiativeAdd"), BearStats.MaxInitiative));
    BearStats.VisionRange = FMath::Max(1, ResolveStat(TEXT("BearVisionRange"), TEXT("BearVisionRangeAdd"), BearStats.VisionRange));
    BearStats.AttackRange = FMath::Max(1, ResolveStat(TEXT("BearAttackRange"), TEXT("BearAttackRangeAdd"), BearStats.AttackRange));
    BearStats.MoveCost = FMath::Max(1, ResolveStat(TEXT("BearMoveCost"), TEXT("BearMoveCostAdd"), BearStats.MoveCost));
    BearStats.AttackCost = FMath::Max(1, ResolveStat(TEXT("BearAttackCost"), TEXT("BearAttackCostAdd"), BearStats.AttackCost));
    BearStats.TurnDelay = FMath::Max(0, ResolveStat(TEXT("BearTurnDelay"), TEXT("BearTurnDelayAdd"), BearStats.TurnDelay));

    Attributes->BaseStats = BearStats;
    Attributes->SetCurrentHealthValue(BearStats.MaxHealth);
    Attributes->SetCurrentInitiativeValue(FMath::Clamp(PreservedInitiative, 0, BearStats.MaxInitiative));
}

void ARoundCharacterDruid::RebuildGrantedAbilitiesForBearForm()
{
    GrantedAbilities.Reset();
    if (ARoundAbilityActor* CancelAbility = GetOrCreateBearCancelAbility())
    {
        GrantedAbilities.Add(CancelAbility);
    }
}

void ARoundCharacterDruid::RestoreGrantedAbilitiesAfterBearForm()
{
    GrantedAbilities = StoredDruidAbilities;
    StoredDruidAbilities.Reset();
}

ARoundAbilityActor* ARoundCharacterDruid::GetOrCreateBearCancelAbility()
{
    if (BearCancelAbility)
    {
        return BearCancelAbility;
    }

    if (!GetWorld())
    {
        return nullptr;
    }

    BearCancelAbility = GetWorld()->SpawnActor<ARoundAbilityActor>(ARoundCancelBearFormAbility::StaticClass());
    if (!BearCancelAbility)
    {
        return nullptr;
    }

    BearCancelAbility->AbilityData = BuildBearCancelAbilityData(BearCancelAbility);
    BearCancelAbility->OwningCharacter = this;
    BearCancelAbility->RowName = TEXT("CancelBearForm");
    BearCancelAbility->OnGrantedToCharacter();
    BearCancelAbility->InitializeCharges();
    BearCancelAbility->RefreshRuntimeState();
    BearCancelAbility->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
    return BearCancelAbility;
}
