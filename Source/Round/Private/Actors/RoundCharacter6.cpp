#include "Actors/RoundCharacter6.h"

#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundGridTile.h"
#include "Actors/RoundTeamActor.h"
#include "Abilities/RoundPuppetMoveAbility.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundAbilityData.h"
#include "Data/RoundCharacterData.h"
#include "Effects/RoundModifierEffect.h"
#include "EngineUtils.h"
#include "Game/RoundGameState.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundCharacter6Passive, Log, All);

namespace
{
ARoundModifierEffect* AddCharacter6ModifierEffect(
    ARoundCharacter* TargetCharacter,
    ARoundCharacter* CauserCharacter,
    const FString& DisplayName,
    const FString& Description,
    const int32 Duration,
    const double IncomingDamageMultiplier = 1.0,
    const double OutgoingDamageMultiplier = 1.0,
    const int32 MoveCostAdd = 0,
    const bool bBlockMovementBonuses = false,
    const bool bOnlyAffectCauserDamage = false,
    const int32 IncomingEffectDurationAdd = 0,
    const int32 DefenseIgnoreAdd = 0,
    const bool bOnlyAgainstRangedSources = false)
{
    if (!TargetCharacter || !TargetCharacter->GetWorld())
    {
        return nullptr;
    }

    ARoundModifierEffect* ModifierEffect = TargetCharacter->GetWorld()->SpawnActor<ARoundModifierEffect>(ARoundModifierEffect::StaticClass());
    if (!ModifierEffect)
    {
        return nullptr;
    }

    ModifierEffect->DisplayName = DisplayName;
    ModifierEffect->Description = Description;
    ModifierEffect->SetDurationInitiative(FMath::Max(1, Duration), false);
    ModifierEffect->Amount = 0.0;
    ModifierEffect->IncomingDamageMultiplier = IncomingDamageMultiplier;
    ModifierEffect->OutgoingDamageMultiplier = OutgoingDamageMultiplier;
    ModifierEffect->MoveCostAdd = MoveCostAdd;
    ModifierEffect->bBlockMovementBonuses = bBlockMovementBonuses;
    ModifierEffect->bOnlyAffectCauserDamage = bOnlyAffectCauserDamage;
    ModifierEffect->IncomingEffectDurationAdd = IncomingEffectDurationAdd;
    ModifierEffect->CauserCharacter = CauserCharacter;
    ModifierEffect->DefenseIgnoreAdd = DefenseIgnoreAdd;
    ModifierEffect->bOnlyAgainstRangedSources = bOnlyAgainstRangedSources;
    TargetCharacter->AddEffect(ModifierEffect);
    return ModifierEffect;
}
}

ARoundCharacter6::ARoundCharacter6()
{
    CharacterDataAsset = TSoftObjectPtr<URoundCharacterData>(FSoftObjectPath(TEXT("/Game/TopDown/Characters/Character_6/Character6Details.Character6Details")));
}

void ARoundCharacter6::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundCharacter6, NativePuppetCharacter);
}

void ARoundCharacter6::BeginPlay()
{
    Super::BeginPlay();

    if (bSpawnPuppetOnBeginPlay)
    {
        EnsurePuppetCharacter();
    }
}

void ARoundCharacter6::BeginTurn()
{
    Super::BeginTurn();
}

void ARoundCharacter6::AdvanceGlobalInitiative(int32 Amount)
{
    Super::AdvanceGlobalInitiative(Amount);
}

FRoundActionResult ARoundCharacter6::UseAbilityIndex(int32 AbilityIndex, ARoundGridTile* TargetTile)
{
    return Super::UseAbilityIndex(AbilityIndex, TargetTile);
}

