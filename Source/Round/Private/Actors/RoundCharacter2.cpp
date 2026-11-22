#include "Actors/RoundCharacter2.h"

#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundAbilityData.h"
#include "Data/RoundCharacterData.h"
#include "Effects/RoundConcentrateEffect.h"
#include "Effects/RoundModifierEffect.h"
#include "Net/UnrealNetwork.h"

ARoundCharacter2::ARoundCharacter2()
{
    CharacterDataAsset = TSoftObjectPtr<URoundCharacterData>(FSoftObjectPath(TEXT("/Game/TopDown/Characters/Character_2/Character2Details.Character2Details")));
}

void ARoundCharacter2::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundCharacter2, Ammo);
    DOREPLIFETIME(ARoundCharacter2, bAttackedThisTurn);
    DOREPLIFETIME(ARoundCharacter2, bSpentAmmoThisTurn);
    DOREPLIFETIME(ARoundCharacter2, bMovedThisTurn);
    DOREPLIFETIME(ARoundCharacter2, bStartedTurnWithFullAmmo);
    DOREPLIFETIME(ARoundCharacter2, bHeldPositionFromPreviousTurn);
    DOREPLIFETIME(ARoundCharacter2, bUsedFreeBasicAttackThisTurn);
    DOREPLIFETIME(ARoundCharacter2, bTriggeredEmptyMagazineThisTurn);
    DOREPLIFETIME(ARoundCharacter2, bRefundedLastAmmoKillThisTurn);
    DOREPLIFETIME(ARoundCharacter2, bUsedSnipeRangeRefundThisTurn);
    DOREPLIFETIME(ARoundCharacter2, bUsedSnipeKillCooldownReductionThisTurn);
    DOREPLIFETIME(ARoundCharacter2, StationaryShotStacks);
    DOREPLIFETIME(ARoundCharacter2, bHasFreeConcentrateStep);
}

void ARoundCharacter2::OnRep_ResourceState()
{
    NotifyRoundStateChanged();
}

int32 ARoundCharacter2::GetMaxAmmo() const
{
    const int32 MaxAmmo = FMath::TruncToInt(GetValueFromKey(TEXT("MaxAmmo"))) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MaxAmmoAdd")));
    return FMath::Max(1, MaxAmmo);
}

bool ARoundCharacter2::HasAmmo(int32 RequiredAmmo) const
{
    return Ammo >= FMath::Max(1, RequiredAmmo);
}

int32 ARoundCharacter2::ConsumeAmmo(int32 Amount)
{
    const int32 RequestedAmmo = FMath::Max(1, Amount);
    const int32 ConsumedAmmo = FMath::Min(Ammo, RequestedAmmo);
    Ammo -= ConsumedAmmo;
    bAttackedThisTurn = bAttackedThisTurn || ConsumedAmmo > 0;
    bSpentAmmoThisTurn = bSpentAmmoThisTurn || ConsumedAmmo > 0;
    if (ConsumedAmmo > 0
        && Ammo == 0
        && !bTriggeredEmptyMagazineThisTurn
        && GetPassiveValueFromKey(TEXT("EmptyMagazineInitiativeGain")) > 0.0)
    {
        bTriggeredEmptyMagazineThisTurn = true;
        if (Attributes)
        {
            Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("EmptyMagazineInitiativeGain")))));
        }
    }
    if (ConsumedAmmo > 0)
    {
        ForceNetUpdate();
        NotifyRoundStateChanged();
    }
    return ConsumedAmmo;
}

int32 ARoundCharacter2::RegenerateAmmo(int32 Amount)
{
    const int32 AmmoToRestore = FMath::Max(0, Amount);
    const int32 PreviousAmmo = Ammo;
    Ammo = FMath::Min(GetMaxAmmo(), Ammo + AmmoToRestore);
    if (Ammo != PreviousAmmo)
    {
        ForceNetUpdate();
        NotifyRoundStateChanged();
    }
    return Ammo;
}

void ARoundCharacter2::ResolveAmmoShot(bool bKilledTarget, bool bRefundAmmo)
{
    const int32 AmmoBeforeShot = Ammo;
    ConsumeAmmo();

    const bool bLastAmmoKillRefund = bKilledTarget
        && AmmoBeforeShot == 1
        && !bRefundedLastAmmoKillThisTurn
        && GetPassiveValueFromKey(TEXT("LastAmmoKillRefund")) > 0.0;
    if (bRefundAmmo || bLastAmmoKillRefund)
    {
        RegenerateAmmo(1);
    }
    bRefundedLastAmmoKillThisTurn = bRefundedLastAmmoKillThisTurn || bLastAmmoKillRefund;

    if (!bMovedThisTurn && GetPassiveValueFromKey(TEXT("StationaryShotDamagePerStack")) > 0.0)
    {
        const int32 MaxStacks = FMath::Max(1, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("StationaryShotMaxStacks"))));
        StationaryShotStacks = FMath::Min(MaxStacks, StationaryShotStacks + 1);
    }
    ForceNetUpdate();
    NotifyRoundStateChanged();
}

