#include "Abilities/RoundRecallBladeGuardAbility.h"

#include "Actors/RoundBladeActor.h"
#include "Actors/RoundCharacterBladecaller.h"
#include "Actors/RoundEffectActor.h"
#include "Actors/RoundGridTile.h"
#include "Effects/RoundBladeGuardEffect.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundBladeGuardAbility, Log, All);

void ARoundRecallBladeGuardAbility::OnGrantedToCharacter()
{
    ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    if (!Bladecaller)
    {
        return;
    }

    Bladecaller->GetBladePickedUpEvent().RemoveAll(this);
    Bladecaller->GetBladePickedUpEvent().AddUObject(this, &ARoundRecallBladeGuardAbility::HandleBladePickedUp);
    bBladePickupSubscribed = true;

    UE_LOG(
        LogRoundBladeGuardAbility,
        Log,
        TEXT("OnGrantedToCharacter subscribed Ability=%s Character=%s RuntimeClass=%s"),
        *GetName(),
        *GetNameSafe(Bladecaller),
        *GetNameSafe(GetClass()));
}

void ARoundRecallBladeGuardAbility::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter))
    {
        Bladecaller->GetBladePickedUpEvent().RemoveAll(this);
    }

    Super::EndPlay(EndPlayReason);
}

bool ARoundRecallBladeGuardAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    const ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    return Bladecaller
        && TargetTile
        && Bladecaller->CurrentTile
        && IsTargetInRange(TargetTile)
        && GetAbilityVisibility() == ERoundButtonState::Enabled
        && (!Bladecaller->CurrentTile->OccupyingFieldObject || Bladecaller->CurrentTile->OccupyingFieldObject == Bladecaller->FindBladeAtTile(const_cast<ARoundGridTile*>(TargetTile)))
        && Bladecaller->FindBladeAtTile(const_cast<ARoundGridTile*>(TargetTile)) != nullptr;
}

void ARoundRecallBladeGuardAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    const ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    const ARoundGameState* RoundGameState = Bladecaller ? Bladecaller->GetRoundGameState() : nullptr;
    if (!Bladecaller || !RoundGameState || !Bladecaller->CurrentTile || !TargetTile)
    {
        return;
    }

    OutTiles.Append(RoundGameState->GetFieldsBetween(const_cast<ARoundGridTile*>(TargetTile), Bladecaller->CurrentTile, true));
}

FRoundActionResult ARoundRecallBladeGuardAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    UE_LOG(
        LogRoundBladeGuardAbility,
        Log,
        TEXT("ExecuteResolvedAbility start Ability=%s Character=%s TargetTile=%s CurrentTile=%s"),
        *GetName(),
        *GetNameSafe(Bladecaller),
        *GetNameSafe(TargetTile),
        *GetNameSafe(Bladecaller ? Bladecaller->CurrentTile : nullptr));
    if (!Bladecaller || !TargetTile || !Bladecaller->CurrentTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundBladeActor* Blade = Bladecaller->FindBladeAtTile(TargetTile);
    if (!Blade)
    {
        UE_LOG(LogRoundBladeGuardAbility, Warning, TEXT("ExecuteResolvedAbility no blade on tile Character=%s TargetTile=%s"), *GetNameSafe(Bladecaller), *GetNameSafe(TargetTile));
        Result.Reason = TEXT("No blade on target tile.");
        return Result;
    }

    UE_LOG(
        LogRoundBladeGuardAbility,
        Log,
        TEXT("ExecuteResolvedAbility found blade Character=%s Blade=%s BladeTile=%s CharacterTile=%s"),
        *GetNameSafe(Bladecaller),
        *GetNameSafe(Blade),
        *GetNameSafe(Blade->CurrentTile),
        *GetNameSafe(Bladecaller->CurrentTile));

    if (Bladecaller->CurrentTile->OccupyingFieldObject && Bladecaller->CurrentTile->OccupyingFieldObject != Blade)
    {
        UE_LOG(
            LogRoundBladeGuardAbility,
            Warning,
            TEXT("ExecuteResolvedAbility blocked current tile Character=%s CurrentTile=%s BlockingObject=%s"),
            *GetNameSafe(Bladecaller),
            *GetNameSafe(Bladecaller->CurrentTile),
            *GetNameSafe(Bladecaller->CurrentTile->OccupyingFieldObject));
        Result.Reason = TEXT("Current tile is blocked by another field object.");
        return Result;
    }

    if (Blade->CurrentTile != Bladecaller->CurrentTile)
    {
        const FRoundFieldObjectMoveResult MoveResult = Bladecaller->RecallBlade(Blade);
        const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
        for (ARoundCharacter* HitCharacter : MoveResult.HitCharacters)
        {
            if (!HitCharacter || !Bladecaller->IsEnemyFor(HitCharacter)) continue;
            bool bDidDie = false;
            ApplyAbilityDamageToTarget(HitCharacter, GetAbilityDamage(), DefenseIgnore, bDidDie);
        }
        UE_LOG(
            LogRoundBladeGuardAbility,
            Log,
            TEXT("ExecuteResolvedAbility recall result Character=%s Blade=%s Traversed=%d ReachedDestination=%d BladeTileAfter=%s"),
            *GetNameSafe(Bladecaller),
            *GetNameSafe(Blade),
            MoveResult.TraversedTiles.Num(),
            MoveResult.bReachedDestination ? 1 : 0,
            *GetNameSafe(Blade->CurrentTile));
        if (Blade->CurrentTile != Bladecaller->CurrentTile)
        {
            UE_LOG(
                LogRoundBladeGuardAbility,
                Warning,
                TEXT("ExecuteResolvedAbility recall failed Character=%s Blade=%s CharacterTile=%s BladeTile=%s"),
                *GetNameSafe(Bladecaller),
                *GetNameSafe(Blade),
                *GetNameSafe(Bladecaller->CurrentTile),
                *GetNameSafe(Blade->CurrentTile));
            Result.Reason = TEXT("Blade could not be pulled to the caster.");
            return Result;
        }
    }

    if (!Bladecaller->PickUpBlade(Blade))
    {
        UE_LOG(
            LogRoundBladeGuardAbility,
            Warning,
            TEXT("ExecuteResolvedAbility pickup failed Character=%s Blade=%s BladeTile=%s"),
            *GetNameSafe(Bladecaller),
            *GetNameSafe(Blade),
            *GetNameSafe(Blade->CurrentTile));
        Result.Reason = TEXT("Blade could not be picked up.");
        return Result;
    }

    UE_LOG(LogRoundBladeGuardAbility, Log, TEXT("ExecuteResolvedAbility success Character=%s"), *GetNameSafe(Bladecaller));
    Result.bSuccess = true;
    Result.Reason = TEXT("Blade picked up into guard.");
    return Result;
}

