#include "Triggers/RoundAimFieldTriggerActor.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter2.h"
#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundEffectActor.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundGameDataRegistryLookup.h"
#include "Effects/RoundSlowEffect.h"
#include "Effects/RoundModifierEffect.h"
#include "Effects/RoundNextOutgoingDamageEffect.h"
#include "Effects/RoundSnipeMarkEffect.h"
#include "Effects/RoundAimTrackingEffect.h"
#include "Data/RoundAbilityData.h"
#include "Game/RoundGameState.h"
#include "Net/UnrealNetwork.h"

namespace
{
TSubclassOf<AActor> GAimVisualClass = nullptr;
}

ARoundAimFieldTriggerActor::ARoundAimFieldTriggerActor()
{
}

void ARoundAimFieldTriggerActor::InitializeTrigger(ARoundCharacter* InCauserCharacter, const TArray<ARoundGridTile*>& InTriggerTiles)
{
    Super::InitializeTrigger(InCauserCharacter, InTriggerTiles);

    if (!GAimVisualClass)
    {
        GAimVisualClass = RoundGameDataRegistryLookup::ResolveAimVisualActorClass();
    }

    if (!VisualActor && GAimVisualClass && TriggerTiles.Num() > 0 && TriggerTiles[0] && GetWorld())
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.Owner = this;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        VisualActor = GetWorld()->SpawnActor<AActor>(GAimVisualClass, TriggerTiles[0]->GetActorTransform(), SpawnParameters);
    }
}

void ARoundAimFieldTriggerActor::RemoveTrigger()
{
    if (VisualActor)
    {
        VisualActor->Destroy();
        VisualActor = nullptr;
    }

    Super::RemoveTrigger();
}

