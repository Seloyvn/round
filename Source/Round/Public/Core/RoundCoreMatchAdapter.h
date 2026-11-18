#pragma once

#include "CoreMinimal.h"
#include "round/core/Game.h"

class ARoundCharacter;
class ARoundGameState;
class ARoundGridTile;

// Converts the live Unreal board to the standard-C++ state used by gameplay,
// training and deterministic tests. This adapter contains no gameplay rules.
class ROUND_API FRoundCoreMatchAdapter
{
public:
    bool Rebuild(const ARoundGameState& GameState, FString& OutReason);
    bool IsMoveLegal(const ARoundCharacter* Character, const ARoundGridTile* TargetTile, FString& OutReason) const;

    const roundcore::Game& GetGame() const { return CoreGame; }

private:
    roundcore::Game CoreGame;
    TMap<const ARoundGridTile*, roundcore::EntityId> TileIds;
    TMap<const ARoundCharacter*, roundcore::EntityId> CharacterIds;
};
