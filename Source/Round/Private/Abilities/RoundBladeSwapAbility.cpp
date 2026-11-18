#include "Abilities/RoundBladeSwapAbility.h"

#include "Actors/RoundBladeActor.h"
#include "Actors/RoundCharacterBladecaller.h"
#include "Actors/RoundGridTile.h"
#include "Game/RoundGameState.h"
#include "Components/RoundAttributeSetComponent.h"

bool ARoundBladeSwapAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    const ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    if (!Bladecaller || !Bladecaller->CurrentTile || !TargetTile || TargetTile == Bladecaller->CurrentTile)
    {
        return false;
    }

    if (GetAbilityVisibility() != ERoundButtonState::Enabled || !IsTargetInRange(TargetTile))
    {
        return false;
    }

    if (TargetTile->OccupyingCharacter && TargetTile->OccupyingCharacter != Bladecaller)
    {
        return false;
    }

    return Bladecaller->FindBladeAtTile(const_cast<ARoundGridTile*>(TargetTile)) != nullptr;
}

void ARoundBladeSwapAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    const ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    const ARoundGameState* GameState = Bladecaller ? Bladecaller->GetRoundGameState() : nullptr;
    if (!Bladecaller || !GameState || !Bladecaller->CurrentTile || !TargetTile)
    {
        return;
    }

    OutTiles.Append(GameState->GetFieldsBetween(Bladecaller->CurrentTile, const_cast<ARoundGridTile*>(TargetTile), true));
}

FRoundActionResult ARoundBladeSwapAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterBladecaller* Bladecaller = Cast<ARoundCharacterBladecaller>(OwningCharacter);
    ARoundGameState* GameState = Bladecaller ? Bladecaller->GetRoundGameState() : nullptr;
    if (!Bladecaller || !GameState || !Bladecaller->CurrentTile || !TargetTile)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    ARoundBladeActor* Blade = Bladecaller->FindBladeAtTile(TargetTile);
    if (!Blade)
    {
        Result.Reason = TEXT("No blade on target tile.");
        return Result;
    }

    ARoundGridTile* CharacterStartTile = Bladecaller->CurrentTile;
    const int32 SwapDistance = CharacterStartTile->GetDistance(TargetTile);
    TArray<ARoundGridTile*> BladeReturnPath = GameState->GetFieldsBetween(TargetTile, CharacterStartTile, true);
    if (BladeReturnPath.Num() > 0 && BladeReturnPath[0] == TargetTile)
    {
        BladeReturnPath.RemoveAt(0);
    }

    Bladecaller->SetField(TargetTile, true);

    const FRoundFieldObjectMoveResult MoveResult = Blade->MoveAlongPath(BladeReturnPath);
    const int32 DefenseIgnore = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
    for (ARoundCharacter* HitCharacter : MoveResult.HitCharacters)
    {
        if (!HitCharacter || !Bladecaller->IsEnemyFor(HitCharacter))
        {
            continue;
        }

        bool bDidDie = false;
        const double PerTile = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("DamageMultiplierAddPerSwapTile")));
        const int32 Damage = FMath::RoundToInt(GetAbilityDamage() * (1.0 + PerTile * SwapDistance));
        ApplyAbilityDamageToTarget(HitCharacter, Damage, DefenseIgnore, bDidDie);
    }

    const int32 MinimumDistance = FMath::Max(1, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("LongSwapMinimumDistance"))));
    if (Bladecaller->Attributes && SwapDistance >= MinimumDistance)
    {
        Bladecaller->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGainOnLongSwap")))));
    }
    if (GetAbilityValueFromKey(TEXT("RefreshSwappedBladeLifetime")) > 0.0) Blade->SetLifetimeInitiative(Bladecaller->GetBladeLifetimeInitiative());

    if (GameState)
    {
        GameState->OnBoardStateChanged.Broadcast();
    }

    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Swapped with blade and hit %d target(s)."), MoveResult.HitCharacters.Num());
    return Result;
}