void ARoundCharacter2::ResolveConcentrateHit(ARoundCharacter* TargetCharacter, ARoundAbilityActor* UsedAbility, bool bKilledTarget, bool bWasBasicAttack)
{
    for (ARoundEffectActor* Effect : ActiveEffects)
    {
        if (ARoundConcentrateEffect* Concentrate = Cast<ARoundConcentrateEffect>(Effect))
        {
            Concentrate->ResolveEmpoweredHit(this, TargetCharacter, UsedAbility, bKilledTarget, bWasBasicAttack);
            return;
        }
    }
}

void ARoundCharacter2::FireConcentrateMagazineVolley(ARoundCharacter* TargetCharacter)
{
    const int32 ShotsToFire = Ammo;
    for (int32 Shot = 0; Shot < ShotsToFire; ++Shot)
    {
        bool bKilled = !TargetCharacter || !TargetCharacter->IsAlive();
        if (!bKilled && Attributes)
        {
            int32 Damage = FMath::RoundToInt(Attributes->BaseStats.Attack * 0.5);
            if (FMath::FRand() < GetCritChance()) Damage = FMath::RoundToInt(Damage * GetCritModifier());
            int32 DefenseIgnore = 0;
            Damage = ModifyOutgoingDamage(Damage, ERoundDamageType::Physical, TargetCharacter, DefenseIgnore);
            TargetCharacter->ApplyRoundDamage(Damage, ERoundDamageType::Physical, DefenseIgnore, this, bKilled);
        }
        ResolveAmmoShot(bKilled);
    }
}

void ARoundCharacter2::AddPiercingChainMarks(const TArray<ARoundCharacter*>& Targets, double DamageMultiplier)
{
    PiercingChainMarkedTargets.Reset();
    for (ARoundCharacter* Target : Targets)
    {
        if (Target && Target->IsAlive())
        {
            PiercingChainMarkedTargets.AddUnique(Target);
        }
    }
    PiercingChainDamageMultiplier = FMath::Max(0.0, DamageMultiplier);
}

void ARoundCharacter2::AddPiercingDefenseStacks(int32 StackCount, int32 ReductionPerStack, int32 DurationInitiative)
{
    PendingPiercingDefenseStacks += FMath::Max(0, StackCount);
    PiercingDefenseReductionPerStack = FMath::Max(0, ReductionPerStack);
    PiercingDefenseReductionDuration = FMath::Max(0, DurationInitiative);
    ForceNetUpdate();
    NotifyRoundStateChanged();
}

void ARoundCharacter2::GrantFreeSuppressionFollowup(ARoundCharacter* TargetCharacter)
{
    FreeSuppressionFollowupTarget = TargetCharacter;
    ForceNetUpdate();
    NotifyRoundStateChanged();
}

void ARoundCharacter2::FireSuppressionReaction(ARoundCharacter* TargetCharacter, double DamageMultiplier)
{
    if (!TargetCharacter || !TargetCharacter->IsAlive() || !HasAmmo() || !Attributes) return;
    int32 Damage = FMath::RoundToInt(static_cast<double>(Attributes->BaseStats.Attack) * FMath::Max(0.0, DamageMultiplier));
    if (FMath::FRand() < GetCritChance()) Damage = FMath::RoundToInt(static_cast<double>(Damage) * GetCritModifier());
    int32 DefenseIgnore = 0;
    Damage = ModifyOutgoingDamage(Damage, ERoundDamageType::Physical, TargetCharacter, DefenseIgnore);
    bool bKilled = false;
    TargetCharacter->ApplyRoundDamage(Damage, ERoundDamageType::Physical, DefenseIgnore, this, bKilled);
    ResolveAmmoShot(bKilled);
}

void ARoundCharacter2::InitializeCharacter()
{
    Super::InitializeCharacter();
    Ammo = GetMaxAmmo();
    ForceNetUpdate();
    NotifyRoundStateChanged();
}

void ARoundCharacter2::BeginTurn()
{
    Super::BeginTurn();
    ForceNetUpdate();
    NotifyRoundStateChanged();
}

