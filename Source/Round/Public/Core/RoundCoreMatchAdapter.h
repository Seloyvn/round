#pragma once

#include "CoreMinimal.h"
#include "round/core/Game.h"

class ARoundCharacter;
class ARoundGameState;
class ARoundGridTile;
class ARoundAbilityActor;

struct FRoundCoreAbilityRuntimeUpdate
{
    ARoundAbilityActor* Ability = nullptr;
    int32 Cooldown = 0;
    int32 Charges = 0;
};

struct FRoundCoreBasicAttackPreview
{
    int32 InitiativeCost = 0;
    int32 Damage = 0;
    bool bTargetDied = false;
    bool bMatchEnded = false;
    bool bCritical = false;
};

struct FRoundCoreCharacterRuntimeUpdate
{
    ARoundCharacter* Character = nullptr;
    int32 Health = 0;
    int32 Initiative = 0;
    bool bAlive = false;
};

struct FRoundCoreEffectRemoval
{
    ARoundCharacter* Character = nullptr;
    FString EffectId;
};

struct FRoundCoreBasicAttackExecution
{
    TArray<FRoundCoreCharacterRuntimeUpdate> CharacterUpdates;
    TArray<FRoundCoreAbilityRuntimeUpdate> AbilityUpdates;
    TArray<FRoundCoreEffectRemoval> RemovedEffects;
    int32 AppliedDamage = 0;
    bool bCritical = false;
    bool bTargetDied = false;
};

struct FRoundCoreAbilityExecution
{
    TArray<FRoundCoreCharacterRuntimeUpdate> CharacterUpdates;
    TArray<FRoundCoreAbilityRuntimeUpdate> AbilityUpdates;
    TArray<FRoundCoreEffectRemoval> RemovedEffects;
    int32 AppliedDamage = 0;
    int32 AppliedHealing = 0;
    bool bTargetDied = false;
};

// Converts the live Unreal board to the standard-C++ state used by gameplay,
// training and deterministic tests. This adapter contains no gameplay rules.
class ROUND_API FRoundCoreMatchAdapter
{
public:
    bool Rebuild(const ARoundGameState& GameState, FString& OutReason);
    bool IsMoveLegal(const ARoundCharacter* Character, const ARoundGridTile* TargetTile, FString& OutReason) const;
    bool IsBasicAttackLegal(const ARoundCharacter* Character, const ARoundCharacter* TargetCharacter, FString& OutReason) const;
    bool PreviewBasicAttack(const ARoundCharacter* Character, const ARoundCharacter* TargetCharacter,
        FRoundCoreBasicAttackPreview& OutPreview, FString& OutReason) const;
    bool ResolveBasicAttackCritical(const ARoundCharacter* Character, bool& bOutCritical, FString& OutReason);
    bool ExecuteBasicAttack(
        const ARoundCharacter* Character,
        const ARoundCharacter* TargetCharacter,
        FRoundCoreBasicAttackExecution& OutExecution,
        FString& OutReason);
    bool ExecuteAbility(
        const ARoundCharacter* Character,
        const ARoundAbilityActor* Ability,
        const ARoundCharacter* TargetCharacter,
        FRoundCoreAbilityExecution& OutExecution,
        FString& OutReason);
    bool GetVisibleTiles(
        const ARoundCharacter* Character,
        TArray<ARoundGridTile*>& OutVisibleTiles,
        FString& OutReason) const;
    bool ExecuteMove(
        const ARoundCharacter* Character,
        const ARoundGridTile* TargetTile,
        int32& OutMovementCost,
        int32& OutInitiative,
        FString& OutReason);
    bool ExecuteTurnStep(
        const ARoundCharacter* EndingCharacter,
        TArray<TPair<ARoundCharacter*, int32>>& OutInitiatives,
        TArray<FRoundCoreAbilityRuntimeUpdate>& OutAbilityUpdates,
        ARoundCharacter*& OutActiveCharacter,
        int32& OutGlobalInitiativeAdvance,
        FString& OutReason);

    const roundcore::Game& GetGame() const { return CoreGame; }

private:
    roundcore::Game CoreGame;
    TMap<const ARoundGridTile*, roundcore::EntityId> TileIds;
    TMap<roundcore::EntityId, ARoundGridTile*> TileActors;
    TMap<const ARoundCharacter*, roundcore::EntityId> CharacterIds;
    TMap<roundcore::EntityId, ARoundCharacter*> CharacterActors;
    TMap<FString, ARoundAbilityActor*> AbilityActors;
    int32 LastMatchSeed = 0;
    std::uint64_t PreservedRandomState = 0;
};