void ARoundCharacter6::GrantDefaultAbilities()
{
    Super::GrantDefaultAbilities();

    if (GetAllowedAbilityCount() <= 0)
    {
        return;
    }

    URoundAbilityData* PuppetMoveAbilityData = LoadPuppetMoveAbilityData();
    if (!PuppetMoveAbilityData)
    {
        UE_LOG(LogTemp, Error, TEXT("Character6 missing PuppetMove ability data - Character=%s"), *GetName());
        return;
    }

    for (ARoundAbilityActor* ExistingAbility : GrantedAbilities)
    {
        if (ExistingAbility && ExistingAbility->AbilityData == PuppetMoveAbilityData)
        {
            if (GrantedAbilities[0] != ExistingAbility)
            {
                GrantedAbilities.RemoveSingleSwap(ExistingAbility);
                GrantedAbilities.Insert(ExistingAbility, 0);
            }
            return;
        }
    }

    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("Character6 cannot grant PuppetMove without world - Character=%s"), *GetName());
        return;
    }

    TSubclassOf<ARoundAbilityActor> PuppetMoveAbilityClass = PuppetMoveAbilityData->AbilityClass;
    if (!PuppetMoveAbilityClass)
    {
        PuppetMoveAbilityClass = ARoundPuppetMoveAbility::StaticClass();
        UE_LOG(LogTemp, Warning, TEXT("Character6 PuppetMove ability data has no class, using native fallback - AbilityData=%s"),
            *GetNameSafe(PuppetMoveAbilityData));
    }

    ARoundAbilityActor* AbilityActor = GetWorld()->SpawnActor<ARoundAbilityActor>(PuppetMoveAbilityClass);
    if (!AbilityActor)
    {
        UE_LOG(LogTemp, Error, TEXT("Character6 failed to spawn PuppetMove ability actor - AbilityClass=%s"),
            *GetNameSafe(PuppetMoveAbilityClass));
        return;
    }

    AbilityActor->AbilityData = PuppetMoveAbilityData;
    AbilityActor->OwningCharacter = this;
    AbilityActor->RowName = PuppetMoveAbilityData->AbilityId;
    AbilityActor->InitializeCharges();
    AbilityActor->RefreshRuntimeState();
    AbilityActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
    GrantedAbilities.Insert(AbilityActor, 0);
}

void ARoundCharacter6::SetPuppetCharacter(ARoundCharacter* InPuppetCharacter)
{
    NativePuppetCharacter = InPuppetCharacter;
    bHasCachedPuppetBaseStats = false;
    ApplyPuppetRoleStats();
    NotifyRoundStateChanged();
}

void ARoundCharacter6::ApplySimCoordinatorState(const bool bInPuppetMovedThisTurn, const double InPendingPuppetDamageMultiplier, const int32 InPendingActionInitiativeRefund)
{
    bPuppetMovedThisTurn = bInPuppetMovedThisTurn;
    PendingPuppetDamageMultiplier = InPendingPuppetDamageMultiplier > 0.0 ? InPendingPuppetDamageMultiplier : 1.0;
    PendingActionInitiativeRefund = FMath::Max(0, InPendingActionInitiativeRefund);
    NotifyRoundStateChanged();
}

ARoundCharacter* ARoundCharacter6::EnsurePuppetCharacter()
{
    if (NativePuppetCharacter || PuppetRowName.IsNone() || !CurrentTile)
    {
        return NativePuppetCharacter;
    }

    ARoundGameState* RoundGameState = GetRoundGameState();
    if (!RoundGameState || !HasAuthority())
    {
        return NativePuppetCharacter;
    }

    UObject* TeamObject = GetOwningTeamActor();

    if (!TeamObject)
    {
        for (UObject* Candidate : RoundGameState->Teams)
        {
            if (ARoundTeamActor* TeamActor = Cast<ARoundTeamActor>(Candidate))
            {
                if (TeamActor->TeamId == Team)
                {
                    TeamObject = TeamActor;
                    SetOwningTeamActor(TeamActor);
                    break;
                }
            }
        }
    }

    ARoundGridTile* PuppetField = RoundGameState->GetCloseFreeField(CurrentTile);
    if (!PuppetField)
    {
        PuppetField = CurrentTile;
    }

    ARoundCharacter* SpawnedPuppet = RoundGameState->SpawnCharacterByRowName(PuppetRowName, PuppetField->GridPosition, TeamObject, 0, 0, {});
    if (!SpawnedPuppet)
    {
        return nullptr;
    }

    NativePuppetCharacter = SpawnedPuppet;
    SpawnedPuppet->SetOwningTeamActor(GetOwningTeamActor());

    if (FObjectProperty* ControllerProperty = FindFProperty<FObjectProperty>(SpawnedPuppet->GetClass(), TEXT("Controller_0")))
    {
        ControllerProperty->SetObjectPropertyValue_InContainer(SpawnedPuppet, this);
    }

    ApplyPuppetRoleStats();
    NotifyRoundStateChanged();
    return NativePuppetCharacter;
}

