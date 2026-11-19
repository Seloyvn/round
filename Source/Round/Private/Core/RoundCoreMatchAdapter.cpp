#include "Core/RoundCoreMatchAdapter.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundGridTile.h"
#include "Actors/RoundTerrainActor.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"

#include <string>

namespace
{
std::string ToCoreString(const FString& Value)
{
    const FTCHARToUTF8 Converted(*Value);
    return std::string(Converted.Get(), Converted.Length());
}

roundcore::Team ToCoreTeam(const ERoundTeam Team)
{
    switch (Team)
    {
    case ERoundTeam::TeamA:
        return roundcore::Team::TeamA;
    case ERoundTeam::TeamB:
        return roundcore::Team::TeamB;
    default:
        return roundcore::Team::Neutral;
    }
}
}

bool FRoundCoreMatchAdapter::Rebuild(const ARoundGameState& GameState, FString& OutReason)
{
    OutReason.Reset();
    TileIds.Reset();
    TileActors.Reset();
    CharacterIds.Reset();
    CharacterActors.Reset();
    AbilityActors.Reset();

    const int32 MatchSeed = GameState.GetMatchRandomSeed();
    if (LastMatchSeed == MatchSeed)
    {
        PreservedRandomState = CoreGame.State().randomState;
    }
    else
    {
        PreservedRandomState = 0;
        LastMatchSeed = MatchSeed;
    }

    roundcore::MatchSetup Setup;
    Setup.definitions.contentVersion = "unreal-live-v1";
    Setup.initialState.matchId = static_cast<roundcore::MatchId>(static_cast<uint32>(GameState.GetMatchRandomSeed()));
    Setup.initialState.seed = static_cast<uint32>(MatchSeed);
    Setup.initialState.randomState = PreservedRandomState;
    Setup.initialState.globalInitiative = static_cast<uint64>(FMath::Max(0, GameState.GetRecordedGlobalInitiativeElapsed()));

    TArray<ARoundGridTile*> SortedTiles = GameState.GetFields();
    SortedTiles.RemoveAll([](const ARoundGridTile* Tile) { return !IsValid(Tile); });
    SortedTiles.Sort([](const ARoundGridTile& Left, const ARoundGridTile& Right)
    {
        if (Left.GridCoordinate.X != Right.GridCoordinate.X)
        {
            return Left.GridCoordinate.X < Right.GridCoordinate.X;
        }
        if (Left.GridCoordinate.Y != Right.GridCoordinate.Y)
        {
            return Left.GridCoordinate.Y < Right.GridCoordinate.Y;
        }
        return Left.GetName() < Right.GetName();
    });

    roundcore::EntityId NextTileId = 1;
    for (const ARoundGridTile* Tile : SortedTiles)
    {
        const roundcore::EntityId TileId = NextTileId++;
        TileIds.Add(Tile, TileId);
        TileActors.Add(TileId, const_cast<ARoundGridTile*>(Tile));

        const ARoundTerrainActor* TerrainActor = Cast<ARoundTerrainActor>(Tile->TerrainObject);
        const bool bWalkable = Tile->IsMoveable();
        const bool bStopsMovement = Tile->TerrainStopsMovement();
        const bool bBlocksVision = TerrainActor ? TerrainActor->BlocksVision() : Tile->bBlocksVision;
        const int32 BaseMoveCost = TerrainActor ? TerrainActor->GetTerrainBaseMoveCost() : FMath::Max(1, Tile->MoveCost);
        const int32 MultiplierBasisPoints = TerrainActor
            ? FMath::Max(0, FMath::RoundToInt(TerrainActor->GetTerrainMoveCostMultiplier() * 10000.0f))
            : 0;
        const int32 VisionCost = FMath::Max(0, Tile->GetTerrainVisionCost());
        const int32 VisionRangeModifier = Tile->GetTerrainVisionRangeModifier();
        const FString TerrainKey = FString::Printf(
            TEXT("Terrain:%d:%d:%d:%d:%d:%d:%d:%d"),
            static_cast<int32>(Tile->TerrainType),
            bWalkable ? 1 : 0,
            bBlocksVision ? 1 : 0,
            bStopsMovement ? 1 : 0,
            BaseMoveCost,
            MultiplierBasisPoints,
            VisionCost,
            VisionRangeModifier);
        const std::string TerrainId = ToCoreString(TerrainKey);

        if (Setup.definitions.terrains.find(TerrainId) == Setup.definitions.terrains.end())
        {
            roundcore::TerrainDefinition Definition;
            Definition.id = TerrainId;
            Definition.walkable = bWalkable;
            Definition.blocksVision = bBlocksVision;
            Definition.stopsMovement = bStopsMovement;
            Definition.movementCost = BaseMoveCost;
            Definition.movementCostMultiplierBasisPoints = MultiplierBasisPoints;
            Definition.visionCost = VisionCost;
            Definition.visionRangeModifier = VisionRangeModifier;
            Setup.definitions.terrains.emplace(TerrainId, std::move(Definition));
        }

        roundcore::TileState TileState;
        TileState.id = TileId;
        TileState.coord = {Tile->GridCoordinate.X, Tile->GridCoordinate.Y};
        TileState.terrainId = TerrainId;
        TileState.blocksMovement = Tile->HasBlockingFieldObject();
        Setup.initialState.tiles.push_back(std::move(TileState));
    }

    TArray<ARoundCharacter*> SortedCharacters;
    for (ARoundCharacter* Character : GameState.RegisteredCharacters)
    {
        if (IsValid(Character) && Character->CurrentTile && Character->Attributes && TileIds.Contains(Character->CurrentTile))
        {
            SortedCharacters.Add(Character);
        }
    }
    SortedCharacters.Sort([this](const ARoundCharacter& Left, const ARoundCharacter& Right)
    {
        const roundcore::EntityId LeftTile = TileIds.FindRef(Left.CurrentTile);
        const roundcore::EntityId RightTile = TileIds.FindRef(Right.CurrentTile);
        if (LeftTile != RightTile)
        {
            return LeftTile < RightTile;
        }
        if (Left.Team != Right.Team)
        {
            return static_cast<uint8>(Left.Team) < static_cast<uint8>(Right.Team);
        }
        if (Left.CharacterRowName != Right.CharacterRowName)
        {
            return Left.CharacterRowName.LexicalLess(Right.CharacterRowName);
        }
        return Left.GetName() < Right.GetName();
    });

    roundcore::EntityId NextCharacterId = 1;
    for (const ARoundCharacter* Character : SortedCharacters)
    {
        const roundcore::EntityId CharacterId = NextCharacterId++;
        CharacterIds.Add(Character, CharacterId);
        CharacterActors.Add(CharacterId, const_cast<ARoundCharacter*>(Character));

        const FString DefinitionKey = FString::Printf(TEXT("CharacterInstance:%u:%s"), CharacterId, *Character->CharacterRowName.ToString());
        const std::string DefinitionId = ToCoreString(DefinitionKey);
        const FRoundStatBlock& Stats = Character->Attributes->BaseStats;

        roundcore::CharacterDefinition Definition;
        Definition.id = DefinitionId;
        Definition.stats.maxHealth = Stats.MaxHealth;
        Definition.stats.attack = Stats.Attack;
        Definition.stats.defense = Stats.Defense;
        Definition.stats.maxInitiative = Stats.MaxInitiative;
        Definition.stats.visionRange = Stats.VisionRange;
        Definition.stats.attackRange = Character->GetAttackRange();
        Definition.stats.moveCost = Character->GetMoveCost();
        Definition.stats.attackCost = Character->GetAttackCost();
        Definition.stats.turnDelay = Character->GetTurnDelay();
        Definition.criticalChanceBasisPoints = FMath::Clamp(
            FMath::RoundToInt(Character->GetCritChance() * 10000.0), 0, 10000);
        Definition.criticalDamageMultiplierBasisPoints = FMath::Max(
            0, FMath::RoundToInt(Character->GetCritModifier() * 10000.0));
        roundcore::CharacterState CharacterState;
        CharacterState.id = CharacterId;
        CharacterState.definitionId = DefinitionId;
        CharacterState.team = ToCoreTeam(Character->Team);
        CharacterState.tileId = TileIds.FindRef(Character->CurrentTile);
        CharacterState.health = Character->Attributes->CurrentHealth;
        CharacterState.initiative = Character->Attributes->CurrentInitiative;
        CharacterState.alive = Character->IsAlive();
        CharacterState.movementBlocked = Character->HasMovementBlock();
        CharacterState.canTakeTurn = !Character->CharacterRowName.ToString().Contains(TEXT("Puppet"), ESearchCase::IgnoreCase)
            && !GameState.IsCharacterBlockedByObjectiveChannel(Character);
        CharacterState.initiativeGainPerTick = Character->ModifyInitiativeGainFromEffects(1);
        CharacterState.delayedActionInitiative = FMath::Max(0, Character->NativeDelayedActionInitiative);
        for (int32 AbilityIndex = 0; AbilityIndex < Character->GrantedAbilities.Num(); ++AbilityIndex)
        {
            ARoundAbilityActor* Ability = Character->GrantedAbilities[AbilityIndex];
            if (!Ability)
            {
                continue;
            }

            const FString AbilityKey = FString::Printf(
                TEXT("AbilityInstance:%u:%d:%s"), CharacterId, AbilityIndex, *Ability->RowName.ToString());
            const std::string AbilityId = ToCoreString(AbilityKey);
            roundcore::AbilityDefinition AbilityDefinition;
            AbilityDefinition.id = AbilityId;
            for (const TPair<FString, double>& Value : Ability->GetResolvedAbilityValues())
            {
                AbilityDefinition.values.emplace(ToCoreString(Value.Key), Value.Value);
            }
            Setup.definitions.abilities.emplace(AbilityId, std::move(AbilityDefinition));
            Definition.abilityIds.push_back(AbilityId);

            roundcore::AbilityState AbilityState;
            AbilityState.definitionId = AbilityId;
            AbilityState.cooldownRemaining = FMath::Max(0, Ability->CurrentCooldown);
            AbilityState.cooldownDuration = FMath::Max(0, Ability->GetAbilityCooldown());
            AbilityState.currentCharges = FMath::Max(0, Ability->CurrentCharges);
            AbilityState.maxCharges = FMath::Max(1, Ability->GetAbilityMaxCharges());
            AbilityState.usesCharges = Ability->UsesCharges();
            CharacterState.abilities.push_back(std::move(AbilityState));
            AbilityActors.Add(AbilityKey, Ability);
        }
        Setup.definitions.characters.emplace(DefinitionId, std::move(Definition));
        Setup.initialState.characters.push_back(std::move(CharacterState));
    }

    // During Core-driven initiative advancement there is intentionally no
    // active character. GetActiveCharacter() falls back to the UI turn-order
    // index, so only the replicated authoritative pointer may seed Core here.
    if (const ARoundCharacter* ActiveCharacter = GameState.CurrentTurnCharacter)
    {
        Setup.initialState.activeCharacterId = CharacterIds.FindRef(ActiveCharacter);
    }

    const roundcore::ActionResult Result = CoreGame.Reset(std::move(Setup));
    if (!Result.success)
    {
        OutReason = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(Result.errorCode.c_str()), UTF8_TO_TCHAR(Result.message.c_str()));
        return false;
    }
    return true;
}