FRoundActionResult ARoundCharacter2::AttackField(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacter* TargetCharacter = TargetTile ? TargetTile->OccupyingCharacter : nullptr;
    ARoundConcentrateEffect* Concentrate = nullptr;
    bool bCoreConcentrateFreeAttack = false;
    bool bCoreSuppressionFreeAttack = false;
    for (ARoundEffectActor* Effect : ActiveEffects)
    {
        Concentrate = Cast<ARoundConcentrateEffect>(Effect);
        if (Concentrate) break;
        const FString* SourceAbilityId = Effect ? Effect->StringState.Find(TEXT("CoreSourceAbilityId")) : nullptr;
        const int32 Flags = Effect ? Effect->IntegerState.FindRef(TEXT("CoreFlags")) : 0;
        if (SourceAbilityId && SourceAbilityId->Equals(TEXT("Concentrate"), ESearchCase::IgnoreCase)
            && (Flags & 2) != 0)
        {
            bCoreConcentrateFreeAttack = true;
        }
        const FString* PreferredTarget = Effect ? Effect->StringState.Find(TEXT("CorePreferredTarget")) : nullptr;
        if (SourceAbilityId && SourceAbilityId->Equals(TEXT("SuppressingFire"), ESearchCase::IgnoreCase)
            && Effect->IntegerState.FindRef(TEXT("CoreValueA")) == 1 && TargetCharacter
            && PreferredTarget && *PreferredTarget == TargetCharacter->GetPathName())
        {
            bCoreSuppressionFreeAttack = true;
        }
    }
    const bool bConcentrateFreeAttack = (Concentrate && Concentrate->bBasicAttackConsumesNoAmmo)
        || bCoreConcentrateFreeAttack;
    const bool bPassiveFreeAttack = !bUsedFreeBasicAttackThisTurn && GetPassiveValueFromKey(TEXT("FirstBasicAttackFreeAmmo")) > 0.0;
    const bool bSuppressionFreeAttack = (TargetCharacter && TargetCharacter == FreeSuppressionFollowupTarget)
        || bCoreSuppressionFreeAttack;
    const bool bFreeBasicAttack = bConcentrateFreeAttack || bPassiveFreeAttack || bSuppressionFreeAttack;
    if (!bFreeBasicAttack && !HasAmmo())
    {
        Result.Reason = TEXT("No ammo.");
        return Result;
    }

    PiercingMarkedAttackRangeBonus = 0;
    if (TargetCharacter)
    {
        for (const ARoundEffectActor* Effect : TargetCharacter->ActiveEffects)
        {
            if (!Effect || Effect->CauserCharacter != this) continue;
            if (Effect->EffectId == FName(TEXT("PiercingTargetMark")))
            {
                PiercingMarkedAttackRangeBonus = FMath::Max(0, FMath::RoundToInt(Effect->Amount));
                break;
            }
            const FString* SourceAbilityId = Effect->StringState.Find(TEXT("CoreSourceAbilityId"));
            if (SourceAbilityId && SourceAbilityId->Equals(TEXT("PiercingShot"), ESearchCase::IgnoreCase))
            {
                for (const ARoundAbilityActor* Ability : GrantedAbilities)
                {
                    if (Ability && Ability->AbilityData
                        && Ability->AbilityData->AbilityId == FName(TEXT("PiercingShot")))
                    {
                        PiercingMarkedAttackRangeBonus = FMath::Max(0,
                            FMath::RoundToInt(Ability->GetAbilityValueFromKey(TEXT("AttackRangeVsMarked"))));
                        break;
                    }
                }
                break;
            }
        }
    }
    Result = Super::AttackField(TargetTile);
    PiercingMarkedAttackRangeBonus = 0;
    if (Result.bSuccess)
    {
        bAttackedThisTurn = true;
        if (bFreeBasicAttack)
        {
            bUsedFreeBasicAttackThisTurn = bUsedFreeBasicAttackThisTurn || bPassiveFreeAttack;
            if (bSuppressionFreeAttack) FreeSuppressionFollowupTarget = nullptr;
            ForceNetUpdate();
            NotifyRoundStateChanged();
        }
        ResolveConcentrateHit(TargetCharacter, nullptr, TargetCharacter && !TargetCharacter->IsAlive(), true);

        if (TargetCharacter && PiercingChainMarkedTargets.Contains(TargetCharacter))
        {
            const TArray<TObjectPtr<ARoundCharacter>> MarkedTargets = PiercingChainMarkedTargets;
            PiercingChainMarkedTargets.Reset();
            for (ARoundCharacter* MarkedTarget : MarkedTargets)
            {
                if (!MarkedTarget || MarkedTarget == TargetCharacter || !MarkedTarget->IsAlive() || !Attributes)
                {
                    continue;
                }
                int32 DefenseIgnore = 0;
                int32 Damage = FMath::RoundToInt(static_cast<double>(Attributes->BaseStats.Attack) * PiercingChainDamageMultiplier);
                Damage = ModifyOutgoingDamage(Damage, ERoundDamageType::Physical, MarkedTarget, DefenseIgnore);
                bool bDidDie = false;
                MarkedTarget->ApplyRoundDamage(Damage, ERoundDamageType::Physical, DefenseIgnore, this, bDidDie);
            }
        }

        if (TargetCharacter && PendingPiercingDefenseStacks > 0 && PiercingDefenseReductionPerStack > 0)
        {
            if (ARoundModifierEffect* Debuff = GetWorld()->SpawnActor<ARoundModifierEffect>())
            {
                Debuff->EffectId = TEXT("PiercingDefenseBreak");
                Debuff->DisplayName = TEXT("Panzerbrechende Vorbereitung");
                Debuff->Description = TEXT("Die Defense wurde durch den vorbereiteten Schuss reduziert.");
                Debuff->DefenseIgnoreAdd = PendingPiercingDefenseStacks * PiercingDefenseReductionPerStack;
                Debuff->bTreatAsDebuff = true;
                Debuff->CauserCharacter = this;
                Debuff->SetDurationInitiative(PiercingDefenseReductionDuration, false);
                TargetCharacter->AddEffect(Debuff);
            }
            PendingPiercingDefenseStacks = 0;
        }
    }
    return Result;
}

