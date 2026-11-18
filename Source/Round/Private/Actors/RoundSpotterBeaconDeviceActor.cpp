#include "Actors/RoundSpotterBeaconDeviceActor.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Effects/RoundSpottedEffect.h"
#include "Effects/RoundModifierEffect.h"
#include "EngineUtils.h"
#include "Game/RoundGameState.h"

ARoundSpotterBeaconDeviceActor::ARoundSpotterBeaconDeviceActor()
{
    bCanTriggerManually = true;
    TriggerIntervalInitiative = 40;
}

FRoundActionResult ARoundSpotterBeaconDeviceActor::TriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext)
{
    FRoundActionResult Result;
    ARoundCharacter* OwnerCharacter = OwningCharacter;
    if (!OwnerCharacter || !CurrentTile || !GetWorld())
    {
        Result.Reason = TEXT("Spotter beacon has no valid owner or tile.");
        return Result;
    }

    int32 AppliedCount = 0;
    for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
    {
        ARoundCharacter* Candidate = *It;
        if (!Candidate || !Candidate->IsAlive() || !OwnerCharacter->IsEnemyFor(Candidate) || !Candidate->CurrentTile)
        {
            continue;
        }

        if (CurrentTile->GetDistance(Candidate->CurrentTile) > BeaconRadius)
        {
            continue;
        }

        ARoundSpottedEffect* SpottedEffect = GetWorld()->SpawnActor<ARoundSpottedEffect>(ARoundSpottedEffect::StaticClass());
        if (!SpottedEffect)
        {
            continue;
        }

        SpottedEffect->EffectId = TEXT("EngineerSpotted");
        SpottedEffect->DisplayName = TEXT("Spotted");
        SpottedEffect->Description = TEXT("Ist aufgedeckt und erleidet mehr Schaden.");
        SpottedEffect->CauserCharacter = OwnerCharacter;
        SpottedEffect->DamageTakenMultiplier = DamageTakenMultiplier;
        SpottedEffect->SetDurationInitiative(DebuffDurationInitiative, false);
        Candidate->AddEffect(SpottedEffect);
        if (InitiativeLoss > 0) Candidate->ApplyDelay(InitiativeLoss);
        if (MoveCostAdd > 0 || (OutgoingDamageMultiplier > 0.0 && OutgoingDamageMultiplier < 1.0))
        {
            if (ARoundModifierEffect* Debuff = GetWorld()->SpawnActor<ARoundModifierEffect>())
            {
                Debuff->EffectId = TEXT("EngineerSpotterUtility"); Debuff->DisplayName = TEXT("Targeted");
                Debuff->MoveCostAdd = MoveCostAdd; Debuff->OutgoingDamageMultiplier = OutgoingDamageMultiplier > 0.0 ? OutgoingDamageMultiplier : 1.0;
                Debuff->bTreatAsDebuff = true; Debuff->CauserCharacter = OwnerCharacter;
                Debuff->SetDurationInitiative(DebuffDurationInitiative, false); Candidate->AddEffect(Debuff);
            }
        }
        ++AppliedCount;
    }

    if (ARoundGameState* GameState = OwnerCharacter->GetRoundGameState())
    {
        GameState->OnBoardStateChanged.Broadcast();
    }

    Result.bSuccess = AppliedCount > 0;
    Result.Reason = FString::Printf(TEXT("Spotter beacon marked %d target(s)."), AppliedCount);
    return Result;
}

TArray<ARoundGridTile*> ARoundSpotterBeaconDeviceActor::GetDeviceVisionFields_Implementation() const
{
    if (!CurrentTile)
    {
        return {};
    }

    TArray<ARoundGridTile*> VisionFields = CurrentTile->GetDistanceFields(BeaconRadius);
    VisionFields.AddUnique(CurrentTile);
    return VisionFields;
}