ARoundCharacter* ARoundCharacter6::TryGetPuppetCharacter() const
{
    return NativePuppetCharacter;
}

int32 ARoundCharacter6::GetPuppetControlRange() const
{
    return FMath::Max(0, FMath::RoundToInt(GetValueFromKey(TEXT("PuppetControlRange"))) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("PuppetControlRangeAdd"))));
}

bool ARoundCharacter6::IsPuppetWithinControlRange() const
{
    return NativePuppetCharacter && NativePuppetCharacter->CurrentTile && CurrentTile
        && CurrentTile->GetDistance(NativePuppetCharacter->CurrentTile) <= GetPuppetControlRange();
}

bool ARoundCharacter6::IsPuppetDefendActive() const
{
    if (!NativePuppetCharacter)
    {
        return false;
    }

    for (ARoundEffectActor* Effect : NativePuppetCharacter->ActiveEffects)
    {
        if (Effect && Effect->EffectId == FName(TEXT("PuppetDefend")))
        {
            return true;
        }
    }

    return false;
}

bool ARoundCharacter6::IsTargetNearPuppet(const ARoundCharacter* TargetCharacter) const
{
    return NativePuppetCharacter && NativePuppetCharacter->CurrentTile && TargetCharacter && TargetCharacter->CurrentTile
        && NativePuppetCharacter->CurrentTile->GetDistance(TargetCharacter->CurrentTile) <= 1;
}

bool ARoundCharacter6::IsTargetInPuppetAttackRange(const ARoundCharacter* TargetCharacter) const
{
    return NativePuppetCharacter && NativePuppetCharacter->CurrentTile && TargetCharacter && TargetCharacter->CurrentTile
        && NativePuppetCharacter->CurrentTile->GetDistance(TargetCharacter->CurrentTile) <= NativePuppetCharacter->GetAttackRange();
}

bool ARoundCharacter6::WasTargetDamagedByCharacter6ThisTurn(const ARoundCharacter* TargetCharacter) const
{
    return TargetCharacter && PuppetTargetStacks.Contains(TargetCharacter);
}

bool ARoundCharacter6::WasTargetDamagedByPuppetThisTurn(const ARoundCharacter* TargetCharacter) const
{
    return TargetCharacter && Character6TargetStacks.Contains(TargetCharacter);
}

void ARoundCharacter6::SetPendingPuppetDamageMultiplier(const double InMultiplier)
{
    PendingPuppetDamageMultiplier = FMath::Max(PendingPuppetDamageMultiplier, InMultiplier);
}

void ARoundCharacter6::AddPendingActionInitiativeRefund(const int32 Amount)
{
    PendingActionInitiativeRefund = FMath::Max(PendingActionInitiativeRefund, Amount);
}

ARoundAbilityActor* ARoundCharacter6::FindGrantedAbilityById(const FName AbilityId) const
{
    for (ARoundAbilityActor* AbilityActor : GrantedAbilities)
    {
        if (AbilityActor && AbilityActor->AbilityData && AbilityActor->AbilityData->AbilityId == AbilityId)
        {
            return AbilityActor;
        }
    }

    return nullptr;
}

ARoundCharacter6* ARoundCharacter6::ResolveCoordinatorForCharacter(const ARoundCharacter* Character)
{
    if (!Character)
    {
        return nullptr;
    }

    if (ARoundCharacter6* Character6 = Cast<ARoundCharacter6>(const_cast<ARoundCharacter*>(Character)))
    {
        return Character6;
    }

    if (const FObjectProperty* ControllerProperty = FindFProperty<FObjectProperty>(Character->GetClass(), TEXT("Controller_0")))
    {
        return Cast<ARoundCharacter6>(ControllerProperty->GetObjectPropertyValue_InContainer(Character));
    }

    return nullptr;
}

