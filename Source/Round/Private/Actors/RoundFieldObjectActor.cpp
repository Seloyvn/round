#include "Actors/RoundFieldObjectActor.h"

#include "Actors/RoundGridTile.h"
#include "Components/SceneComponent.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundFieldObjectRuntime, Log, All);

ARoundFieldObjectActor::ARoundFieldObjectActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(true);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
}

void ARoundFieldObjectActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bAnimateMovement)
    {
        return;
    }

    const FVector CurrentLocation = GetActorLocation();
    const FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, TargetWorldLocation, DeltaSeconds, FMath::Max(1.0f, MoveSpeed));
    SetActorLocation(NewLocation);

    if (NewLocation.Equals(TargetWorldLocation, 1.0f))
    {
        SetActorLocation(TargetWorldLocation);
        bAnimateMovement = false;
    }
}

void ARoundFieldObjectActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundFieldObjectActor, CurrentTile);
    DOREPLIFETIME(ARoundFieldObjectActor, OwningCharacter);
    DOREPLIFETIME(ARoundFieldObjectActor, RemainingInitiative);
}

void ARoundFieldObjectActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (CurrentTile && CurrentTile->OccupyingFieldObject == this)
    {
        CurrentTile->OccupyingFieldObject = nullptr;
        CurrentTile->ForceNetUpdate();
    }

    CurrentTile = nullptr;
    Super::EndPlay(EndPlayReason);
}

bool ARoundFieldObjectActor::CanOccupyTile(const ARoundGridTile* Tile) const
{
    return Tile && (!Tile->OccupyingFieldObject || Tile->OccupyingFieldObject == this);
}

bool ARoundFieldObjectActor::SetField(ARoundGridTile* NewTile)
{
    if (!NewTile || !CanOccupyTile(NewTile))
    {
        return false;
    }

    if (CurrentTile == NewTile)
    {
        return true;
    }

    if (CurrentTile && CurrentTile->OccupyingFieldObject == this)
    {
        CurrentTile->OccupyingFieldObject = nullptr;
        CurrentTile->ForceNetUpdate();
    }

    const bool bHadPreviousTile = CurrentTile != nullptr;
    CurrentTile = NewTile;
    CurrentTile->OccupyingFieldObject = this;
    CurrentTile->ForceNetUpdate();
    if (!bHadPreviousTile)
    {
        SnapToCurrentTile();
    }
    else
    {
        StartMoveToCurrentTile();
    }
    ForceNetUpdate();
    return true;
}

void ARoundFieldObjectActor::RemoveFromField()
{
    UE_LOG(
        LogRoundFieldObjectRuntime,
        Log,
        TEXT("RemoveFromField Object=%s CurrentTile=%s Owner=%s Hidden=%d"),
        *GetName(),
        *GetNameSafe(CurrentTile),
        *GetNameSafe(OwningCharacter),
        IsHidden() ? 1 : 0);

    if (CurrentTile && CurrentTile->OccupyingFieldObject == this)
    {
        CurrentTile->OccupyingFieldObject = nullptr;
        CurrentTile->ForceNetUpdate();
    }

    CurrentTile = nullptr;
    ForceNetUpdate();
    UE_LOG(LogRoundFieldObjectRuntime, Log, TEXT("RemoveFromField destroy Object=%s"), *GetName());
    Destroy();
}

void ARoundFieldObjectActor::SetLifetimeInitiative(int32 DurationInitiative)
{
    RemainingInitiative = FMath::Max(0, DurationInitiative);
    ForceNetUpdate();
}

void ARoundFieldObjectActor::OnInitiativeAdvanced(int32 InitiativeAmount)
{
    if (InitiativeAmount <= 0 || RemainingInitiative <= 0)
    {
        return;
    }

    RemainingInitiative = FMath::Max(0, RemainingInitiative - InitiativeAmount);
    if (RemainingInitiative <= 0)
    {
        RemoveFromField();
        return;
    }

    ForceNetUpdate();
}

void ARoundFieldObjectActor::OnRep_CurrentTile()
{
    if (!CurrentTile)
    {
        bAnimateMovement = false;
        return;
    }

    if (!bHasBeenPlacedOnTile)
    {
        SnapToCurrentTile();
        return;
    }

    StartMoveToCurrentTile();
}

FVector ARoundFieldObjectActor::GetTileWorldLocation(const ARoundGridTile* Tile) const
{
    if (!Tile)
    {
        return FVector::ZeroVector;
    }

    FVector Location = Tile->GetActorLocation();
    Location.Z += FieldObjectHeightOffset;
    return Location;
}

void ARoundFieldObjectActor::SnapToCurrentTile()
{
    if (!CurrentTile)
    {
        return;
    }

    TargetWorldLocation = GetTileWorldLocation(CurrentTile);
    bAnimateMovement = false;
    bHasBeenPlacedOnTile = true;
    SetActorLocation(TargetWorldLocation);
}

void ARoundFieldObjectActor::StartMoveToCurrentTile()
{
    if (!CurrentTile)
    {
        return;
    }

    TargetWorldLocation = GetTileWorldLocation(CurrentTile);
    bAnimateMovement = true;
}
