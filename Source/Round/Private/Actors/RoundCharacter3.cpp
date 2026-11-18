#include "Actors/RoundCharacter3.h"

#include "Actors/RoundGridTile.h"
#include "Abilities/RoundHookShotAbility.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundCharacterData.h"
#include "Effects/RoundBleedEffect.h"
#include "Effects/RoundAssassinHuntMarkEffect.h"
#include "Effects/RoundInvisibilityEffect.h"
#include "Net/UnrealNetwork.h"

ARoundCharacter3::ARoundCharacter3()
{
    CharacterDataAsset = TSoftObjectPtr<URoundCharacterData>(FSoftObjectPath(TEXT("/Game/TopDown/Characters/Character_3/Character3Details.Character3Details")));
}

void ARoundCharacter3::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundCharacter3, NativeComboPoints);
}

void ARoundCharacter3::OnRep_ComboState()
{
    NotifyRoundStateChanged();
}

int32 ARoundCharacter3::GetMaxComboPoints() const
{
    const int32 MaxComboPoints = FMath::RoundToInt(GetValueFromKey(TEXT("MaxComboPoints"))) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MaxComboPointsAdd")));
    return FMath::Max(1, MaxComboPoints);
}

int32 ARoundCharacter3::GetComboPointsStart() const
{
    const int32 ComboPointsStart = FMath::RoundToInt(GetValueFromKey(TEXT("ComboPointsStart"))) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("ComboPointsStartAdd")));
    return FMath::Max(0, ComboPointsStart);
}

void ARoundCharacter3::InitializeCharacter()
{
    Super::InitializeCharacter();
    NativeComboPoints = FMath::Clamp(NativeComboPoints, 0, GetMaxComboPoints());

    if (NativeComboPoints == 0)
    {
        NativeComboPoints = FMath::Clamp(GetComboPointsStart(), 0, GetMaxComboPoints());
    }

    NotifyRoundStateChanged();
    EnsureStartInvisibility();
}

int32 ARoundCharacter3::TryGetComboPoints() const
{
    return FMath::Clamp(NativeComboPoints, 0, GetMaxComboPoints());
}

void ARoundCharacter3::ApplySimAssassinState(
    const int32 InComboPoints,
    const int32 InMovedFieldsThisTurn,
    const int32 InMarkedTargetCooldownTurnsRemaining,
    const int32 InColdPressureCooldownTurnsRemaining,
    const int32 InComboSpentThisTurn,
    const bool bInFirstDamageTurnConsumed,
    const bool bInFirstDamageTurnActive,
    const bool bInStartInvisibilityApplied)
{
    NativeComboPoints = FMath::Clamp(InComboPoints, 0, GetMaxComboPoints());
    NativeMovedFieldsThisTurn = FMath::Max(0, InMovedFieldsThisTurn);
    NativeMarkedTargetCooldownTurnsRemaining = FMath::Max(0, InMarkedTargetCooldownTurnsRemaining);
    NativeColdPressureCooldownTurnsRemaining = FMath::Max(0, InColdPressureCooldownTurnsRemaining);
    NativeComboSpentThisTurn = FMath::Max(0, InComboSpentThisTurn);
    bNativeFirstDamageTurnConsumed = bInFirstDamageTurnConsumed;
    bNativeFirstDamageTurnActive = bInFirstDamageTurnActive;
    bNativeStartInvisibilityApplied = bInStartInvisibilityApplied;
    NotifyRoundStateChanged();
}

int32 ARoundCharacter3::TryConsumeComboPoints(int32 MaxAmount)
{
    const int32 CurrentComboPoints = FMath::Clamp(NativeComboPoints, 0, GetMaxComboPoints());
    const int32 ConsumedComboPoints = FMath::Min(CurrentComboPoints, FMath::Max(0, MaxAmount));
    NativeComboPoints = CurrentComboPoints - ConsumedComboPoints;
    NativeComboSpentThisTurn += ConsumedComboPoints;

    const int32 InitiativePerPoint = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("ComboSpendInitiativeGainPerPoint"))));
    if (Attributes && ConsumedComboPoints > 0 && InitiativePerPoint > 0)
    {
        Attributes->GainInitiative(ConsumedComboPoints * InitiativePerPoint);
    }

    NotifyRoundStateChanged();
    return ConsumedComboPoints;
}