double ARoundCharacter6::GetIncomingDamageMultiplier(ERoundDamageType DamageType) const
{
    double Multiplier = Super::GetIncomingDamageMultiplier(DamageType);

    if (FMath::RoundToInt(GetPassiveValueFromKey(TEXT("DamageMultiplierWhilePuppetCloser"))) > 0 && NativePuppetCharacter && NativePuppetCharacter->CurrentTile && CurrentTile)
    {
        ARoundCharacter* ClosestEnemyToSelf = nullptr;
        int32 SelfDistance = MAX_int32;
        int32 PuppetDistance = MAX_int32;

        if (UWorld* World = GetWorld())
        {
            for (TActorIterator<ARoundCharacter> It(World); It; ++It)
            {
                ARoundCharacter* OtherCharacter = *It;
                if (!OtherCharacter || !IsEnemyFor(OtherCharacter) || !OtherCharacter->CurrentTile)
                {
                    continue;
                }

                const int32 SelfCandidate = CurrentTile->GetDistance(OtherCharacter->CurrentTile);
                if (SelfCandidate < SelfDistance)
                {
                    SelfDistance = SelfCandidate;
                    ClosestEnemyToSelf = OtherCharacter;
                }
            }
        }

        if (ClosestEnemyToSelf && ClosestEnemyToSelf->CurrentTile)
        {
            PuppetDistance = NativePuppetCharacter->CurrentTile->GetDistance(ClosestEnemyToSelf->CurrentTile);
            if (PuppetDistance < SelfDistance)
            {
                const double PassiveMultiplier = GetPassiveValueFromKey(TEXT("DamageMultiplierWhilePuppetCloser"));
                if (PassiveMultiplier > 0.0)
                {
                    Multiplier *= PassiveMultiplier;
                }
            }
        }
    }

    if (FMath::RoundToInt(GetPassiveValueFromKey(TEXT("SafeDistanceDamageMultiplier"))) > 0 && !IsCharacterAdjacentToAnyEnemy(this) && IsPuppetWithinControlRange())
    {
        const double PassiveMultiplier = GetPassiveValueFromKey(TEXT("SafeDistanceDamageMultiplier"));
        if (PassiveMultiplier > 0.0)
        {
            Multiplier *= PassiveMultiplier;
        }
    }

    if (IsPuppetDefendActive())
    {
        if (const ARoundAbilityActor* PuppetDefendAbility = FindGrantedAbilityById(FName(TEXT("PuppetDefend"))))
        {
            const double DefendMultiplier = PuppetDefendAbility->GetAbilityValueFromKey(TEXT("SelfIncomingDamageMultiplierIfPuppetCloser"));
            if (DefendMultiplier > 0.0 && NativePuppetCharacter && NativePuppetCharacter->CurrentTile && CurrentTile)
            {
                ARoundCharacter* ClosestEnemyToSelf = nullptr;
                int32 SelfDistance = MAX_int32;
                int32 PuppetDistance = MAX_int32;

                if (UWorld* World = GetWorld())
                {
                    for (TActorIterator<ARoundCharacter> It(World); It; ++It)
                    {
                        ARoundCharacter* OtherCharacter = *It;
                        if (!OtherCharacter || !IsEnemyFor(OtherCharacter) || !OtherCharacter->CurrentTile)
                        {
                            continue;
                        }

                        const int32 SelfCandidate = CurrentTile->GetDistance(OtherCharacter->CurrentTile);
                        if (SelfCandidate < SelfDistance)
                        {
                            SelfDistance = SelfCandidate;
                            ClosestEnemyToSelf = OtherCharacter;
                        }
                    }
                }

                if (ClosestEnemyToSelf && ClosestEnemyToSelf->CurrentTile)
                {
                    PuppetDistance = NativePuppetCharacter->CurrentTile->GetDistance(ClosestEnemyToSelf->CurrentTile);
                    if (PuppetDistance < SelfDistance)
                    {
                        Multiplier *= DefendMultiplier;
                    }
                }
            }
        }
    }

    return Multiplier;
}

