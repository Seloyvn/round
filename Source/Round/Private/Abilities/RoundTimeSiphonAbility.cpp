#include "Abilities/RoundTimeSiphonAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacterChronomancer.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"

FRoundActionResult ARoundTimeSiphonAbility::ExecuteResolvedAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    ARoundCharacterChronomancer* Chronomancer = Cast<ARoundCharacterChronomancer>(OwningCharacter);
    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!Chronomancer || !TargetCharacter || !TargetCharacter->Attributes)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    const int32 InitiativeDrainValue = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeDrain")));
    const int32 FragmentsGainedValue = FMath::RoundToInt(GetAbilityValueFromKey(TEXT("FragmentsGained")));
    int32 InitiativeDrain = FMath::Max(0, InitiativeDrainValue > 0 ? InitiativeDrainValue : 40);
    if (Chronomancer->GetFragments() >= Chronomancer->GetMaxFragments())
    {
        InitiativeDrain += FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BonusDrainAtFullFragments"))));
    }
    const int32 FragmentsGained = FMath::Max(0, FragmentsGainedValue > 0 ? FragmentsGainedValue : 1);
    if (InitiativeDrain > 0)
    {
        TargetCharacter->Attributes->PassInitiative(InitiativeDrain);
    }

    if (FragmentsGained > 0)
    {
        Chronomancer->GainFragments(FragmentsGained);
    }

    if (Chronomancer->Attributes)
    {
        Chronomancer->Attributes->GainInitiative(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("SelfInitiativeGain")))));
    }
    const int32 SplashDrain = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("AdjacentInitiativeDrain"))));
    if (SplashDrain > 0)
    {
        if (ARoundGameState* GameState = Chronomancer->GetRoundGameState())
        {
            for (ARoundGridTile* Neighbor : GameState->GetNeighborFields(TargetTile))
            {
                ARoundCharacter* Other = Neighbor ? Neighbor->GetResolvedOccupyingCharacter() : nullptr;
                if (Other && Other->Attributes && Chronomancer->IsEnemyFor(Other)) Other->Attributes->PassInitiative(SplashDrain);
            }
        }
    }

    Result.bSuccess = true;
    Result.Reason = TEXT("Time Siphon executed.");
    return Result;
}
