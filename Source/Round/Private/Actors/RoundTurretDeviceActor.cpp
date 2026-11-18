#include "Actors/RoundTurretDeviceActor.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "EngineUtils.h"
#include "Game/RoundGameState.h"

ARoundTurretDeviceActor::ARoundTurretDeviceActor()
{
    bCanTriggerManually = true;
    TriggerIntervalInitiative = 40;
}

FRoundActionResult ARoundTurretDeviceActor::TriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext)
{
    FRoundActionResult Result;
    ARoundCharacter* OwnerCharacter = OwningCharacter;
    if (!OwnerCharacter || !CurrentTile || !GetWorld())
    {
        Result.Reason = TEXT("Turret has no valid owner or tile.");
        return Result;
    }

    ARoundCharacter* BestTarget = nullptr;
    int32 BestDistance = MAX_int32;
    for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
    {
        ARoundCharacter* Candidate = *It;
        if (!Candidate || !Candidate->IsAlive() || !OwnerCharacter->IsEnemyFor(Candidate) || !Candidate->CurrentTile)
        {
            continue;
        }

        const int32 Distance = CurrentTile->GetDistance(Candidate->CurrentTile);
        if (Distance > TurretRange)
        {
            continue;
        }

        if (!BestTarget || Distance < BestDistance)
        {
            BestTarget = Candidate;
            BestDistance = Distance;
        }
    }

    if (!BestTarget)
    {
        Result.Reason = TEXT("No target in range.");
        return Result;
    }

    auto FireAt = [this, OwnerCharacter](ARoundCharacter* Target, int32 Damage)
    {
        bool bDidDie = false;
        int32 LocalDefenseIgnore = DefenseIgnore;
        int32 FinalDamage = Damage;
        if (Target && Target->Attributes && Target->Attributes->BaseStats.MaxHealth > 0 && LowHealthThreshold > 0.0 && LowHealthDamageMultiplier > 0.0 &&
            static_cast<double>(Target->Attributes->CurrentHealth) / Target->Attributes->BaseStats.MaxHealth <= LowHealthThreshold)
        {
            FinalDamage = FMath::RoundToInt(FinalDamage * LowHealthDamageMultiplier);
        }
        FinalDamage = OwnerCharacter->ModifyOutgoingDamage(FinalDamage, TurretDamageType, Target, LocalDefenseIgnore);
        const int32 Applied = Target->ApplyRoundDamage(FinalDamage, TurretDamageType, LocalDefenseIgnore, OwnerCharacter, bDidDie);
        if (!bDidDie && InitiativeLossOnHit > 0) Target->ApplyDelay(InitiativeLossOnHit);
        return Applied;
    };

    const int32 DamageDone = FireAt(BestTarget, TurretDamage);
    if (SecondaryTargetDamageMultiplier > 0.0)
    {
        ARoundCharacter* SecondTarget = nullptr; int32 SecondDistance = MAX_int32;
        for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
        {
            ARoundCharacter* Candidate = *It;
            if (!Candidate || Candidate == BestTarget || !Candidate->IsAlive() || !OwnerCharacter->IsEnemyFor(Candidate) || !Candidate->CurrentTile) continue;
            const int32 Distance = CurrentTile->GetDistance(Candidate->CurrentTile);
            if (Distance <= TurretRange && Distance < SecondDistance) { SecondTarget = Candidate; SecondDistance = Distance; }
        }
        if (SecondTarget) FireAt(SecondTarget, FMath::RoundToInt(TurretDamage * SecondaryTargetDamageMultiplier));
    }
    if (ARoundGameState* GameState = OwnerCharacter->GetRoundGameState())
    {
        GameState->OnBoardStateChanged.Broadcast();
    }

    Result.bSuccess = DamageDone > 0;
    Result.Reason = FString::Printf(TEXT("Turret hit %s for %d damage."), *GetNameSafe(BestTarget), DamageDone);
    return Result;
}

bool ARoundTurretDeviceActor::BlocksCharactersOnTile_Implementation() const
{
    return true;
}
