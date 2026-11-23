#include "Abilities/RoundIcebeamAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundSlowEffect.h"
#include "Effects/RoundIcebeamEffect.h"
#include "Effects/RoundModifierEffect.h"
#include "Effects/RoundRootEffect.h"
#include "Game/RoundGameState.h"

FRoundActionResult ARoundIcebeamAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    if (FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CoreOperation"))) == 22)
        return ARoundAbilityActor::ExecuteAbility(TargetTile);
    ARoundCharacter* InitialTarget = ResolveTargetCharacter(TargetTile);
    bool bHadIcebeamSlow = false;
    if (InitialTarget)
    {
        for (ARoundEffectActor* Effect : InitialTarget->ActiveEffects)
        {
            if (Cast<ARoundIcebeamEffect>(Effect) && Effect->CauserCharacter == OwningCharacter)
            {
                bHadIcebeamSlow = true;
                break;
            }
        }
    }

    FRoundActionResult Result = ExecuteDefaultAbility(TargetTile);
    if (!Result.bSuccess || !OwningCharacter || !TargetTile)
    {
        return Result;
    }

    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    ARoundGameState* RoundGameState = OwningCharacter->GetRoundGameState();
    if (!TargetCharacter || !RoundGameState)
    {
        return Result;
    }

    TArray<ARoundEffectActor*> PreviousIcebeamEffects;
    for (ARoundEffectActor* Effect : TargetCharacter->ActiveEffects)
    {
        if ((Cast<ARoundIcebeamEffect>(Effect) || Cast<ARoundSlowEffect>(Effect)) && Effect->CauserCharacter == OwningCharacter && Effect->DisplayName.Contains(TEXT("Icebeam")))
        {
            PreviousIcebeamEffects.Add(Effect);
        }
    }
    for (ARoundEffectActor* Effect : PreviousIcebeamEffects)
    {
        Effect->RemoveEffect();
    }

    const int32 SlowDuration = FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("SlowDuration"))));
    if (bHadIcebeamSlow && GetAbilityValueFromKey(TEXT("RootIfAlreadySlowed")) > 0.0)
    {
        if (ARoundRootEffect* Root = GetWorld()->SpawnActor<ARoundRootEffect>())
        {
            Root->EffectId = TEXT("IcebeamAbsoluteZero");
            Root->DisplayName = TEXT("Absoluter Nullpunkt");
            Root->Description = TEXT("Kann sich nicht bewegen.");
            Root->CauserCharacter = OwningCharacter;
            Root->SetDurationInitiative(FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RootDurationInitiative")))), false);
            TargetCharacter->AddEffect(Root);
        }
    }
    else
    {
        ApplyIcebeamSlow(TargetCharacter, GetAbilityValueFromKey(TEXT("SlowAmount")), SlowDuration, true);
    }

    const double SplashSlowPercent = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("SplashSlowAmount")));
    const int32 SplashSlowDuration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("SplashSlowDurationInitiative"))));
    const int32 AoEInitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AoEInitiativeLossRadiusOne"))));
    for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(TargetTile))
    {
        ARoundCharacter* NeighborCharacter = Neighbor ? Neighbor->OccupyingCharacter : nullptr;
        if (!NeighborCharacter || !OwningCharacter->IsEnemyFor(NeighborCharacter))
        {
            continue;
        }

        if (SplashSlowPercent > 0.0 && SplashSlowDuration > 0)
        {
            ApplyIcebeamSlow(NeighborCharacter, SplashSlowPercent, SplashSlowDuration, false);
        }

        if (AoEInitiativeLoss > 0)
        {
            NeighborCharacter->ApplyDelay(AoEInitiativeLoss);
        }
    }


    const double ChainDamageMultiplier = GetAbilityValueFromKey(TEXT("ChainDamageMultiplier"));
    if (ChainDamageMultiplier > 0.0)
    {
        ARoundCharacter* ChainTarget = nullptr;
        int32 BestDistance = MAX_int32;
        for (ARoundGridTile* Tile : TargetTile->GetDistanceFields(FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ChainSearchRange"))))))
        {
            ARoundCharacter* Candidate = Tile ? Tile->OccupyingCharacter : nullptr;
            const int32 Distance = Tile ? TargetTile->GetDistance(Tile) : MAX_int32;
            if (Candidate && Candidate != TargetCharacter && OwningCharacter->IsEnemyFor(Candidate) && Distance < BestDistance)
            {
                ChainTarget = Candidate;
                BestDistance = Distance;
            }
        }
        if (ChainTarget)
        {
            bool bDidDie = false;
            ApplyAbilityDamageToTarget(ChainTarget, FMath::RoundToInt(GetAbilityDamage() * ChainDamageMultiplier), 0, bDidDie);
            if (!bDidDie)
            {
                ApplyIcebeamSlow(ChainTarget, FMath::Max(0.0, GetAbilityValueFromKey(TEXT("ChainSlowAmount"))), FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ChainSlowDurationInitiative")))), false);
            }
        }
    }

    const int32 PiercingRange = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("PiercingAdditionalRange"))));
    const double PiercingDamageMultiplier = GetAbilityValueFromKey(TEXT("PiercingDamageMultiplier"));
    if (PiercingRange > 0 && PiercingDamageMultiplier > 0.0 && OwningCharacter->CurrentTile)
    {
        const FVector2D BeamDirection = TargetTile->GridPosition - OwningCharacter->CurrentTile->GridPosition;
        ARoundGridTile* CurrentTile = TargetTile;
        TArray<ARoundCharacter*> PiercedTargets;
        for (int32 Step = 0; Step < PiercingRange && CurrentTile; ++Step)
        {
            ARoundGridTile* NextTile = nullptr;
            double BestDot = 0.0;
            for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(CurrentTile))
            {
                if (!Neighbor || Neighbor == TargetTile)
                {
                    continue;
                }
                const double Dot = FVector2D::DotProduct(BeamDirection, Neighbor->GridPosition - CurrentTile->GridPosition);
                if (Dot > BestDot)
                {
                    BestDot = Dot;
                    NextTile = Neighbor;
                }
            }
            CurrentTile = NextTile;
            ARoundCharacter* PiercedTarget = CurrentTile ? CurrentTile->OccupyingCharacter : nullptr;
            if (!PiercedTarget || PiercedTargets.Contains(PiercedTarget) || !OwningCharacter->IsEnemyFor(PiercedTarget))
            {
                continue;
            }
            bool bDidDie = false;
            ApplyAbilityDamageToTarget(PiercedTarget, FMath::RoundToInt(GetAbilityDamage() * PiercingDamageMultiplier), 0, bDidDie);
            PiercedTargets.Add(PiercedTarget);
            if (!bDidDie)
            {
                ApplyIcebeamSlow(PiercedTarget, GetAbilityValueFromKey(TEXT("SlowAmount")), SlowDuration, false);
            }
        }
    }

    const double BrittleMultiplier = GetAbilityValueFromKey(TEXT("BrittleIncomingDamageMultiplier"));
    if (BrittleMultiplier > 1.0 && TargetCharacter->Attributes && TargetCharacter->Attributes->IsAlive())
    {
        if (ARoundModifierEffect* Brittle = GetWorld()->SpawnActor<ARoundModifierEffect>())
        {
            Brittle->EffectId = TEXT("IcebeamBrittle");
            Brittle->DisplayName = TEXT("Sproedes Eis");
            Brittle->Description = TEXT("Erleidet erhoehten Schaden.");
            Brittle->IncomingDamageMultiplier = BrittleMultiplier;
            Brittle->bTreatAsDebuff = true;
            Brittle->CauserCharacter = OwningCharacter;
            Brittle->SetDurationInitiative(FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BrittleDurationInitiative")))), false);
            TargetCharacter->AddEffect(Brittle);
        }
    }

    return Result;
}