bool FRoundCoreMatchAdapter::IsBasicAttackLegal(
    const ARoundCharacter* Character,
    const ARoundCharacter* TargetCharacter,
    FString& OutReason) const
{
    const roundcore::EntityId* CharacterId = CharacterIds.Find(Character);
    const roundcore::EntityId* TargetId = CharacterIds.Find(TargetCharacter);
    if (!CharacterId || !TargetId)
    {
        OutReason = TEXT("Attacker or target is not part of the Core snapshot.");
        return false;
    }

    roundcore::GameAction Action;
    Action.type = roundcore::ActionType::BasicAttack;
    Action.actorId = *CharacterId;
    Action.targetEntityId = *TargetId;
    const roundcore::ActionResult Result = CoreGame.ValidateAction(Action);
    OutReason = Result.success
        ? TEXT("Basic attack is legal in RoundCore.")
        : FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(Result.errorCode.c_str()), UTF8_TO_TCHAR(Result.message.c_str()));
    return Result.success;
}

bool FRoundCoreMatchAdapter::GetVisibleTiles(
    const ARoundCharacter* Character,
    TArray<ARoundGridTile*>& OutVisibleTiles,
    FString& OutReason) const
{
    OutVisibleTiles.Reset();
    const roundcore::EntityId* CharacterId = CharacterIds.Find(Character);
    if (!CharacterId)
    {
        OutReason = TEXT("Character is not part of the Core snapshot.");
        return false;
    }
    for (const roundcore::EntityId TileId : CoreGame.VisibleTileIds(*CharacterId))
    {
        if (ARoundGridTile* const* Tile = TileActors.Find(TileId))
        {
            OutVisibleTiles.Add(*Tile);
        }
    }
    OutReason = TEXT("Visibility resolved by RoundCore.");
    return true;
}

