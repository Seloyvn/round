#include "Abilities/RoundDashAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter5.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundBloodMarkEffect.h"
#include "Effects/RoundModifierEffect.h"
#include "Game/RoundGameState.h"

FRoundActionResult ARoundDashAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 26)
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    FRoundActionResult Result;
    if (!CanTargetTile(TargetTile) || !OwningCharacter || !OwningCharacter->Attributes || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState();
    if (!TargetCharacter || !RoundGameState)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundGridTile* StartTile = OwningCharacter->CurrentTile;
    ARoundGridTile* DestinationTile = RoundGameState->GetCloseFreeFieldFrom(TargetTile, StartTile);
    if (!DestinationTile)
    {
        Result.Reason = TEXT("No free tile near target.");
        return Result;
    }

    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    SetCooldown(GetAbilityCooldown());
    OwningCharacter->SetField(DestinationTile, true);

    bool bDidDie = false;
    int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    int32 Damage = GetAbilityDamage();
    const int32 TravelDistance = StartTile ? StartTile->GetDistance(TargetTile) : 0;
    if (TravelDistance >= FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MomentumRequiredDistance")))))
    {
        Damage += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MomentumDamage"))));
    }
    if (TargetCharacter->Attributes && TargetCharacter->Attributes->BaseStats.MaxHealth > 0)
    {
        const double HealthFraction = static_cast<double>(TargetCharacter->Attributes->CurrentHealth) / TargetCharacter->Attributes->BaseStats.MaxHealth;
        const double Threshold = GetAbilityValueFromKey(TEXT("LowHealthThreshold"));
        if (Threshold > 0.0 && HealthFraction <= Threshold)
        {
            const double Multiplier = GetAbilityValueFromKey(TEXT("DamageMultiplierVsLowHealth"));
            if (Multiplier > 0.0) Damage = FMath::RoundToInt(Damage * Multiplier);
        }
    }
    const int32 FinalDamage = OwningCharacter->ModifyOutgoingDamage(Damage, GetAbilityDamageType(), TargetCharacter, DefenseIgnore);
    const int32 AppliedDamage = TargetCharacter->ApplyRoundDamage(FinalDamage, GetAbilityDamageType(), DefenseIgnore, OwningCharacter, bDidDie);

    if (ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(OwningCharacter))
    {
        const double BonusBloodFraction = GetAbilityValueFromKey(TEXT("BonusBloodFractionOfDamage"));
        if (BonusBloodFraction > 0.0) Character5->GainRawBlood(FMath::RoundToInt(AppliedDamage * BonusBloodFraction));
    }

    const int32 EffectDuration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("EffectDurationInitiative"))));
    const double GuardMultiplier = GetAbilityValueFromKey(TEXT("SelfIncomingDamageMultiplierAfterUse"));
    if (GuardMultiplier > 0.0 && EffectDuration > 0)
    {
        if (ARoundModifierEffect* Guard = GetWorld()->SpawnActor<ARoundModifierEffect>())
        {
            Guard->DisplayName = TEXT("Mitten rein"); Guard->IncomingDamageMultiplier = GuardMultiplier; Guard->bTreatAsBuff = true;
            Guard->SetDurationInitiative(EffectDuration, false); OwningCharacter->AddEffect(Guard);
        }
    }
    const int32 MoveCostAdd = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MoveCostAddOnTarget")));
    if (MoveCostAdd > 0 && EffectDuration > 0)
    {
        if (ARoundModifierEffect* Cripple = GetWorld()->SpawnActor<ARoundModifierEffect>())
        {
            Cripple->DisplayName = TEXT("Festgenagelt"); Cripple->MoveCostAdd = MoveCostAdd; Cripple->bTreatAsDebuff = true;
            Cripple->CauserCharacter = OwningCharacter; Cripple->SetDurationInitiative(EffectDuration, false); TargetCharacter->AddEffect(Cripple);
        }
    }

    const double SplashMultiplier = GetAbilityValueFromKey(TEXT("ImpactSplashDamageMultiplier"));
    if (SplashMultiplier > 0.0)
    {
        for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(TargetTile))
        {
            ARoundCharacter* Enemy = Neighbor ? Neighbor->OccupyingCharacter : nullptr;
            if (!Enemy || Enemy == TargetCharacter || !OwningCharacter->IsEnemyFor(Enemy)) continue;
            bool bSplashDied = false; int32 SplashIgnore = DefenseIgnore;
            const int32 SplashDamage = OwningCharacter->ModifyOutgoingDamage(FMath::RoundToInt(GetAbilityDamage() * SplashMultiplier), GetAbilityDamageType(), Enemy, SplashIgnore);
            Enemy->ApplyRoundDamage(SplashDamage, GetAbilityDamageType(), SplashIgnore, OwningCharacter, bSplashDied);
        }
    }

    const int32 Refund = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeRefundVsLowHealth"))));
    const double RefundThreshold = GetAbilityValueFromKey(TEXT("InitiativeRefundHealthThreshold"));
    if (Refund > 0 && RefundThreshold > 0.0 && TargetCharacter->Attributes &&
        static_cast<double>(TargetCharacter->Attributes->CurrentHealth) / TargetCharacter->Attributes->BaseStats.MaxHealth <= RefundThreshold)
    {
        OwningCharacter->Attributes->GainInitiative(Refund);
    }

    const double MarkMultiplier = GetAbilityValueFromKey(TEXT("NextHitDamageMultiplier"));
    if (!bDidDie && MarkMultiplier > 0.0)
    {
        if (ARoundBloodMarkEffect* Mark = GetWorld()->SpawnActor<ARoundBloodMarkEffect>())
        {
            Mark->DisplayName = TEXT("Zerreissen"); Mark->DamageMultiplier = MarkMultiplier; Mark->CauserCharacter = OwningCharacter;
            Mark->SetDurationInitiative(FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MarkDurationInitiative")))), false); TargetCharacter->AddEffect(Mark);
        }
    }

    if (bDidDie)
    {
        ReduceCooldown(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionOnKill")))));
    }

    if (GetAbilityValueFromKey(TEXT("CarryThrough")) > 0.0 && !bDidDie)
    {
        ARoundGridTile* CarryTile = nullptr;
        int32 BestDistance = TargetTile->GetDistance(StartTile);
        for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(TargetTile))
        {
            if (Neighbor && !Neighbor->OccupyingCharacter && Neighbor->GetDistance(StartTile) > BestDistance)
            {
                CarryTile = Neighbor; BestDistance = Neighbor->GetDistance(StartTile);
            }
        }
        if (CarryTile) OwningCharacter->SetField(CarryTile, true);
    }
    Result.bSuccess = true;
    Result.Reason = TEXT("Dash executed.");
    return Result;
}
