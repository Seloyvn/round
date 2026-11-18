#include "Effects/RoundDeathMarkEffect.h"

#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter3.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundInvisibilityEffect.h"
#include "EngineUtils.h"

int32 ARoundDeathMarkEffect::ModifyIncomingDamage(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType, int32& DefenseIgnore) const
{
    if (SourceCharacter != CauserCharacter)
    {
        return Damage;
    }

    DefenseIgnore += FMath::Max(0, DefenseIgnoreFromCauser);
    double Multiplier = DamageMultiplierFromCauser;
    if (TargetCharacter && TargetCharacter->Attributes && LowHealthThreshold > 0.0
        && TargetCharacter->Attributes->CurrentHealth <= FMath::RoundToInt(TargetCharacter->Attributes->BaseStats.MaxHealth * LowHealthThreshold))
    {
        Multiplier += LowHealthDamageMultiplierAdd;
    }
    return FMath::Max(0, FMath::RoundToInt(static_cast<double>(Damage) * FMath::Max(0.0, Multiplier)));
}

void ARoundDeathMarkEffect::OnDamageTaken(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 AppliedDamage, ERoundDamageType)
{
    ARoundCharacter3* Assassin = Cast<ARoundCharacter3>(CauserCharacter);
    if (!Assassin || AppliedDamage <= 0 || !SourceCharacter)
    {
        return;
    }

    const bool bEligibleSource = SourceCharacter == Assassin
        || (bComboCanTriggerFromAllies && !Assassin->IsEnemyFor(SourceCharacter));
    const int32 TurnSequence = Assassin->GetNativeTurnSequence();
    const bool bCanGrantRepeatedCombo = bGrantComboEachCauserTurn
        && SourceCharacter == Assassin
        && LastComboGrantTurnSequence != TurnSequence;
    if (bEligibleSource && (!bInitialComboGranted || bCanGrantRepeatedCombo))
    {
        const int32 ComboBefore = Assassin->TryGetComboPoints();
        Assassin->TryGainComboPoints(FMath::Max(0, ComboGainOnTrigger));
        bInitialComboGranted = true;
        LastComboGrantTurnSequence = TurnSequence;
        if (ComboBefore < Assassin->GetMaxComboPoints() && Assassin->TryGetComboPoints() >= Assassin->GetMaxComboPoints())
        {
            ReduceAbilityCooldown(TEXT("Execute"), ExecuteCooldownReductionOnMaxCombo);
        }
    }

    if (SourceCharacter == Assassin && TargetCharacter && !TargetCharacter->IsAlive() && TransferRangeOnKill > 0 && !bTransferred)
    {
        TransferToNearestEnemy(TargetCharacter);
    }
}

void ARoundDeathMarkEffect::OnOwnerMoved(ARoundCharacter*, ARoundGridTile*, ARoundGridTile*)
{
    if (!bMoveExtensionUsed && MoveDurationExtension > 0)
    {
        RemainingInitiative += MoveDurationExtension;
        bMoveExtensionUsed = true;
    }
}

void ARoundDeathMarkEffect::OnInitiativeAdvanced(ARoundCharacter* TargetCharacter, int32 InitiativeAmount)
{
    const bool bWillExpire = RemainingInitiative > 0 && InitiativeAmount >= RemainingInitiative;
    if (bWillExpire && TargetCharacter && TargetCharacter->IsAlive())
    {
        ReduceAbilityCooldown(TEXT("DeathMark"), DeathMarkCooldownReductionOnExpiry);
    }
    Super::OnInitiativeAdvanced(TargetCharacter, InitiativeAmount);
}

bool ARoundDeathMarkEffect::PreventsEffectApplication(const ARoundEffectActor* IncomingEffect) const
{
    return bPreventInvisibility && IncomingEffect && IncomingEffect->IsA<ARoundInvisibilityEffect>();
}

void ARoundDeathMarkEffect::ReduceAbilityCooldown(FName AbilityId, int32 ReductionAmount) const
{
    if (!CauserCharacter || ReductionAmount <= 0) return;
    for (ARoundAbilityActor* Ability : CauserCharacter->GrantedAbilities)
    {
        if (Ability && Ability->AbilityData && Ability->AbilityData->AbilityId == AbilityId)
        {
            Ability->ReduceCooldown(ReductionAmount);
            return;
        }
    }
}

void ARoundDeathMarkEffect::TransferToNearestEnemy(ARoundCharacter* DeadTarget)
{
    if (!DeadTarget || !DeadTarget->CurrentTile || !CauserCharacter || !GetWorld()) return;

    ARoundCharacter* NearestEnemy = nullptr;
    int32 NearestDistance = MAX_int32;
    for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
    {
        ARoundCharacter* Candidate = *It;
        if (!Candidate || Candidate == DeadTarget || !Candidate->IsAlive() || !Candidate->CurrentTile || !CauserCharacter->IsEnemyFor(Candidate)) continue;
        const int32 Distance = DeadTarget->CurrentTile->GetDistance(Candidate->CurrentTile);
        if (Distance <= TransferRangeOnKill && Distance < NearestDistance)
        {
            NearestEnemy = Candidate;
            NearestDistance = Distance;
        }
    }

    if (!NearestEnemy) return;
    ARoundDeathMarkEffect* NewMark = GetWorld()->SpawnActor<ARoundDeathMarkEffect>();
    if (!NewMark) return;
    NewMark->EffectId = EffectId;
    NewMark->DisplayName = DisplayName;
    NewMark->Description = Description;
    NewMark->CauserCharacter = CauserCharacter;
    NewMark->ComboGainOnTrigger = ComboGainOnTrigger;
    NewMark->DamageMultiplierFromCauser = DamageMultiplierFromCauser;
    NewMark->DefenseIgnoreFromCauser = DefenseIgnoreFromCauser;
    NewMark->MoveDurationExtension = MoveDurationExtension;
    NewMark->ExecuteCooldownReductionOnMaxCombo = ExecuteCooldownReductionOnMaxCombo;
    NewMark->DeathMarkCooldownReductionOnExpiry = DeathMarkCooldownReductionOnExpiry;
    NewMark->TransferRangeOnKill = TransferRangeOnKill;
    NewMark->LowHealthThreshold = LowHealthThreshold;
    NewMark->LowHealthDamageMultiplierAdd = LowHealthDamageMultiplierAdd;
    NewMark->bComboCanTriggerFromAllies = bComboCanTriggerFromAllies;
    NewMark->bPreventInvisibility = bPreventInvisibility;
    NewMark->bGrantComboEachCauserTurn = bGrantComboEachCauserTurn;
    NewMark->bTransferred = true;
    NewMark->bInitialComboGranted = bInitialComboGranted;
    NewMark->LastComboGrantTurnSequence = LastComboGrantTurnSequence;
    NewMark->SetDurationInitiative(FMath::Max(1, RemainingInitiative), false);
    NearestEnemy->AddEffect(NewMark);
}