void ARoundRecallBladeGuardAbility::HandleBladePickedUp(ARoundGridTile* PickedUpFromTile)
{
    ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    if (!Bladecaller || !Bladecaller->HasAuthority() || !Bladecaller->GetWorld())
    {
        UE_LOG(
            LogRoundBladeGuardAbility,
            Warning,
            TEXT("HandleBladePickedUp rejected Ability=%s Character=%s HasAuthority=%d World=%d"),
            *GetName(),
            *GetNameSafe(Bladecaller),
            Bladecaller && Bladecaller->HasAuthority() ? 1 : 0,
            Bladecaller && Bladecaller->GetWorld() ? 1 : 0);
        return;
    }

    const double DamageReductionPerStack = FMath::Clamp(GetAbilityValueFromKey(TEXT("DamageReduction")), 0.0, 1.0);
    const int32 GuardDurationInitiative = FMath::Max(1, GetAbilityDurationInitiativeFromKey(TEXT("DurationInitiative")));
    UE_LOG(
        LogRoundBladeGuardAbility,
        Log,
        TEXT("HandleBladePickedUp Ability=%s Character=%s Tile=%s DamageReduction=%.3f Duration=%d"),
        *GetName(),
        *GetNameSafe(Bladecaller),
        *GetNameSafe(PickedUpFromTile),
        DamageReductionPerStack,
        GuardDurationInitiative);

    if (DamageReductionPerStack <= 0.0 || GuardDurationInitiative <= 0)
    {
        UE_LOG(
            LogRoundBladeGuardAbility,
            Warning,
            TEXT("HandleBladePickedUp aborted Ability=%s Character=%s DamageReduction=%.3f Duration=%d"),
            *GetName(),
            *GetNameSafe(Bladecaller),
            DamageReductionPerStack,
            GuardDurationInitiative);
        return;
    }

    ARoundBladeGuardEffect* GuardEffect = nullptr;
    for (ARoundEffectActor* Effect : Bladecaller->ActiveEffects)
    {
        GuardEffect = Cast<ARoundBladeGuardEffect>(Effect);
        if (GuardEffect && GuardEffect->CauserCharacter == Bladecaller)
        {
            break;
        }

        GuardEffect = nullptr;
    }

    if (!GuardEffect)
    {
        GuardEffect = Bladecaller->GetWorld()->SpawnActor<ARoundBladeGuardEffect>(ARoundBladeGuardEffect::StaticClass());
        if (!GuardEffect)
        {
            UE_LOG(LogRoundBladeGuardAbility, Error, TEXT("HandleBladePickedUp failed to spawn guard effect Character=%s"), *GetNameSafe(Bladecaller));
            return;
        }

        GuardEffect->EffectId = TEXT("BladeGuard");
        GuardEffect->DisplayName = TEXT("Blade Guard");
        GuardEffect->Description = TEXT("Aufgehobene Klingen reduzieren eingehenden Schaden.");
        GuardEffect->CauserCharacter = Bladecaller;
        GuardEffect->DamageReductionPerStack = DamageReductionPerStack;
        Bladecaller->AddEffect(GuardEffect);
        UE_LOG(
            LogRoundBladeGuardAbility,
            Log,
            TEXT("HandleBladePickedUp created guard effect Character=%s Effect=%s"),
            *GetNameSafe(Bladecaller),
            *GetNameSafe(GuardEffect));
    }

    GuardEffect->AddStack(GuardDurationInitiative);
    const int32 HealOnRecall = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("HealOnRecall"))));
    if (HealOnRecall > 0) Bladecaller->Heal(HealOnRecall);
    if (Bladecaller->Attributes) Bladecaller->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnRecall")))));
    ReduceCooldown(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionOnRecall")))));
    GuardEffect->ForceNetUpdate();
    UE_LOG(
        LogRoundBladeGuardAbility,
        Log,
        TEXT("HandleBladePickedUp stacked guard Character=%s Effect=%s StackCount=%d RemainingInitiative=%d"),
        *GetNameSafe(Bladecaller),
        *GetNameSafe(GuardEffect),
        GuardEffect->StackRemainingInitiatives.Num(),
        GuardEffect->RemainingInitiative);
}
