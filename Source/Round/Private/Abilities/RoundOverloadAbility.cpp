#include "Abilities/RoundOverloadAbility.h"

#include "Actors/RoundCharacterEngineer.h"
#include "Actors/RoundEngineerDeviceActor.h"
#include "Actors/RoundGridTile.h"
#include "Interfaces/RoundDeviceInterface.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"
#include "EngineUtils.h"

FRoundActionResult ARoundOverloadAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterEngineer* Engineer = Cast<ARoundCharacterEngineer>(OwningCharacter);
    if (!Engineer || !Engineer->CurrentTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    const bool bDestroyAfterOverload = GetAbilityValueFromKey(TEXT("DestroyAfterOverload")) > 0.0;
    const bool bDoubleTrigger = GetAbilityValueFromKey(TEXT("DoubleTrigger")) > 0.0;
    const int32 RepairAmount = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("RepairDevices"))));
    const int32 LifetimeExtension = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("LifetimeExtensionInitiative"))));
    const int32 DestroyExplosionDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DestroyExplosionDamage"))));
    const int32 OverloadRange = GetAbilityRange();
    int32 TriggeredCount = 0;

    const TArray<TObjectPtr<ARoundEngineerDeviceActor>> DevicesSnapshot = Engineer->GetActiveDevices();
    for (ARoundEngineerDeviceActor* DeviceActor : DevicesSnapshot)
    {
        if (!DeviceActor || !DeviceActor->CurrentTile)
        {
            continue;
        }

        if (OverloadRange > 0 && Engineer->CurrentTile->GetDistance(DeviceActor->CurrentTile) > OverloadRange)
        {
            continue;
        }

        FRoundDeviceTriggerContext TriggerContext;
        TriggerContext.TriggerType = ERoundDeviceTriggerType::Manual;
        TriggerContext.TriggeringCharacter = Engineer;
        TriggerContext.TriggeringAbility = this;
        TriggerContext.TriggerTile = DeviceActor->CurrentTile;

        if (!IRoundDeviceInterface::Execute_CanTriggerDevice(DeviceActor, TriggerContext))
        {
            continue;
        }

        if (RepairAmount > 0) DeviceActor->RepairDevice(RepairAmount);
        if (LifetimeExtension > 0) DeviceActor->SetLifetimeInitiative(DeviceActor->RemainingInitiative + LifetimeExtension);
        const FRoundActionResult TriggerResult = IRoundDeviceInterface::Execute_TriggerDevice(DeviceActor, TriggerContext);
        if (TriggerResult.bSuccess)
        {
            ++TriggeredCount;
            if (bDoubleTrigger && IsValid(DeviceActor)) IRoundDeviceInterface::Execute_TriggerDevice(DeviceActor, TriggerContext);
        }

        if (bDestroyAfterOverload && IsValid(DeviceActor))
        {
            if (DestroyExplosionDamage > 0 && DeviceActor->CurrentTile)
            {
                for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
                {
                    ARoundCharacter* Enemy = *It;
                    if (!Enemy || !Enemy->IsAlive() || !Engineer->IsEnemyFor(Enemy) || !Enemy->CurrentTile || DeviceActor->CurrentTile->GetDistance(Enemy->CurrentTile) > 1) continue;
                    bool bDidDie = false; int32 DefenseIgnore = 0;
                    const int32 Damage = Engineer->ModifyOutgoingDamage(DestroyExplosionDamage, ERoundDamageType::Lightning, Enemy, DefenseIgnore);
                    Enemy->ApplyRoundDamage(Damage, ERoundDamageType::Lightning, DefenseIgnore, Engineer, bDidDie);
                }
            }
            DeviceActor->DestroyDevice();
        }
    }

    if (TriggeredCount > 0 && Engineer->Attributes)
    {
        Engineer->Attributes->GainInitiative(TriggeredCount * FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainPerDevice")))));
        ReduceCooldown(TriggeredCount * FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionPerDevice")))));
    }

    if (ARoundGameState* GameState = Engineer->GetRoundGameState())
    {
        GameState->OnBoardStateChanged.Broadcast();
    }

    Result.bSuccess = TriggeredCount > 0;
    Result.Reason = FString::Printf(TEXT("Overload triggered %d device(s)."), TriggeredCount);
    return Result;
}

void ARoundOverloadAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    const ARoundCharacterEngineer* Engineer = Cast<ARoundCharacterEngineer>(OwningCharacter);
    if (!Engineer || !Engineer->CurrentTile)
    {
        return;
    }

    const int32 OverloadRange = GetAbilityRange();
    for (ARoundEngineerDeviceActor* DeviceActor : Engineer->GetActiveDevices())
    {
        if (!DeviceActor || !DeviceActor->CurrentTile)
        {
            continue;
        }

        if (OverloadRange > 0 && Engineer->CurrentTile->GetDistance(DeviceActor->CurrentTile) > OverloadRange)
        {
            continue;
        }

        OutTiles.AddUnique(DeviceActor->CurrentTile);
    }
}
