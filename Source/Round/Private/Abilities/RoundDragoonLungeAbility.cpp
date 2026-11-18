#include "Abilities/RoundDragoonLungeAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacterDragoon.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundRootEffect.h"

namespace
{
struct FDragoonCubeCoord
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
};

FDragoonCubeCoord DragoonAxialToCube(const FVector2D& Axial)
{
    return { Axial.X, Axial.Y, -Axial.X - Axial.Y };
}

FDragoonCubeCoord DragoonCubeLerp(const FDragoonCubeCoord& A, const FDragoonCubeCoord& B, const double Alpha)
{
    return { FMath::Lerp(A.X, B.X, Alpha), FMath::Lerp(A.Y, B.Y, Alpha), FMath::Lerp(A.Z, B.Z, Alpha) };
}

FDragoonCubeCoord DragoonCubeRound(const FDragoonCubeCoord& Cube)
{
    int32 RoundedX = FMath::RoundToInt(Cube.X);
    int32 RoundedY = FMath::RoundToInt(Cube.Y);
    int32 RoundedZ = FMath::RoundToInt(Cube.Z);

    const double XDiff = FMath::Abs(static_cast<double>(RoundedX) - Cube.X);
    const double YDiff = FMath::Abs(static_cast<double>(RoundedY) - Cube.Y);
    const double ZDiff = FMath::Abs(static_cast<double>(RoundedZ) - Cube.Z);

    if (XDiff > YDiff && XDiff > ZDiff)
    {
        RoundedX = -RoundedY - RoundedZ;
    }
    else if (YDiff > ZDiff)
    {
        RoundedY = -RoundedX - RoundedZ;
    }
    else
    {
        RoundedZ = -RoundedX - RoundedY;
    }

    return { static_cast<double>(RoundedX), static_cast<double>(RoundedY), static_cast<double>(RoundedZ) };
}

FVector2D DragoonCubeToAxial(const FDragoonCubeCoord& Cube)
{
    return FVector2D(static_cast<float>(Cube.X), static_cast<float>(Cube.Y));
}

TArray<ARoundGridTile*> BuildDragoonLinePath(const ARoundCharacter* Character, const ARoundGridTile* TargetTile, const int32 MaxRange)
{
    TArray<ARoundGridTile*> Result;
    if (!Character || !Character->CurrentTile || !TargetTile)
    {
        return Result;
    }

    const ARoundGameState* GameState = Character->GetRoundGameState();
    if (!GameState)
    {
        return Result;
    }

    const int32 SelectedDistance = Character->CurrentTile->GetDistance(TargetTile);
    if (SelectedDistance <= 0)
    {
        return Result;
    }

    const FDragoonCubeCoord StartCube = DragoonAxialToCube(Character->CurrentTile->GridPosition);
    const FDragoonCubeCoord TargetCube = DragoonAxialToCube(TargetTile->GridPosition);
    const double RangeScale = static_cast<double>(FMath::Max(1, MaxRange)) / static_cast<double>(SelectedDistance);
    const FDragoonCubeCoord ExtendedTargetCube = {
        StartCube.X + ((TargetCube.X - StartCube.X) * RangeScale),
        StartCube.Y + ((TargetCube.Y - StartCube.Y) * RangeScale),
        StartCube.Z + ((TargetCube.Z - StartCube.Z) * RangeScale)
    };

    for (int32 StepIndex = 0; StepIndex < MaxRange; ++StepIndex)
    {
        const double Alpha = static_cast<double>(StepIndex + 1) / static_cast<double>(MaxRange);
        ARoundGridTile* StepTile = GameState->GetField(DragoonCubeToAxial(DragoonCubeRound(DragoonCubeLerp(StartCube, ExtendedTargetCube, Alpha))));
        if (!StepTile)
        {
            break;
        }

        Result.AddUnique(StepTile);
    }

    return Result;
}
}

bool ARoundDragoonLungeAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    return OwningCharacter
        && OwningCharacter->CurrentTile
        && TargetTile
        && TargetTile != OwningCharacter->CurrentTile
        && IsTargetInRange(TargetTile)
        && GetAbilityVisibility() == ERoundButtonState::Enabled;
}

void ARoundDragoonLungeAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    OutTiles.Append(BuildDragoonLinePath(OwningCharacter, TargetTile, GetAbilityRange()));
}

FRoundActionResult ARoundDragoonLungeAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterDragoon* Dragoon = Cast<ARoundCharacterDragoon>(OwningCharacter);
    if (!Dragoon || !Dragoon->CurrentTile || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    const TArray<ARoundGridTile*> FullPath = BuildDragoonLinePath(Dragoon, TargetTile, GetAbilityRange());
    TArray<ARoundGridTile*> TraversedTiles;
    ARoundCharacter* HitEnemy = nullptr;

    for (ARoundGridTile* StepTile : FullPath)
    {
        if (!StepTile)
        {
            break;
        }

        if (StepTile->OccupyingCharacter && Dragoon->IsEnemyFor(StepTile->OccupyingCharacter))
        {
            HitEnemy = StepTile->OccupyingCharacter;
            break;
        }

        if (!StepTile->IsMoveable() || StepTile->HasBlockingFieldObject() || (StepTile->OccupyingCharacter && StepTile->OccupyingCharacter != Dragoon))
        {
            break;
        }

        TraversedTiles.Add(StepTile);
    }

    for (int32 Index = 0; Index < TraversedTiles.Num(); ++Index)
    {
        Dragoon->SetField(TraversedTiles[Index], Index == TraversedTiles.Num() - 1);
    }

    if (HitEnemy)
    {
        bool bDidDie = false;
        const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
        const double DamagePerTile = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("DamageMultiplierAddPerTile")));
        const int32 Damage = FMath::RoundToInt(GetAbilityDamage() * (1.0 + TraversedTiles.Num() * DamagePerTile));
        const int32 AppliedDamage = ApplyAbilityDamageToTarget(HitEnemy, Damage, DefenseIgnore, bDidDie);
        Dragoon->RecordEnemyHit(HitEnemy, AppliedDamage, GetAbilityDamageType());
        Dragoon->GainMomentumStacks(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusMomentumOnHit")))));
        if (Dragoon->Attributes && TraversedTiles.Num() >= GetAbilityRange() - 1) Dragoon->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeRefundAtLongRange")))));
        ReduceCooldown(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionOnHit")))));
        const int32 RootDuration = FMath::Max(0, GetAbilityDurationInitiativeFromKey(TEXT("RootDurationInitiative")));
        if (!bDidDie && RootDuration > 0 && GetWorld())
        {
            if (ARoundRootEffect* Root = GetWorld()->SpawnActor<ARoundRootEffect>(ARoundRootEffect::StaticClass()))
            {
                Root->EffectId = TEXT("LungeRoot"); Root->DisplayName = TEXT("Pinned"); Root->CauserCharacter = Dragoon; Root->SetDurationInitiative(RootDuration, false); HitEnemy->AddEffect(Root);
            }
        }
    }

    Result.bSuccess = TraversedTiles.Num() > 0 || HitEnemy != nullptr;
    Result.Reason = HitEnemy ? TEXT("Lunge hit an enemy.") : TEXT("Lunge executed.");
    return Result;
}