void ARoundCharacter6::HandlePassiveDamageTaken(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType)
{
    Super::HandlePassiveDamageTaken(AppliedDamage, SourceCharacter, DamageType);

    if (AppliedDamage <= 0 || !SourceCharacter || !IsPuppetDefendActive())
    {
        return;
    }

    const ARoundAbilityActor* PuppetDefendAbility = FindGrantedAbilityById(FName(TEXT("PuppetDefend")));
    if (!PuppetDefendAbility)
    {
        return;
    }

    const int32 InitiativeLossOnAttack = FMath::RoundToInt(PuppetDefendAbility->GetAbilityValueFromKey(TEXT("InitiativeLossOnAttack")));
    if (InitiativeLossOnAttack > 0)
    {
        SourceCharacter->ApplyDelay(InitiativeLossOnAttack);
    }

    if (IsTargetNearPuppet(SourceCharacter))
    {
        const int32 InitiativeLossNearPuppet = FMath::RoundToInt(PuppetDefendAbility->GetAbilityValueFromKey(TEXT("InitiativeLossOnAttackNearPuppet")));
        if (InitiativeLossNearPuppet > 0)
        {
            SourceCharacter->ApplyDelay(InitiativeLossNearPuppet);
        }

        const double EnemyOutgoingMultiplier = PuppetDefendAbility->GetAbilityValueFromKey(TEXT("OutgoingDamageMultiplierEnemiesNearPuppet"));
        if (EnemyOutgoingMultiplier > 0.0 && EnemyOutgoingMultiplier < 1.0)
        {
            AddCharacter6ModifierEffect(
                SourceCharacter,
                this,
                TEXT("Sperrpunkt"),
                TEXT("Verursacht in der Naehe der Puppe voruebergehend weniger Schaden."),
                1,
                1.0,
                EnemyOutgoingMultiplier,
                0,
                false,
                false,
                0);
        }
    }
}

TObjectPtr<URoundAbilityData> ARoundCharacter6::LoadPuppetMoveAbilityData() const
{
    static const TSoftObjectPtr<URoundAbilityData> PuppetMoveAsset(
        FSoftObjectPath(TEXT("/Game/TopDown/Data/Abilities/DA_Ability_PuppetMove.DA_Ability_PuppetMove")));

    return PuppetMoveAsset.LoadSynchronous();
}

void ARoundCharacter6::ApplyPuppetRoleStats()
{
    ARoundCharacter* PuppetCharacter = NativePuppetCharacter;
    if (!PuppetCharacter || !PuppetCharacter->Attributes)
    {
        return;
    }

    if (!bHasCachedPuppetBaseStats)
    {
        CachedPuppetBaseStats = PuppetCharacter->Attributes->BaseStats;
        bHasCachedPuppetBaseStats = true;
    }

    FRoundStatBlock NewStats = CachedPuppetBaseStats;

    NewStats.MaxHealth = FMath::Max(1, NewStats.MaxHealth
        + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("PuppetMaxHealthAdd")))
        + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("PuppetRoleMaxHealthAdd"))));
    NewStats.Attack = FMath::Max(0, NewStats.Attack
        + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("PuppetAttackAdd")))
        + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("PuppetRoleAttackAdd"))));
    NewStats.Defense = FMath::Max(0, NewStats.Defense
        + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("PuppetDefenseAdd")))
        + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("PuppetRoleDefenseAdd"))));
    NewStats.MoveCost = FMath::Max(1, NewStats.MoveCost + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("PuppetMoveCostAdd"))));
    NewStats.AttackRange = FMath::Max(1, NewStats.AttackRange + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("PuppetRangeAdd"))));
    NewStats.TurnDelay = FMath::Max(0, NewStats.TurnDelay + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("PuppetTurnDelayAdd"))));

    const int32 PreviousMaxHealth = PuppetCharacter->Attributes->BaseStats.MaxHealth;
    const int32 PreviousCurrentHealth = PuppetCharacter->Attributes->CurrentHealth;
    PuppetCharacter->Attributes->InitializeFromStats(NewStats);
    if (PreviousMaxHealth > 0)
    {
        const double HealthRatio = static_cast<double>(PreviousCurrentHealth) / static_cast<double>(PreviousMaxHealth);
        PuppetCharacter->Attributes->CurrentHealth = FMath::Clamp(FMath::RoundToInt(static_cast<double>(NewStats.MaxHealth) * HealthRatio), 1, NewStats.MaxHealth);
    }
}