bool FRoundCoreMatchAdapter::ExecuteTurnStep(
    const ARoundCharacter* EndingCharacter,
    TArray<TPair<ARoundCharacter*, int32>>& OutInitiatives,
    TArray<FRoundCoreAbilityRuntimeUpdate>& OutAbilityUpdates,
    ARoundCharacter*& OutActiveCharacter,
    int32& OutGlobalInitiativeAdvance,
    FString& OutReason)
{
    OutInitiatives.Reset();
    OutAbilityUpdates.Reset();
    OutActiveCharacter = nullptr;
    OutGlobalInitiativeAdvance = 0;

    roundcore::GameAction Action;
    Action.type = roundcore::ActionType::EndTurn;
    if (EndingCharacter)
    {
        const roundcore::EntityId* CharacterId = CharacterIds.Find(EndingCharacter);
        if (!CharacterId)
        {
            OutReason = TEXT("Ending character is not part of the Core snapshot.");
            return false;
        }
        Action.actorId = *CharacterId;
    }

    const roundcore::ActionResult Result = CoreGame.ApplyAction(Action);
    if (!Result.success)
    {
        OutReason = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(Result.errorCode.c_str()), UTF8_TO_TCHAR(Result.message.c_str()));
        return false;
    }

    for (const roundcore::GameEvent& Event : Result.events)
    {
        if (Event.type == roundcore::EventType::GlobalInitiativeAdvanced)
        {
            OutGlobalInitiativeAdvance += static_cast<int32>(Event.amount);
        }
    }
    for (const roundcore::CharacterState& State : CoreGame.State().characters)
    {
        if (ARoundCharacter* const* Character = CharacterActors.Find(State.id))
        {
            OutInitiatives.Emplace(*Character, State.initiative);
        }
        for (const roundcore::AbilityState& AbilityState : State.abilities)
        {
            const FString AbilityKey = UTF8_TO_TCHAR(AbilityState.definitionId.c_str());
            if (ARoundAbilityActor* const* Ability = AbilityActors.Find(AbilityKey))
            {
                FRoundCoreAbilityRuntimeUpdate& Update = OutAbilityUpdates.AddDefaulted_GetRef();
                Update.Ability = *Ability;
                Update.Cooldown = AbilityState.cooldownRemaining;
                Update.Charges = AbilityState.currentCharges;
            }
        }
    }
    if (ARoundCharacter* const* Active = CharacterActors.Find(CoreGame.State().activeCharacterId))
    {
        OutActiveCharacter = *Active;
    }
    OutReason = UTF8_TO_TCHAR(Result.message.c_str());
    return true;
}

