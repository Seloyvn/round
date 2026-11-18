#include "Effects/RoundConcentrateEffect.h"
#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter2.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundModifierEffect.h"
#include "Game/RoundGameState.h"

int32 ARoundConcentrateEffect::ModifyOutgoingDamage(ARoundCharacter*, ARoundCharacter*, int32 Damage, ERoundDamageType, int32&) const
{
    if (bHitPendingResolution || Damage <= 0) return Damage;
    auto* Mutable = const_cast<ARoundConcentrateEffect*>(this);
    Mutable->bHitPendingResolution = true;
    Mutable->LastEmpoweredDamage = FMath::Max(0, FMath::RoundToInt(Damage * Amount));
    return Mutable->LastEmpoweredDamage;
}

void ARoundConcentrateEffect::OnInitiativeAdvanced(ARoundCharacter* Target, int32 Initiative)
{
    Super::OnInitiativeAdvanced(Target, Initiative);
    if (!IsValid(this) || Initiative <= 0) return;
    if (PendingAmmoReload > 0)
    {
        AmmoReloadElapsed += Initiative;
        if (AmmoReloadElapsed >= AmmoReloadDelay)
        {
            if (ARoundCharacter2* Character2 = Cast<ARoundCharacter2>(Target)) Character2->RegenerateAmmo(PendingAmmoReload);
            PendingAmmoReload = 0;
        }
    }
    if (ChargeBonusPerInterval <= 0.0) return;
    if (InitialAmount <= 0.0) InitialAmount = Amount;
    ChargeElapsed += Initiative;
    Amount = InitialAmount + FMath::Min(MaxChargeBonus, (ChargeElapsed / FMath::Max(1, ChargeInterval)) * ChargeBonusPerInterval);
}

void ARoundConcentrateEffect::OnOwnerMoved(ARoundCharacter*, ARoundGridTile* Previous, ARoundGridTile* Next)
{
    if (bExpireOnMove && Previous != Next) RemoveEffect();
}

void ARoundConcentrateEffect::OnDamageTaken(ARoundCharacter*, ARoundCharacter*, int32 Damage, ERoundDamageType)
{
    if (bExpireOnDamage && Damage > 0) RemoveEffect();
}

void ARoundConcentrateEffect::ResolveEmpoweredHit(ARoundCharacter2* Source, ARoundCharacter* Target, ARoundAbilityActor* Ability, bool bKilled, bool bBasic)
{
    if (!bHitPendingResolution || !Source) return;
    if (Target && !bKilled && DefenseReductionOnHit > 0)
    {
        if (auto* Debuff = GetWorld()->SpawnActor<ARoundModifierEffect>())
        {
            Debuff->EffectId = TEXT("ConcentrateDefenseBreak");
            Debuff->DisplayName = TEXT("Panzerbruch");
            Debuff->DefenseIgnoreAdd = DefenseReductionOnHit;
            Debuff->bTreatAsDebuff = true;
            Debuff->CauserCharacter = Source;
            Debuff->SetDurationInitiative(DefenseReductionDuration, false);
            Target->AddEffect(Debuff);
        }
    }
    if (Target && !bKilled && CurrentInitiativeLossFraction > 0.0 && Target->Attributes)
        Target->ApplyDelay(FMath::RoundToInt(Target->Attributes->CurrentInitiative * CurrentInitiativeLossFraction));
    if (bGrantFreeStepAfterHit) Source->bHasFreeConcentrateStep = true;
    if (Ability && AbilityCooldownReductionOnConsume > 0) Ability->ReduceCooldown(AbilityCooldownReductionOnConsume);
    if (SplashDamageFraction > 0.0 && Target && Target->CurrentTile)
    {
        if (auto* State = Source->GetRoundGameState()) for (ARoundGridTile* Tile : State->GetNeighborFields(Target->CurrentTile))
        {
            ARoundCharacter* Other = Tile ? Tile->OccupyingCharacter : nullptr;
            if (Other && Source->IsEnemyFor(Other)) { bool bDied = false; Other->ApplyRoundDamage(FMath::RoundToInt(LastEmpoweredDamage * SplashDamageFraction), ERoundDamageType::Physical, 0, Source, bDied); }
        }
    }
    if (bPersistOnKill && bKilled) { bHitPendingResolution = false; return; }
    if (--RemainingEmpoweredHits > 0) { Amount = 1.0 + (Amount - 1.0) * SecondHitBonusFraction; bHitPendingResolution = false; return; }
    const bool bVolley = bDumpMagazineAfterBasic && bBasic;
    RemoveEffect();
    if (bVolley && Target) Source->FireConcentrateMagazineVolley(Target);
}
