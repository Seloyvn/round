#include "Effects/RoundSuppressingFireEffect.h"

#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter2.h"
#include "Actors/RoundGridTile.h"
#include "Data/RoundAbilityData.h"
#include "Game/RoundGameState.h"

int32 ARoundSuppressingFireEffect::GetMovecost_Implementation(int32 In) const
{
    return FMath::Max(1, Super::GetMovecost_Implementation(In) + FlatMoveCostAdd);
}

bool ARoundSuppressingFireEffect::SetHasVision_Implementation(ARoundCharacter*, bool bCurrentVision) const
{
    return bRevealToAllTeams || bCurrentVision;
}

bool ARoundSuppressingFireEffect::PreventsAbilityUse(const ARoundAbilityActor* Ability) const
{
    if (!bRevealToAllTeams || !Ability || !Ability->AbilityData) return false;
    return Ability->AbilityData->AbilityId.ToString().Contains(TEXT("Invisible"), ESearchCase::IgnoreCase)
        || Ability->AbilityData->AbilityId.ToString().Contains(TEXT("Invisibility"), ESearchCase::IgnoreCase);
}

bool ARoundSuppressingFireEffect::PreventsMovement() const
{
    return bRootUntilDamaged && !bRootBroken;
}

int32 ARoundSuppressingFireEffect::ModifyIncomingDamage(ARoundCharacter*, ARoundCharacter*, int32 Damage, ERoundDamageType, int32& DefenseIgnore) const
{
    DefenseIgnore += FMath::Max(0, DefenseReduction);
    return Damage;
}

int32 ARoundSuppressingFireEffect::ModifyBasicAttackCost(ARoundCharacter*, int32 AttackCost) const
{
    return AttackCost + (bBasicAttackTaxAvailable ? FMath::Max(0, BasicAttackCostAdd) : 0);
}

void ARoundSuppressingFireEffect::OnBasicAttackExecuted(ARoundCharacter*, ARoundCharacter*)
{
    bBasicAttackTaxAvailable = false;
}

void ARoundSuppressingFireEffect::OnOwnerMoved(ARoundCharacter* TargetCharacter, ARoundGridTile* PreviousTile, ARoundGridTile* NewTile)
{
    Super::OnOwnerMoved(TargetCharacter, PreviousTile, NewTile);
    if (bMovementReactionUsed || !TargetCharacter || !PreviousTile || !NewTile || PreviousTile == NewTile) return;
    bMovementReactionUsed = true;
    if (ARoundCharacter2* Sniper = Cast<ARoundCharacter2>(CauserCharacter))
    {
        if (CooldownReductionOnMove > 0)
        {
            for (ARoundAbilityActor* Ability : Sniper->GrantedAbilities)
            {
                if (Ability && Ability->AbilityData && Ability->AbilityData->AbilityId == FName(TEXT("SuppressingFire"))) Ability->ReduceCooldown(CooldownReductionOnMove);
            }
        }
        if (ReactionDamageMultiplier > 0.0) Sniper->FireSuppressionReaction(TargetCharacter, ReactionDamageMultiplier);
    }
}

void ARoundSuppressingFireEffect::OnDamageTaken(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 AppliedDamage, ERoundDamageType)
{
    if (!TargetCharacter || AppliedDamage <= 0) return;
    if (bRootUntilDamaged) bRootBroken = true;

    ARoundCharacter2* Sniper = Cast<ARoundCharacter2>(CauserCharacter);
    const bool bAllyHit = Sniper && SourceCharacter && SourceCharacter != Sniper && SourceCharacter->Team == Sniper->Team;
    if (bAllyHit && !AlliesThatExtendedDuration.Contains(SourceCharacter))
    {
        AlliesThatExtendedDuration.Add(SourceCharacter);
        RemainingInitiative += FMath::Max(0, AllyHitDurationExtension);
        if (bGrantFreeCauserAttackAfterAllyHit) Sniper->GrantFreeSuppressionFollowup(TargetCharacter);
    }

    if (!TargetCharacter->IsAlive() && !bKillResolved)
    {
        bKillResolved = true;
        if (Sniper && AmmoOnKill > 0) Sniper->RegenerateAmmo(AmmoOnKill);
        if (bChainOnKill && Sniper && TargetCharacter->CurrentTile)
        {
            ARoundCharacter* Nearest = nullptr;
            int32 BestDistance = MAX_int32;
            if (ARoundGameState* State = Sniper->GetRoundGameState())
            {
                for (ARoundGridTile* Tile : State->Fields)
                {
                    ARoundCharacter* Candidate = Tile ? Tile->OccupyingCharacter : nullptr;
                    const int32 Distance = Tile ? TargetCharacter->CurrentTile->GetDistance(Tile) : MAX_int32;
                    if (Candidate && Candidate != TargetCharacter && Sniper->IsEnemyFor(Candidate) && Distance <= ChainRange && Distance < BestDistance)
                    {
                        Nearest = Candidate;
                        BestDistance = Distance;
                    }
                }
            }
            if (Nearest)
            {
                ARoundSuppressingFireEffect* Chained = GetWorld()->SpawnActor<ARoundSuppressingFireEffect>();
                if (Chained)
                {
                    Chained->EffectId = EffectId;
                    Chained->DisplayName = DisplayName;
                    Chained->Description = Description;
                    Chained->Amount = Amount;
                    Chained->FlatMoveCostAdd = FlatMoveCostAdd;
                    Chained->CauserCharacter = CauserCharacter;
                    Chained->BasicAttackCostAdd = BasicAttackCostAdd;
                    Chained->bRevealToAllTeams = bRevealToAllTeams;
                    Chained->AllyHitDurationExtension = AllyHitDurationExtension;
                    Chained->DefenseReduction = DefenseReduction;
                    Chained->bGrantFreeCauserAttackAfterAllyHit = bGrantFreeCauserAttackAfterAllyHit;
                    Chained->CooldownReductionOnMove = CooldownReductionOnMove;
                    Chained->AmmoOnKill = AmmoOnKill;
                    Chained->bChainOnKill = bChainOnKill;
                    Chained->ChainRange = ChainRange;
                    Chained->ReactionDamageMultiplier = ReactionDamageMultiplier;
                    Chained->bRootUntilDamaged = bRootUntilDamaged;
                    Chained->SetDurationInitiative(RemainingInitiative, false);
                    Nearest->AddEffect(Chained);
                }
            }
        }
    }
}