bool ARoundCharacter6::IsCharacterAdjacentToAnyEnemy(const ARoundCharacter* Character) const
{
    if (!Character || !Character->CurrentTile || !GetWorld())
    {
        return false;
    }

    for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
    {
        ARoundCharacter* OtherCharacter = *It;
        if (!OtherCharacter || !Character->IsEnemyFor(OtherCharacter) || !OtherCharacter->CurrentTile)
        {
            continue;
        }

        if (Character->CurrentTile->GetDistance(OtherCharacter->CurrentTile) <= 1)
        {
            return true;
        }
    }

    return false;
}

int32 ARoundCharacter6::GetStacksAgainstTarget(const ARoundCharacter* DamageSource, const ARoundCharacter* TargetCharacter) const
{
    if (!DamageSource || !TargetCharacter)
    {
        return 0;
    }

    if (DamageSource == this)
    {
        return Character6TargetStacks.FindRef(TargetCharacter);
    }

    if (DamageSource == NativePuppetCharacter)
    {
        return PuppetTargetStacks.FindRef(TargetCharacter);
    }

    return 0;
}

void ARoundCharacter6::AddHuntStackForOtherActor(const ARoundCharacter* DamageSource, ARoundCharacter* TargetCharacter)
{
    if (!DamageSource || !TargetCharacter)
    {
        return;
    }

    if (DamageSource == this)
    {
        PuppetTargetStacks.FindOrAdd(TargetCharacter) += 1;
    }
    else if (DamageSource == NativePuppetCharacter)
    {
        Character6TargetStacks.FindOrAdd(TargetCharacter) += 1;
    }
}

void ARoundCharacter6::TickTargetCooldownMap(TMap<TWeakObjectPtr<ARoundCharacter>, int32>& CooldownMap, int32 Amount)
{
    const int32 EffectiveAmount = FMath::Max(0, Amount);
    for (auto It = CooldownMap.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
            continue;
        }

        const int32 NewValue = It.Value() - EffectiveAmount;
        if (NewValue <= 0)
        {
            It.RemoveCurrent();
        }
        else
        {
            It.Value() = NewValue;
        }
    }
}

int32 ARoundCharacter6::ModifyCoordinatedOutgoingDamage(const ARoundCharacter* DamageSource, ARoundCharacter* TargetCharacter, int32 Amount) const
{
    int32 ModifiedAmount = Amount;
    if (!DamageSource || !TargetCharacter)
    {
        return ModifiedAmount;
    }

    if (GetStacksAgainstTarget(DamageSource, TargetCharacter) > 0)
    {
        const double HuntMultiplier = GetPassiveValueFromKey(TEXT("SharedHuntDamageMultiplier"));
        if (HuntMultiplier > 0.0)
        {
            const int32 StackCount = GetStacksAgainstTarget(DamageSource, TargetCharacter);
            for (int32 Index = 0; Index < StackCount; ++Index)
            {
                ModifiedAmount = FMath::Max(0, FMath::RoundToInt(static_cast<double>(ModifiedAmount) * HuntMultiplier));
            }
        }
    }

    if (DamageSource == NativePuppetCharacter && PendingPuppetDamageMultiplier > 1.0)
    {
        ModifiedAmount = FMath::Max(0, FMath::RoundToInt(static_cast<double>(ModifiedAmount) * PendingPuppetDamageMultiplier));
    }

    if (TargetCharacter->CurrentTile && CurrentTile && NativePuppetCharacter && NativePuppetCharacter->CurrentTile)
    {
        const bool bSelfCanReach = CurrentTile->GetDistance(TargetCharacter->CurrentTile) <= GetAttackRange();
        const bool bPuppetCanReach = NativePuppetCharacter->CurrentTile->GetDistance(TargetCharacter->CurrentTile) <= NativePuppetCharacter->GetAttackRange();
        if (bSelfCanReach && bPuppetCanReach)
        {
            const double Multiplier = GetPassiveValueFromKey(TEXT("DamageMultiplierWhenBothThreatenSameTarget"));
            if (Multiplier > 0.0)
            {
                ModifiedAmount = FMath::Max(0, FMath::RoundToInt(static_cast<double>(ModifiedAmount) * Multiplier));
            }
        }
    }

    return ModifiedAmount;
}

