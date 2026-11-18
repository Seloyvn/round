#include "Core/RoundCoreMatchAdapter.h"

#include "Actors/RoundCharacter.h"
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
    CharacterIds.Reset();

    roundcore::MatchSetup Setup;
    Setup.definitions.contentVersion = "unreal-live-v1";
    Setup.initialState.matchId = static_cast<roundcore::MatchId>(static_cast<uint32>(GameState.GetMatchRandomSeed()));
    Setup.initialState.seed = static_cast<uint32>(GameState.GetMatchRandomSeed());

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

        const ARoundTerrainActor* TerrainActor = Cast<ARoundTerrainActor>(Tile->TerrainObject);
        const bool bWalkable = Tile->IsMoveable();
        const bool bStopsMovement = Tile->TerrainStopsMovement();
        const bool bBlocksVision = TerrainActor ? TerrainActor->BlocksVision() : Tile->bBlocksVision;
        const int32 BaseMoveCost = TerrainActor ? TerrainActor->GetTerrainBaseMoveCost() : FMath::Max(1, Tile->MoveCost);
        const int32 MultiplierBasisPoints = TerrainActor
            ? FMath::Max(0, FMath::RoundToInt(TerrainActor->GetTerrainMoveCostMultiplier() * 10000.0f))
            : 0;
        const FString TerrainKey = FString::Printf(
            TEXT("Terrain:%d:%d:%d:%d:%d:%d"),
            static_cast<int32>(Tile->TerrainType),
            bWalkable ? 1 : 0,
            bBlocksVision ? 1 : 0,
            bStopsMovement ? 1 : 0,
            BaseMoveCost,
            MultiplierBasisPoints);
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
        Definition.stats.attackRange = Stats.AttackRange;
        Definition.stats.moveCost = Character->GetMoveCost();
        Definition.stats.attackCost = Stats.AttackCost;
        Definition.stats.turnDelay = Stats.TurnDelay;
        Setup.definitions.characters.emplace(DefinitionId, std::move(Definition));

        roundcore::CharacterState CharacterState;
        CharacterState.id = CharacterId;
        CharacterState.definitionId = DefinitionId;
        CharacterState.team = ToCoreTeam(Character->Team);
        CharacterState.tileId = TileIds.FindRef(Character->CurrentTile);
        CharacterState.health = Character->Attributes->CurrentHealth;
        CharacterState.initiative = Character->Attributes->CurrentInitiative;
        CharacterState.alive = Character->IsAlive();
        CharacterState.movementBlocked = Character->HasMovementBlock();
        Setup.initialState.characters.push_back(std::move(CharacterState));
    }

    if (const ARoundCharacter* ActiveCharacter = GameState.GetActiveCharacter())
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