void ARoundAimFieldTriggerActor::ExecuteTrigger_Implementation(ARoundCharacter* EnteringCharacter, ARoundGridTile* EnteredTile, ARoundGridTile* PreviousTile)
{
    if (!EnteringCharacter || !CauserCharacter)
    {
        return;
    }

    int32 FinalDamage = TriggerDamage;
    if (bBonusIfOwnerStayed && CauserCharacter->CurrentTile == OwnerPlacementTile && StayedDamageMultiplier > 0.0)
    {
        FinalDamage = FMath::RoundToInt(FinalDamage * StayedDamageMultiplier);
    }
    if (bExtraDamageIfTargetMoved)
    {
        FinalDamage = FMath::RoundToInt(static_cast<double>(FinalDamage) * 1.2);
    }

    if (DamageMultiplierVsLowHealth > 1.0 && EnteringCharacter->Attributes)
    {
        const double MaxHealth = EnteringCharacter->Attributes->BaseStats.MaxHealth;
        if (MaxHealth > 0.0 && EnteringCharacter->Attributes->CurrentHealth / MaxHealth <= 0.35)
        {
            FinalDamage = FMath::RoundToInt(static_cast<double>(FinalDamage) * DamageMultiplierVsLowHealth);
        }
    }

    if (FinalDamage > 0)
    {
        bool bDidDie = false;
        int32 ResolvedDefenseIgnore = DefenseIgnore;
        FinalDamage = CauserCharacter->ModifyOutgoingDamage(FinalDamage, DamageType, EnteringCharacter, ResolvedDefenseIgnore);
        EnteringCharacter->ApplyRoundDamage(FinalDamage, DamageType, ResolvedDefenseIgnore, CauserCharacter, bDidDie);

        if (!bDidDie && ExtendDebuffDurationInitiative > 0)
        {
            ARoundEffectActor* Oldest = nullptr;
            int32 LowestDuration = MAX_int32;
            for (ARoundEffectActor* Effect : EnteringCharacter->ActiveEffects)
                if (Effect && Effect->IsDebuff() && Effect->IsRemovable() && Effect->GetRemainingDurationInitiative() < LowestDuration)
                { Oldest = Effect; LowestDuration = Effect->GetRemainingDurationInitiative(); }
            if (Oldest)
            {
                if (Oldest->RemainingInitiative > 0) Oldest->RemainingInitiative += ExtendDebuffDurationInitiative;
                else if (Oldest->RemainingTurns > 0) ++Oldest->RemainingTurns;
            }
        }

        if (bDidDie)
        {
            if (ARoundCharacter2* Character2 = Cast<ARoundCharacter2>(CauserCharacter)) Character2->RegenerateAmmo(AmmoOnKill);
            for (ARoundAbilityActor* Ability : CauserCharacter->GrantedAbilities)
                if (Ability && Ability->AbilityData && Ability->AbilityData->AbilityId == FName(TEXT("Aim"))) Ability->ReduceCooldown(CooldownReductionOnKill);
        }
        else
        {
            if (NextOutgoingDamageMultiplier > 0.0)
            {
                if (auto* Effect = GetWorld()->SpawnActor<ARoundNextOutgoingDamageEffect>())
                { Effect->Amount = NextOutgoingDamageMultiplier; Effect->CauserCharacter = CauserCharacter; Effect->SetDurationInitiative(80, false); EnteringCharacter->AddEffect(Effect); }
            }
            if (MarkDamageMultiplier > 0.0)
            {
                if (auto* Effect = GetWorld()->SpawnActor<ARoundSnipeMarkEffect>())
                { Effect->Amount = MarkDamageMultiplier; Effect->CauserCharacter = CauserCharacter; Effect->SetDurationInitiative(MarkDurationInitiative, false); EnteringCharacter->AddEffect(Effect); }
            }
            if (bCoverFire)
            {
                if (ARoundCharacter2* Character2 = Cast<ARoundCharacter2>(CauserCharacter); Character2 && Character2->HasAmmo() && Character2->Attributes)
                {
                    int32 CoverDamage = FMath::RoundToInt(Character2->Attributes->BaseStats.Attack * 0.5);
                    if (FMath::FRand() < Character2->GetCritChance()) CoverDamage = FMath::RoundToInt(CoverDamage * Character2->GetCritModifier());
                    int32 CoverDefenseIgnore = 0; bool bCoverKilled = false;
                    CoverDamage = Character2->ModifyOutgoingDamage(CoverDamage, ERoundDamageType::Physical, EnteringCharacter, CoverDefenseIgnore);
                    EnteringCharacter->ApplyRoundDamage(CoverDamage, ERoundDamageType::Physical, CoverDefenseIgnore, Character2, bCoverKilled);
                    Character2->ResolveAmmoShot(bCoverKilled);
                }
            }
            if (bApplyTrackingMark)
            {
                if (auto* Effect = GetWorld()->SpawnActor<ARoundAimTrackingEffect>())
                { Effect->TrackingDamage = FMath::RoundToInt(TriggerDamage * 0.5); Effect->DamageType = DamageType; Effect->CauserCharacter = CauserCharacter; Effect->SetDurationInitiative(160, false); EnteringCharacter->AddEffect(Effect); }
            }
        }
    }

    if (TriggerInitiativeLoss > 0)
    {
        EnteringCharacter->ApplyDelay(TriggerInitiativeLoss);
    }

    if (bBlockMovementBonusesOnTrigger && EnteringCharacter->GetWorld())
    {
        if (ARoundSlowEffect* SlowEffect = EnteringCharacter->GetWorld()->SpawnActor<ARoundSlowEffect>(ARoundSlowEffect::StaticClass()))
        {
            SlowEffect->EffectId = TEXT("AimBlockMovement");
            SlowEffect->DisplayName = TEXT("Aim");
            SlowEffect->Description = TEXT("Kann voruebergehend keine Bewegungsboni erhalten.");
            SlowEffect->bBlockMovementBonuses = true;
            SlowEffect->CauserCharacter = CauserCharacter;
            SlowEffect->SetDurationInitiative(40, false);
            EnteringCharacter->AddEffect(SlowEffect);
        }
    }

    if (bOwnerDamageBonusOnTriggeredTarget && EnteringCharacter->GetWorld())
    {
        if (ARoundModifierEffect* ModifierEffect = EnteringCharacter->GetWorld()->SpawnActor<ARoundModifierEffect>(ARoundModifierEffect::StaticClass()))
        {
            ModifierEffect->EffectId = TEXT("AimWindow");
            ModifierEffect->DisplayName = TEXT("Aim");
            ModifierEffect->Description = TEXT("Erleidet voruebergehend mehr Schaden vom Verursacher.");
            ModifierEffect->IncomingDamageMultiplier = 1.15;
            ModifierEffect->bOnlyAffectCauserDamage = true;
            ModifierEffect->CauserCharacter = CauserCharacter;
            ModifierEffect->SetDurationInitiative(40, false);
            EnteringCharacter->AddEffect(ModifierEffect);
        }
    }

    if (AoEInitiativeLoss > 0 && EnteredTile && CauserCharacter->GetRoundGameState())
    {
        for (ARoundGridTile* Neighbor : CauserCharacter->GetRoundGameState()->GetNeighborFields(EnteredTile))
        {
            if (ARoundCharacter* NeighborCharacter = Neighbor ? Neighbor->OccupyingCharacter : nullptr)
            {
                if (CauserCharacter->IsEnemyFor(NeighborCharacter))
                {
                    NeighborCharacter->ApplyDelay(AoEInitiativeLoss);
                }
            }
        }
    }

    if (PersistTriggerCount > 0)
    {
        PersistTriggerCount = FMath::Max(0, PersistTriggerCount - 1);
        TriggerDamage = FMath::RoundToInt(static_cast<double>(TriggerDamage) * 0.5);
        return;
    }

    RemoveTrigger();
}

bool ARoundAimFieldTriggerActor::CanTriggerCharacter_Implementation(ARoundCharacter* EnteringCharacter, ARoundGridTile* EnteredTile, ARoundGridTile* PreviousTile) const
{
    return Super::CanTriggerCharacter_Implementation(EnteringCharacter, EnteredTile, PreviousTile)
        && CauserCharacter
        && EnteringCharacter
        && CauserCharacter->IsEnemyFor(EnteringCharacter);
}

void ARoundAimFieldTriggerActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundAimFieldTriggerActor, TriggerDamage);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, DamageType);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, TriggerInitiativeLoss);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, AoEInitiativeLoss);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, PersistTriggerCount);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, bBlockMovementBonusesOnTrigger);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, bOwnerDamageBonusOnTriggeredTarget);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, bExtraDamageIfTargetMoved);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, DamageMultiplierVsLowHealth);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, DefenseIgnore);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, NextOutgoingDamageMultiplier);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, MarkDamageMultiplier);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, MarkDurationInitiative);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, bBonusIfOwnerStayed);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, StayedDamageMultiplier);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, ExtendDebuffDurationInitiative);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, AmmoOnKill);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, CooldownReductionOnKill);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, bCoverFire);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, bApplyTrackingMark);
    DOREPLIFETIME(ARoundAimFieldTriggerActor, OwnerPlacementTile);
}