void ARoundCharacter6::HandleCoordinatedDamageDealt(const ARoundCharacter* DamageSource, ARoundCharacter* TargetCharacter, int32 AppliedDamage)
{
    if (!DamageSource || !TargetCharacter || AppliedDamage <= 0)
    {
        return;
    }

    AddHuntStackForOtherActor(DamageSource, TargetCharacter);

    if (DamageSource == NativePuppetCharacter)
    {
        PendingPuppetDamageMultiplier = 1.0;
    }

    if (TargetCharacter->CurrentTile && NativePuppetCharacter && NativePuppetCharacter->CurrentTile)
    {
        if (NativePuppetCharacter->CurrentTile->GetDistance(TargetCharacter->CurrentTile) <= 1)
        {
            const int32 CooldownTurns = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InitiativeLossNearPuppetCooldownInitiative"))));
            if (!InitiativeLossCooldowns.Contains(TargetCharacter))
            {
                const int32 InitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InitiativeLossNearPuppetOnHit"))));
                if (InitiativeLoss > 0)
                {
                    TargetCharacter->ApplyDelay(InitiativeLoss);
                    if (CooldownTurns > 0)
                    {
                        InitiativeLossCooldowns.Add(TargetCharacter, CooldownTurns);
                    }
                }
            }
        }
    }

    const int32 SharedLossCooldown = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("SharedHitInitiativeLossCooldownInitiative"))));
    if (!SharedHitInitiativeLossCooldowns.Contains(TargetCharacter))
    {
        const int32 SharedInitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("SharedHitInitiativeLoss"))));
        if (SharedInitiativeLoss > 0)
        {
            TargetCharacter->ApplyDelay(SharedInitiativeLoss);
            if (SharedLossCooldown > 0)
            {
                SharedHitInitiativeLossCooldowns.Add(TargetCharacter, SharedLossCooldown);
            }
        }
    }

    if (IsPuppetDefendActive() && DamageSource == this)
    {
        if (const ARoundAbilityActor* PuppetDefendAbility = FindGrantedAbilityById(FName(TEXT("PuppetDefend"))))
        {
            if (IsTargetInPuppetAttackRange(TargetCharacter))
            {
                const int32 InitiativeGainOnAttackInPuppetRange = FMath::RoundToInt(PuppetDefendAbility->GetAbilityValueFromKey(TEXT("InitiativeGainOnAttackInPuppetRange")));
                if (InitiativeGainOnAttackInPuppetRange > 0 && Attributes)
                {
                    Attributes->GainInitiative(InitiativeGainOnAttackInPuppetRange);
                    NotifyRoundStateChanged();
                }
            }

            const double DamageMultiplierVsTargetsNearPuppet = PuppetDefendAbility->GetAbilityValueFromKey(TEXT("DamageMultiplierVsTargetsNearPuppet"));
            if (DamageMultiplierVsTargetsNearPuppet > 1.0 && IsTargetNearPuppet(TargetCharacter))
            {
                AddCharacter6ModifierEffect(
                    TargetCharacter,
                    this,
                    TEXT("Taktischer Schirm"),
                    TEXT("Character 6 verursacht in der Naehe der Puppe voruebergehend mehr Schaden."),
                    1,
                    DamageMultiplierVsTargetsNearPuppet,
                    1.0,
                    0,
                    false,
                    true,
                    0);
            }
        }
    }
}