void ARoundCharacter3::TryGainComboPoints(int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }

    const bool bWasAtCap = NativeComboPoints >= GetMaxComboPoints();
    NativeComboPoints = FMath::Clamp(NativeComboPoints + Amount, 0, GetMaxComboPoints());

    if (bWasAtCap && !bComboOverflowInitiativeUsedThisTurn && Attributes)
    {
        const int32 OverflowInitiative = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("ComboOverflowInitiativeGain"))));
        if (OverflowInitiative > 0)
        {
            Attributes->GainInitiative(OverflowInitiative);
            bComboOverflowInitiativeUsedThisTurn = true;
        }
    }

    NotifyRoundStateChanged();
}

void ARoundCharacter3::GrantShadowstrikeFollowup(ARoundCharacter* TargetCharacter, int32 CostReduction)
{
    ShadowstrikeFollowupTarget = TargetCharacter;
    ShadowstrikeFollowupCostReduction = FMath::Max(0, CostReduction);
    ShadowstrikeFollowupTurnsRemaining = TargetCharacter && CostReduction > 0 ? 2 : 0;
}

void ARoundCharacter3::GrantShadowstrikeFreeStep()
{
    bFreeRetreatAvailable = true;
    bFreeRetreatUsedThisTurn = false;
}

void ARoundCharacter3::GrantShadowstrikeDamageReduction(double IncomingDamageMultiplier)
{
    ShadowstrikeIncomingDamageMultiplier = FMath::Clamp(IncomingDamageMultiplier, 0.0, 1.0);
}

void ARoundCharacter3::GrantHookShotBasicAttackCostReduction(int32 CostReduction)
{
    HookShotBasicAttackCostReduction = FMath::Max(HookShotBasicAttackCostReduction, FMath::Max(0, CostReduction));
}

void ARoundCharacter3::BeginTurn()
{
    Super::BeginTurn();
    ++NativeTurnSequence;
    for (ARoundAbilityActor* Ability : GrantedAbilities)
    {
        if (ARoundHookShotAbility* HookShot = Cast<ARoundHookShotAbility>(Ability)) HookShot->OnOwnerTurnStarted();
    }
    ShadowstrikeIncomingDamageMultiplier = 0.0;
    if (ShadowstrikeFollowupTurnsRemaining > 0 && --ShadowstrikeFollowupTurnsRemaining == 0)
    {
        ShadowstrikeFollowupTarget.Reset();
        ShadowstrikeFollowupCostReduction = 0;
    }
    NativeMovedFieldsThisTurn = 0;
    NativeComboSpentThisTurn = 0;
    bOpeningCutUsedThisTurn = false;
    bFirstBasicAttackUsedThisTurn = false;
    bComboOverflowInitiativeUsedThisTurn = false;
    bFreeRetreatAvailable = false;
    bFreeRetreatUsedThisTurn = false;
    bDamageAfterMoveReady = false;
    bDamageReductionAfterMoveReady = false;
    HookShotBasicAttackCostReduction = 0;

    if (NativeComboPoints == 0 && GetPassiveValueFromKey(TEXT("ComboOnEmptyTurnStart")) > 0.0)
    {
        TryGainComboPoints(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("ComboOnEmptyTurnStart")))));
    }
}

void ARoundCharacter3::AdvanceGlobalInitiative(int32 Amount)
{
    Super::AdvanceGlobalInitiative(Amount);
    HuntMarkCooldownInitiativeRemaining = FMath::Max(0, HuntMarkCooldownInitiativeRemaining - FMath::Max(0, Amount));
}

void ARoundCharacter3::EndTurn()
{
    for (ARoundAbilityActor* Ability : GrantedAbilities)
    {
        if (ARoundHookShotAbility* HookShot = Cast<ARoundHookShotAbility>(Ability)) HookShot->OnOwnerTurnEnded();
    }
    Super::EndTurn();
}

FRoundActionResult ARoundCharacter3::MoveAlongPath(const TArray<ARoundGridTile*>& Path)
{
    const int32 InitiativeBefore = Attributes ? Attributes->CurrentInitiative : 0;
    FRoundActionResult Result = Super::MoveAlongPath(Path);
    if (!Result.bSuccess)
    {
        return Result;
    }

    NativeMovedFieldsThisTurn += Path.Num();
    const int32 RequiredDistance = FMath::Max(1, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("MoveDistanceForBonuses"))));
    if (NativeMovedFieldsThisTurn >= RequiredDistance)
    {
        bDamageAfterMoveReady |= GetPassiveValueFromKey(TEXT("NextHitDamageMultiplierAfterMove")) > 0.0;
        bDamageReductionAfterMoveReady |= GetPassiveValueFromKey(TEXT("NextIncomingDamageMultiplierAfterMove")) > 0.0;
    }

    if (Path.Num() == 1 && bFreeRetreatAvailable && !bFreeRetreatUsedThisTurn && Attributes)
    {
        Attributes->GainInitiative(FMath::Max(0, InitiativeBefore - Attributes->CurrentInitiative));
        bFreeRetreatAvailable = false;
        bFreeRetreatUsedThisTurn = true;
    }

    return Result;
}

