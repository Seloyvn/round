#include "Actors/RoundShieldEmitterDeviceActor.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Actors/RoundEffectActor.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundShieldEffect.h"
#include "EngineUtils.h"
#include "Game/RoundGameState.h"

ARoundShieldEmitterDeviceActor::ARoundShieldEmitterDeviceActor()
{
    bCanTriggerManually = true;
    TriggerIntervalInitiative = 40;
}

FRoundActionResult ARoundShieldEmitterDeviceActor::TriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext)
{
    FRoundActionResult Result;
    ARoundCharacter* OwnerCharacter = OwningCharacter;
    if (!OwnerCharacter || !CurrentTile || !GetWorld())
    {
        Result.Reason = TEXT("Shield emitter has no valid owner or tile.");
        return Result;
    }

    TArray<ARoundCharacter*> Candidates;
    for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
    {
        ARoundCharacter* Candidate = *It;
        if (!Candidate || !Candidate->IsAlive() || Candidate->Team != OwnerCharacter->Team || !Candidate->CurrentTile)
        {
            continue;
        }

        if (CurrentTile->GetDistance(Candidate->CurrentTile) > ShieldRadius)
        {
            continue;
        }

        Candidates.Add(Candidate);
    }

    int32 AppliedCount = 0;
    for (ARoundCharacter* Candidate : Candidates)
    {
        ARoundShieldEffect* ShieldEffect = GetWorld()->SpawnActor<ARoundShieldEffect>(ARoundShieldEffect::StaticClass());
        if (!ShieldEffect)
        {
            continue;
        }

        ShieldEffect->EffectId = TEXT("EngineerShield");
        ShieldEffect->DisplayName = TEXT("Shield");
        ShieldEffect->Description = TEXT("Absorbiert eingehenden Schaden.");
        ShieldEffect->CauserCharacter = OwnerCharacter;
        ShieldEffect->ShieldAmountRemaining = FMath::RoundToInt(ShieldAmount * (Candidates.Num() == 1 && SoloShieldMultiplier > 0.0 ? SoloShieldMultiplier : 1.0));
        ShieldEffect->SetDurationInitiative(ShieldDurationInitiative, false);
        Candidate->AddEffect(ShieldEffect);
        if (HealAmount > 0) Candidate->Heal(HealAmount);
        if (InitiativeGain > 0 && Candidate->Attributes) Candidate->Attributes->GainInitiative(InitiativeGain);
        if (bCleanseOneDebuff)
        {
            const TArray<TObjectPtr<ARoundEffectActor>> Effects = Candidate->ActiveEffects;
            for (ARoundEffectActor* Effect : Effects)
            {
                if (Effect && Effect->IsDebuff()) { Effect->RemoveEffect(); break; }
            }
        }
        ++AppliedCount;
    }

    if (ARoundGameState* GameState = OwnerCharacter->GetRoundGameState())
    {
        GameState->OnBoardStateChanged.Broadcast();
    }

    Result.bSuccess = AppliedCount > 0;
    Result.Reason = FString::Printf(TEXT("Shield emitter affected %d ally/allies."), AppliedCount);
    return Result;
}
