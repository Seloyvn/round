#include "Actors/RoundMineDeviceActor.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "EngineUtils.h"
#include "Game/RoundGameState.h"
#include "Effects/RoundRootEffect.h"

ARoundMineDeviceActor::ARoundMineDeviceActor()
{
    bCanTriggerManually = true;
    bCanTriggerOnEnter = true;
}

void ARoundMineDeviceActor::SetArmDelay(int32 InArmDelayInitiative)
{
    ArmDelayInitiative = FMath::Max(0, InArmDelayInitiative);
    RemainingArmDelayInitiative = ArmDelayInitiative;
    RemainingEnemyRevealInitiative = FMath::Max(EnemyRevealInitiative, RemainingArmDelayInitiative);
}

void ARoundMineDeviceActor::ApplyCoreMineState(const int32 InArmDelayInitiative, const int32 InEnemyRevealInitiative)
{
    RemainingArmDelayInitiative = FMath::Max(0, InArmDelayInitiative);
    RemainingEnemyRevealInitiative = FMath::Max(0, InEnemyRevealInitiative);
}

void ARoundMineDeviceActor::OnInitiativeAdvanced(int32 InitiativeAmount)
{
    if (IsCoreAuthoritative()) return;
    Super::OnInitiativeAdvanced(InitiativeAmount);
    if (!IsActorBeingDestroyed() && InitiativeAmount > 0 && RemainingArmDelayInitiative > 0)
    {
        RemainingArmDelayInitiative = FMath::Max(0, RemainingArmDelayInitiative - InitiativeAmount);
    }

    if (!IsActorBeingDestroyed() && InitiativeAmount > 0 && RemainingEnemyRevealInitiative > 0)
    {
        RemainingEnemyRevealInitiative = FMath::Max(0, RemainingEnemyRevealInitiative - InitiativeAmount);
    }
}

bool ARoundMineDeviceActor::CanTriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext) const
{
    if (IsCoreAuthoritative()) return false;
    if (!Super::CanTriggerDevice_Implementation(TriggerContext))
    {
        return false;
    }

    if (TriggerContext.TriggerType == ERoundDeviceTriggerType::OnEnter)
    {
        return RemainingArmDelayInitiative <= 0
            && TriggerContext.TriggeringCharacter
            && OwningCharacter
            && OwningCharacter->IsEnemyFor(TriggerContext.TriggeringCharacter);
    }

    return true;
}

FRoundActionResult ARoundMineDeviceActor::TriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext)
{
    FRoundActionResult Result;
    ARoundCharacter* OwnerCharacter = OwningCharacter;
    if (!OwnerCharacter || !CurrentTile || !GetWorld())
    {
        Result.Reason = TEXT("Mine has no valid owner or tile.");
        return Result;
    }

    int32 HitCount = 0;
    for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
    {
        ARoundCharacter* Candidate = *It;
        if (!Candidate || !Candidate->IsAlive() || !OwnerCharacter->IsEnemyFor(Candidate) || !Candidate->CurrentTile)
        {
            continue;
        }

        if (CurrentTile->GetDistance(Candidate->CurrentTile) > ExplosionRadius)
        {
            continue;
        }

        bool bDidDie = false;
        int32 LocalDefenseIgnore = DefenseIgnore;
        const int32 FinalDamage = OwnerCharacter->ModifyOutgoingDamage(ExplosionDamage, ExplosionDamageType, Candidate, LocalDefenseIgnore);
        const int32 DamageDone = Candidate->ApplyRoundDamage(FinalDamage, ExplosionDamageType, LocalDefenseIgnore, OwnerCharacter, bDidDie);
        if (DamageDone > 0)
        {
            ++HitCount;
            if (!bDidDie && InitiativeLossOnHit > 0) Candidate->ApplyDelay(InitiativeLossOnHit);
            if (!bDidDie && RootDurationInitiative > 0)
            {
                if (ARoundRootEffect* Root = GetWorld()->SpawnActor<ARoundRootEffect>())
                {
                    Root->DisplayName = TEXT("Mine Root"); Root->CauserCharacter = OwnerCharacter;
                    Root->SetDurationInitiative(RootDurationInitiative, false); Candidate->AddEffect(Root);
                }
            }
        }
    }

    if (ARoundGameState* GameState = OwnerCharacter->GetRoundGameState())
    {
        GameState->OnBoardStateChanged.Broadcast();
    }

    if (!(bPreserveOnManualTrigger && TriggerContext.TriggerType == ERoundDeviceTriggerType::Manual)) DestroyDevice();
    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Mine exploded and hit %d target(s)."), HitCount);
    return Result;
}

bool ARoundMineDeviceActor::IsDeviceVisibleToTeam_Implementation(ERoundTeam ViewingTeam, bool bTileInVision) const
{
    if (ViewingTeam == ERoundTeam::Neutral)
    {
        return true;
    }

    if (OwningCharacter && OwningCharacter->Team == ViewingTeam)
    {
        return true;
    }

    return bTileInVision && RemainingEnemyRevealInitiative > 0;
}