FRoundActionResult ARoundCharacter3::AttackField(ARoundGridTile* TargetTile)
{
    PendingBasicAttackTarget = TargetTile ? TargetTile->OccupyingCharacter : nullptr;
    FRoundActionResult Result = Super::AttackField(TargetTile);
    PendingBasicAttackTarget.Reset();
    if (Result.bSuccess)
    {
        bFirstBasicAttackUsedThisTurn = true;
        HookShotBasicAttackCostReduction = 0;
        if (TargetTile && TargetTile->OccupyingCharacter == ShadowstrikeFollowupTarget.Get())
        {
            ShadowstrikeFollowupTarget.Reset();
            ShadowstrikeFollowupCostReduction = 0;
            ShadowstrikeFollowupTurnsRemaining = 0;
        }
    }
    return Result;
}

int32 ARoundCharacter3::GetAttackCost() const
{
    int32 Cost = Super::GetAttackCost();
    if (!bFirstBasicAttackUsedThisTurn)
    {
        Cost -= FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("FirstBasicAttackCostReduction"))));
    }
    if (PendingBasicAttackTarget.IsValid() && PendingBasicAttackTarget == ShadowstrikeFollowupTarget)
    {
        Cost -= ShadowstrikeFollowupCostReduction;
    }
    Cost -= HookShotBasicAttackCostReduction;
    return FMath::Max(1, Cost);
}

int32 ARoundCharacter3::ApplyRoundDamage(int32 Amount, ERoundDamageType DamageType, int32 DefenseIgnore, ARoundCharacter* SourceCharacter, bool& bDidDie)
{
    int32 ModifiedAmount = Amount;
    const double MoveReductionMultiplier = GetPassiveValueFromKey(TEXT("NextIncomingDamageMultiplierAfterMove"));
    const double ReductionMultiplier = ShadowstrikeIncomingDamageMultiplier > 0.0
        ? ShadowstrikeIncomingDamageMultiplier
        : MoveReductionMultiplier;
    if ((bDamageReductionAfterMoveReady || ShadowstrikeIncomingDamageMultiplier > 0.0) && ReductionMultiplier > 0.0)
    {
        ModifiedAmount = FMath::RoundToInt(static_cast<double>(ModifiedAmount) * ReductionMultiplier);
    }

    const int32 AppliedDamage = Super::ApplyRoundDamage(ModifiedAmount, DamageType, DefenseIgnore, SourceCharacter, bDidDie);
    if (bDamageReductionAfterMoveReady && AppliedDamage > 0)
    {
        bDamageReductionAfterMoveReady = false;
    }
    if (ShadowstrikeIncomingDamageMultiplier > 0.0 && AppliedDamage > 0)
    {
        ShadowstrikeIncomingDamageMultiplier = 0.0;
    }
    return AppliedDamage;
}

int32 ARoundCharacter3::ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const
{
    int32 ModifiedDamage = Super::ModifyOutgoingDamage(Amount, DamageType, TargetCharacter, DefenseIgnore);
    const double Multiplier = GetPassiveValueFromKey(TEXT("NextHitDamageMultiplierAfterMove"));
    if (bDamageAfterMoveReady && Multiplier > 0.0)
    {
        ModifiedDamage = FMath::RoundToInt(static_cast<double>(ModifiedDamage) * Multiplier);
    }
    return FMath::Max(0, ModifiedDamage);
}

bool ARoundCharacter3::HasInvisibilityEffect() const
{
    for (ARoundEffectActor* Effect : ActiveEffects)
    {
        if (Effect && Effect->IsA<ARoundInvisibilityEffect>())
        {
            return true;
        }
    }

    return false;
}

