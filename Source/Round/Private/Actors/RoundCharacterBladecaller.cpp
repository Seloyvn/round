#include "Actors/RoundCharacterBladecaller.h"

#include "Actors/RoundBladeActor.h"
#include "Actors/RoundEffectActor.h"
#include "Actors/RoundGridTile.h"
#include "Actors/RoundAbilityActor.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundBladePickup, Log, All);

ARoundCharacterBladecaller::ARoundCharacterBladecaller()
{
    BladeActorClass = ARoundBladeActor::StaticClass();
}

void ARoundCharacterBladecaller::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ARoundCharacterBladecaller, ActiveBlades);
}

void ARoundCharacterBladecaller::Die()
{
    RemoveAllBlades();
    Super::Die();
}

void ARoundCharacterBladecaller::AdvanceGlobalInitiative(int32 Amount)
{
    Super::AdvanceGlobalInitiative(Amount);

    if (Amount <= 0)
    {
        return;
    }

    const TArray<TObjectPtr<ARoundBladeActor>> BladesSnapshot = ActiveBlades;
    for (ARoundBladeActor* Blade : BladesSnapshot)
    {
        if (Blade)
        {
            Blade->OnInitiativeAdvanced(Amount);
        }
    }

    CleanupInvalidBlades();
}

TSubclassOf<ARoundBladeActor> ARoundCharacterBladecaller::GetBladeActorClass() const
{
    if (BladeActorClass)
    {
        return BladeActorClass;
    }

    return ARoundBladeActor::StaticClass();
}

int32 ARoundCharacterBladecaller::GetBladeLifetimeInitiative() const
{
    const int32 BaseValue = FMath::RoundToInt(GetValueFromKey(TEXT("BladeLifeTime")));
    return FMath::Max(1, (BaseValue > 0 ? BaseValue : 150) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("BladeLifeTime"))));
}

int32 ARoundCharacterBladecaller::ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const
{
    int32 Modified = Super::ModifyOutgoingDamage(Amount, DamageType, TargetCharacter, DefenseIgnore);
    const double PerBlade = FMath::Max(0.0, GetPassiveValueFromKey(TEXT("DamageMultiplierAddPerActiveBlade")));
    return FMath::RoundToInt(static_cast<double>(Modified) * (1.0 + PerBlade * GetActiveBladeCount()));
}

int32 ARoundCharacterBladecaller::GetActiveBladeCount() const
{
    const_cast<ARoundCharacterBladecaller*>(this)->CleanupInvalidBlades();
    return ActiveBlades.Num();
}

const TArray<TObjectPtr<ARoundBladeActor>>& ARoundCharacterBladecaller::GetActiveBlades() const
{
    const_cast<ARoundCharacterBladecaller*>(this)->CleanupInvalidBlades();
    return ActiveBlades;
}

ARoundBladeActor* ARoundCharacterBladecaller::FindBladeAtTile(ARoundGridTile* Tile) const
{
    if (!Tile)
    {
        return nullptr;
    }

    for (ARoundBladeActor* Blade : ActiveBlades)
    {
        if (Blade && Blade->CurrentTile == Tile)
        {
            return Blade;
        }
    }

    return nullptr;
}

ARoundBladeActor* ARoundCharacterBladecaller::SummonBladeAtTile(ARoundGridTile* Tile)
{
    CleanupInvalidBlades();
    if (!HasAuthority() || !GetWorld() || !Tile)
    {
        return nullptr;
    }

    if (Tile->OccupyingFieldObject)
    {
        return nullptr;
    }

    UClass* BladeClassToSpawn = GetBladeActorClass().Get();
    ARoundBladeActor* Blade = BladeClassToSpawn
        ? GetWorld()->SpawnActor<ARoundBladeActor>(BladeClassToSpawn)
        : nullptr;
    if (!Blade)
    {
        return nullptr;
    }

    Blade->OwningCharacter = this;
    if (!Blade->SetField(Tile))
    {
        Blade->Destroy();
        return nullptr;
    }

    Blade->SetLifetimeInitiative(GetBladeLifetimeInitiative());

    ActiveBlades.Add(Blade);
    if (Attributes)
    {
        Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InitiativeGainOnBladeSummon")))));
    }
    if (GetPassiveValueFromKey(TEXT("RefreshBladesOnSummon")) > 0.0)
    {
        for (ARoundBladeActor* ActiveBlade : ActiveBlades)
        {
            if (ActiveBlade) ActiveBlade->SetLifetimeInitiative(GetBladeLifetimeInitiative());
        }
    }
    NotifyRoundStateChanged();
    return Blade;
}