void ARoundIcebeamAbility::ApplyIcebeamSlow(ARoundCharacter* TargetCharacter, const double SlowAmount, const int32 DurationInitiative, const bool bPrimaryTarget)
{
    if (!TargetCharacter || SlowAmount <= 0.0 || DurationInitiative <= 0 || !GetWorld())
    {
        return;
    }

    if (ARoundIcebeamEffect* Slow = GetWorld()->SpawnActor<ARoundIcebeamEffect>())
    {
        Slow->EffectId = TEXT("IcebeamSlow");
        Slow->DisplayName = TEXT("Icebeam");
        Slow->Description = TEXT("Wird von Frost verlangsamt.");
        Slow->Amount = SlowAmount;
        Slow->CauserCharacter = OwningCharacter;
        if (bPrimaryTarget)
        {
            Slow->FirstMoveInitiativeLoss = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("FirstMoveInitiativeLoss"))));
            Slow->bFirstMovePenaltyAvailable = Slow->FirstMoveInitiativeLoss > 0;
            Slow->MaximumMovedFields = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("SlowEndsAfterMovedFields"))));
            Slow->FrostbreakDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("FrostbreakDamage"))));
        }
        Slow->SetDurationInitiative(DurationInitiative, false);
        TargetCharacter->AddEffect(Slow);
    }
}