FRoundActionResult ARoundCharacter2::MoveAlongPath(const TArray<ARoundGridTile*>& Path)
{
    const int32 InitiativeBeforeMove = Attributes ? Attributes->CurrentInitiative : 0;
    FRoundActionResult Result = Super::MoveAlongPath(Path);
    if (Result.bSuccess)
    {
        bMovedThisTurn = true;
        StationaryShotStacks = 0;
        if (bHasFreeConcentrateStep && Path.Num() == 1 && Attributes)
        {
            Attributes->GainInitiative(FMath::Max(0, InitiativeBeforeMove - Attributes->CurrentInitiative));
            bHasFreeConcentrateStep = false;
        }
        ForceNetUpdate();
        NotifyRoundStateChanged();
    }
    return Result;
}

void ARoundCharacter2::EndTurn()
{
    bHasCompletedTurn = true;
    Super::EndTurn();
}

int32 ARoundCharacter2::ApplyRoundDamage(int32 Amount, ERoundDamageType DamageType, int32 DefenseIgnore, ARoundCharacter* SourceCharacter, bool& bDidDie)
{
    int32 ModifiedAmount = Amount;
    const double RangedMultiplier = GetPassiveValueFromKey(TEXT("RangedDamageMultiplier"));
    if (RangedMultiplier > 0.0
        && SourceCharacter
        && CurrentTile
        && SourceCharacter->CurrentTile
        && CurrentTile->GetDistance(SourceCharacter->CurrentTile) > 1)
    {
        ModifiedAmount = FMath::RoundToInt(static_cast<double>(ModifiedAmount) * RangedMultiplier);
    }
    return Super::ApplyRoundDamage(ModifiedAmount, DamageType, DefenseIgnore, SourceCharacter, bDidDie);
}

int32 ARoundCharacter2::GetTurnDelay() const
{
    const int32 FullAmmoBonus = bStartedTurnWithFullAmmo
        ? FMath::RoundToInt(GetPassiveValueFromKey(TEXT("TurnDelayAddWhileFullAmmo")))
        : 0;
    return FMath::Max(0, Super::GetTurnDelay() + FullAmmoBonus);
}

int32 ARoundCharacter2::GetAttackRange() const
{
    return Super::GetAttackRange() + PiercingMarkedAttackRangeBonus;
}

int32 ARoundCharacter2::ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const
{
    int32 ModifiedDamage = Super::ModifyOutgoingDamage(Amount, DamageType, TargetCharacter, DefenseIgnore);
    if (!bMovedThisTurn)
    {
        const double DamagePerStack = GetPassiveValueFromKey(TEXT("StationaryShotDamagePerStack"));
        if (DamagePerStack > 0.0 && StationaryShotStacks > 0)
        {
            ModifiedDamage = FMath::RoundToInt(static_cast<double>(ModifiedDamage) * (1.0 + DamagePerStack * StationaryShotStacks));
        }
    }

    if (Ammo == 1)
    {
        const double LastAmmoMultiplier = GetPassiveValueFromKey(TEXT("AllDamageMultiplierWhileOneAmmo"));
        if (LastAmmoMultiplier > 0.0)
        {
            ModifiedDamage = FMath::RoundToInt(static_cast<double>(ModifiedDamage) * LastAmmoMultiplier);
        }
    }
    return FMath::Max(0, ModifiedDamage);
}

int32 ARoundCharacter2::GetIncomingBonusDefense(ERoundDamageType DamageType) const
{
    const int32 BaseBonus = Super::GetIncomingBonusDefense(DamageType);
    const int32 HeldPositionBonus = bHeldPositionFromPreviousTurn && !bMovedThisTurn
        ? FMath::RoundToInt(GetPassiveValueFromKey(TEXT("DefenseAddWhileHoldingPosition")))
        : 0;
    return FMath::Max(0, BaseBonus + HeldPositionBonus);
}