FRoundFieldObjectMoveResult ARoundCharacterBladecaller::MoveBladeToTile(ARoundBladeActor* Blade, ARoundGridTile* Tile)
{
    FRoundFieldObjectMoveResult Result;
    CleanupInvalidBlades();
    if (!Blade || !Tile || Blade->OwningCharacter != this)
    {
        return Result;
    }

    Result = Blade->MoveToField(Tile);
    NotifyRoundStateChanged();
    return Result;
}

FRoundFieldObjectMoveResult ARoundCharacterBladecaller::RecallBlade(ARoundBladeActor* Blade)
{
    FRoundFieldObjectMoveResult Result;
    if (!Blade || !CurrentTile)
    {
        return Result;
    }

    return MoveBladeToTile(Blade, CurrentTile);
}

bool ARoundCharacterBladecaller::PickUpBlade(ARoundBladeActor* Blade)
{
    CleanupInvalidBlades();
    UE_LOG(
        LogRoundBladePickup,
        Log,
        TEXT("PickUpBlade start Character=%s HasAuthority=%d Blade=%s BladeOwner=%s BladeTile=%s ActiveBlades=%d"),
        *GetName(),
        HasAuthority() ? 1 : 0,
        *GetNameSafe(Blade),
        *GetNameSafe(Blade ? Blade->OwningCharacter : nullptr),
        *GetNameSafe(Blade ? Blade->CurrentTile : nullptr),
        ActiveBlades.Num());
    if (!HasAuthority() || !GetWorld() || !Blade || Blade->OwningCharacter != this)
    {
        UE_LOG(
            LogRoundBladePickup,
            Warning,
            TEXT("PickUpBlade rejected Character=%s HasAuthority=%d World=%d BladeValid=%d OwnerMatches=%d"),
            *GetName(),
            HasAuthority() ? 1 : 0,
            GetWorld() ? 1 : 0,
            Blade ? 1 : 0,
            (Blade && Blade->OwningCharacter == this) ? 1 : 0);
        return false;
    }

    ARoundGridTile* PickedUpFromTile = Blade->CurrentTile;
    ActiveBlades.Remove(Blade);
    Blade->SetActorHiddenInGame(true);
    Blade->SetActorEnableCollision(false);
    Blade->RemoveFromField();
    CleanupInvalidBlades();
    ForceNetUpdate();
    UE_LOG(
        LogRoundBladePickup,
        Log,
        TEXT("PickUpBlade removed Character=%s Blade=%s PickedUpFromTile=%s ActiveBladesAfter=%d BladePendingKill=%d"),
        *GetName(),
        *GetNameSafe(Blade),
        *GetNameSafe(PickedUpFromTile),
        ActiveBlades.Num(),
        IsValid(Blade) ? 0 : 1);
    UE_LOG(
        LogRoundBladePickup,
        Log,
        TEXT("PickUpBlade broadcast Character=%s PickedUpFromTile=%s"),
        *GetName(),
        *GetNameSafe(PickedUpFromTile));
    BladePickedUpEvent.Broadcast(PickedUpFromTile);
    const int32 HealOnPickup = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("HealOnBladePickup"))));
    if (HealOnPickup > 0) Heal(HealOnPickup);
    if (Attributes) Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("InitiativeGainOnBladePickup")))));
    const int32 CooldownReduction = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("CooldownReductionOnBladePickup"))));
    if (CooldownReduction > 0)
    {
        for (ARoundAbilityActor* Ability : GrantedAbilities)
        {
            if (Ability) Ability->ReduceCooldown(CooldownReduction);
        }
    }
    NotifyRoundStateChanged();
    return true;
}

void ARoundCharacterBladecaller::CleanupInvalidBlades()
{
    ActiveBlades.RemoveAll([](const ARoundBladeActor* Blade)
    {
        return !IsValid(Blade);
    });
}

void ARoundCharacterBladecaller::RemoveAllBlades()
{
    CleanupInvalidBlades();
    const TArray<TObjectPtr<ARoundBladeActor>> BladesSnapshot = ActiveBlades;
    ActiveBlades.Reset();
    for (ARoundBladeActor* Blade : BladesSnapshot)
    {
        if (Blade)
        {
            Blade->RemoveFromField();
        }
    }
}
