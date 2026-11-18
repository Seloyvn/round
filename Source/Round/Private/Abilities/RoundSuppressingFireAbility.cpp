#include "Abilities/RoundSuppressingFireAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter2.h"
#include "Actors/RoundEffectActor.h"
#include "Actors/RoundGridTile.h"
#include "Effects/RoundSuppressingFireEffect.h"

FRoundActionResult ARoundSuppressingFireAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacter2* Sniper = Cast<ARoundCharacter2>(OwningCharacter);
    ARoundCharacter* Target = TargetTile ? TargetTile->OccupyingCharacter : nullptr;
    if (!Sniper || !Target || !OwningCharacter->IsEnemyFor(Target))
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    bool bTargetAlreadyDebuffed = false;
    for (const ARoundEffectActor* Effect : Target->ActiveEffects)
    {
        if (Effect && Effect->IsDebuff()) { bTargetAlreadyDebuffed = true; break; }
    }
    const bool bFreeAmmo = bTargetAlreadyDebuffed && GetAbilityValueFromKey(TEXT("FreeAmmoVsDebuffed")) > 0.0;
    if (!bFreeAmmo && !Sniper->HasAmmo())
    {
        Result.Reason = TEXT("No ammo.");
        return Result;
    }
    if (!ConsumeAbilityActivation(TargetTile, Result)) return Result;

    bool bKilled = false;
    ApplyAbilityDamageToTarget(Target, GetAbilityDamage(), FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore")))), bKilled);

    if (!bKilled)
    {
        if (ARoundSuppressingFireEffect* Effect = GetWorld()->SpawnActor<ARoundSuppressingFireEffect>())
        {
            Effect->EffectId = TEXT("SuppressingFire");
            Effect->DisplayName = TEXT("Suppressing Fire");
            Effect->Description = TEXT("Erhoeht Bewegungskosten und modifiziert Interaktionen mit dem unterdrueckten Ziel.");
            Effect->CauserCharacter = Sniper;
            Effect->FlatMoveCostAdd = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MoveCostAddOnTarget"))));
            Effect->BasicAttackCostAdd = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BasicAttackCostAdd"))));
            Effect->bRevealToAllTeams = GetAbilityValueFromKey(TEXT("RevealToAllTeams")) > 0.0;
            Effect->AllyHitDurationExtension = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AllyHitDurationExtension"))));
            Effect->DefenseReduction = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseReduction"))));
            Effect->bGrantFreeCauserAttackAfterAllyHit = GetAbilityValueFromKey(TEXT("FreeFollowupAfterAllyHit")) > 0.0;
            Effect->CooldownReductionOnMove = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionOnMove"))));
            Effect->AmmoOnKill = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AmmoOnKill"))));
            Effect->bChainOnKill = GetAbilityValueFromKey(TEXT("ChainOnKill")) > 0.0;
            Effect->ChainRange = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ChainRange"))));
            Effect->ReactionDamageMultiplier = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("ReactionDamageMultiplier")));
            Effect->bRootUntilDamaged = GetAbilityValueFromKey(TEXT("RootUntilDamaged")) > 0.0;
            Effect->SetDurationInitiative(40, false);
            Target->AddEffect(Effect);
        }
    }

    if (!bFreeAmmo) Sniper->ResolveAmmoShot(bKilled);
    Sniper->ResolveConcentrateHit(Target, this, bKilled, false);
    Result.bSuccess = true;
    Result.Reason = TEXT("Suppressing Fire executed.");
    return Result;
}