void ARoundCharacter3::EnsureStartInvisibility()
{
    if (!HasAuthority() || bNativeStartInvisibilityApplied || HasInvisibilityEffect() || !GetWorld())
    {
        return;
    }

    const int32 DurationInitiative = FMath::Max(0, FMath::RoundToInt(GetValueFromKey(TEXT("StartInvisibilityDurationInitiative"))));
    if (DurationInitiative <= 0)
    {
        return;
    }

    ARoundInvisibilityEffect* InvisibilityEffect = GetWorld()->SpawnActor<ARoundInvisibilityEffect>(ARoundInvisibilityEffect::StaticClass());
    if (!InvisibilityEffect)
    {
        return;
    }

    InvisibilityEffect->EffectId = TEXT("Character3StartInvisibility");
    InvisibilityEffect->DisplayName = TEXT("Invisibility");
    InvisibilityEffect->Description = TEXT("Startet den Kampf unsichtbar.");
    InvisibilityEffect->SetDurationInitiative(DurationInitiative, false);
    InvisibilityEffect->CauserCharacter = this;
    AddEffect(InvisibilityEffect);
    bNativeStartInvisibilityApplied = true;
}

void ARoundCharacter3::HandleDamageDealt(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType)
{
    if (AppliedDamage <= 0) return;

    const TArray<TObjectPtr<ARoundEffectActor>> EffectsBeforeDamageRewards = ActiveEffects;
    for (ARoundEffectActor* Effect : EffectsBeforeDamageRewards)
    {
        if (Effect && Effect->IsA<ARoundInvisibilityEffect>()) Effect->RemoveEffect();
    }

    bDamageAfterMoveReady = false;
    if (!bFreeRetreatUsedThisTurn && GetPassiveValueFromKey(TEXT("FreeAdjacentMoveAfterDamage")) > 0.0)
    {
        bFreeRetreatAvailable = true;
    }

    if (EnemyCharacter && !bOpeningCutUsedThisTurn && EnemyCharacter->Attributes
        && EnemyCharacter->Attributes->CurrentHealth + AppliedDamage >= EnemyCharacter->Attributes->BaseStats.MaxHealth
        && GetPassiveValueFromKey(TEXT("ComboOnFullHealthHit")) > 0.0)
    {
        TryGainComboPoints(FMath::RoundToInt(GetPassiveValueFromKey(TEXT("ComboOnFullHealthHit"))));
        bOpeningCutUsedThisTurn = true;
    }

    const bool bKilled = EnemyCharacter && !EnemyCharacter->IsAlive();
    if (bKilled)
    {
        for (ARoundAbilityActor* Ability : GrantedAbilities)
        {
            if (ARoundHookShotAbility* HookShot = Cast<ARoundHookShotAbility>(Ability)) HookShot->NotifyOwnerKilledEnemy();
        }
        const int32 ComboOnKill = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("ComboOnKill"))));
        if (ComboOnKill > 0) TryGainComboPoints(ComboOnKill);

        const int32 InvisibilityDuration = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InvisibilityOnKillDurationInitiative"))));
        if (InvisibilityDuration > 0 && GetWorld())
        {
            ARoundInvisibilityEffect* InvisibilityEffect = GetWorld()->SpawnActor<ARoundInvisibilityEffect>();
            if (InvisibilityEffect)
            {
                InvisibilityEffect->EffectId = TEXT("Character3KillInvisibility");
                InvisibilityEffect->DisplayName = TEXT("Spurlos");
                InvisibilityEffect->Description = TEXT("Unsichtbar bis Character 3 Schaden verursacht.");
                InvisibilityEffect->SetDurationInitiative(InvisibilityDuration, false);
                InvisibilityEffect->CauserCharacter = this;
                AddEffect(InvisibilityEffect);
            }
        }
    }
    else if (EnemyCharacter && HuntMarkCooldownInitiativeRemaining <= 0)
    {
        const int32 MarkDuration = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("HuntMarkDurationInitiative"))));
        const double MarkMultiplier = GetPassiveValueFromKey(TEXT("HuntMarkDamageMultiplier"));
        if (MarkDuration > 0 && MarkMultiplier > 0.0 && GetWorld())
        {
            ARoundAssassinHuntMarkEffect* Mark = GetWorld()->SpawnActor<ARoundAssassinHuntMarkEffect>();
            if (Mark)
            {
                Mark->EffectId = TEXT("Character3HuntMark");
                Mark->DisplayName = TEXT("Gejagte Beute");
                Mark->Description = TEXT("Erleidet erhoehten Schaden von Character 3.");
                Mark->DamageMultiplierFromCauser = MarkMultiplier;
                Mark->SetDurationInitiative(MarkDuration, false);
                Mark->CauserCharacter = this;
                EnemyCharacter->AddEffect(Mark);
                HuntMarkCooldownInitiativeRemaining = MarkDuration + FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("HuntMarkCooldownAfterExpiryInitiative"))));
            }
        }
    }

}