bool FRoundCoreMatchAdapter::IsMoveLegal(
    const ARoundCharacter* Character,
    const ARoundGridTile* TargetTile,
    FString& OutReason) const
{
    const roundcore::EntityId* CharacterId = CharacterIds.Find(Character);
    const roundcore::EntityId* TileId = TileIds.Find(TargetTile);
    if (!CharacterId || !TileId)
    {
        OutReason = TEXT("Character or target tile is not part of the Core snapshot.");
        return false;
    }

    roundcore::GameAction Action;
    Action.type = roundcore::ActionType::Move;
    Action.actorId = *CharacterId;
    Action.targetTileId = *TileId;
    const roundcore::ActionResult Result = CoreGame.ValidateAction(Action);
    OutReason = Result.success
        ? TEXT("Movement is legal in RoundCore.")
        : FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(Result.errorCode.c_str()), UTF8_TO_TCHAR(Result.message.c_str()));
    return Result.success;
}

bool FRoundCoreMatchAdapter::ExecuteMove(
    const ARoundCharacter* Character,
    const ARoundGridTile* TargetTile,
    int32& OutMovementCost,
    int32& OutInitiative,
    FString& OutReason)
{
    OutMovementCost = 0;
    OutInitiative = 0;
    const roundcore::EntityId* CharacterId = CharacterIds.Find(Character);
    const roundcore::EntityId* TileId = TileIds.Find(TargetTile);
    if (!CharacterId || !TileId)
    {
        OutReason = TEXT("Character or target tile is not part of the Core snapshot.");
        return false;
    }

    roundcore::GameAction Action;
    Action.type = roundcore::ActionType::Move;
    Action.actorId = *CharacterId;
    Action.targetTileId = *TileId;
    const roundcore::ActionResult Result = CoreGame.ApplyAction(Action);
    if (!Result.success)
    {
        OutReason = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(Result.errorCode.c_str()), UTF8_TO_TCHAR(Result.message.c_str()));
        return false;
    }

    for (const roundcore::GameEvent& Event : Result.events)
    {
        if (Event.type == roundcore::EventType::CharacterMoved && Event.sourceId == *CharacterId)
        {
            OutMovementCost = static_cast<int32>(Event.amount);
            break;
        }
    }
    for (const roundcore::CharacterState& State : CoreGame.State().characters)
    {
        if (State.id == *CharacterId)
        {
            OutInitiative = State.initiative;
            OutReason = TEXT("Movement executed by RoundCore.");
            return true;
        }
    }

    OutReason = TEXT("RoundCore executed movement but returned no character state.");
    return false;
}
