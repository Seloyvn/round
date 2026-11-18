#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RoundTypes.generated.h"

class ARoundCharacter;
class ARoundGridTile;
class ARoundFieldObjectActor;
class UTexture2D;
class UDataTable;
class UClass;

UENUM(BlueprintType)
enum class ERoundTeam : uint8
{
    Neutral,
    TeamA,
    TeamB
};

UENUM(BlueprintType)
enum class ERoundDamageType : uint8
{
    Physical,
    Magical,
    Fire,
    Ice,
    Lightning,
    Holy,
    Ground,
    TrueDamage,
    Nature
};

UENUM(BlueprintType)
enum class ERoundTerrainType : uint8
{
    Ground,
    Forest,
    Thorns,
    Grove,
    Street,
    Fire,
    Rocks,
    Rubble,
    Wall,
      Mountain,
      Sand,
      ShallowWater,
      DeepWater,
      Mud,
      Reeds,
      Dune,
      Oasis,
      Ash,
      Lava,
      Basalt,
      StoneFloor,
      Bridge,
      Gate,
      Meadow,
      DenseForest,
      TallGrass,
      Hedge,
      CanyonFloor,
      Cliff,
      Fog,
      Bog,
      Highland,
      Ridge
};

UENUM(BlueprintType)
enum class ERoundActionType : uint8
{
    None,
    Move,
    Attack,
    Ability,
    ObjectiveClaim,
    Wait,
    EndTurn
};

UENUM(BlueprintType)
enum class ERoundObjectiveType : uint8
{
    HoldZone,
    DestroyNode,
    Channel
};

UENUM(BlueprintType)
enum class ERoundObjectiveState : uint8
{
    Preview,
    Active,
    RewardDropped,
    Completed
};

UENUM(BlueprintType)
enum class ERoundObjectiveRewardType : uint8
{
    Heal,
    AttackAdd,
    DefenseAdd,
    MaxHealthAdd,
    MaxInitiativeAdd
};

UENUM(BlueprintType)
enum class ERoundButtonState : uint8
{
    Hidden,
    Disabled,
    Enabled
};

UENUM(BlueprintType)
enum class ERoundFormat : uint8
{
    Format3x16 UMETA(DisplayName = "3-16"),
    Formattest UMETA(DisplayName = "test"),
    Format5x25 UMETA(DisplayName = "5-25"),
    Format5x16 UMETA(DisplayName = "5-16")
};

UENUM(BlueprintType)
enum class ERoundFieldState : uint8
{
    Any,
    Empty,
    Character,
    Enemy,
    Ally,
    Self
};

UENUM(BlueprintType)
enum class ERoundHoverFieldState : uint8
{
    None,
    Hovered,
    Range,
    Ok,
    NotOk,
    Selected
};

UENUM(BlueprintType)
enum class ERoundCardSuit : uint8
{
    Hearts,
    Spades,
    Clubs,
    Diamonds
};

USTRUCT(BlueprintType)
struct FRoundPlayingCard
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Cards")
    int32 CardInstanceId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Cards")
    ERoundCardSuit Suit = ERoundCardSuit::Hearts;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Cards")
    int32 RankValue = 2;
};

USTRUCT(BlueprintType)
struct FRoundStatBlock
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 MaxHealth = 1000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 Attack = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 Defense = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 MaxInitiative = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 VisionRange = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 AttackRange = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 MoveCost = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 AttackCost = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 TurnDelay = 10;
};

USTRUCT(BlueprintType)
struct FRoundActionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Round")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly, Category = "Round")
    FString Reason;
};

USTRUCT(BlueprintType)
struct FRoundFieldObjectMoveResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Round")
    bool bReachedDestination = false;

    UPROPERTY(BlueprintReadOnly, Category = "Round")
    TArray<TObjectPtr<ARoundGridTile>> TraversedTiles;

    UPROPERTY(BlueprintReadOnly, Category = "Round")
    TArray<TObjectPtr<ARoundCharacter>> HitCharacters;
};

USTRUCT(BlueprintType)
struct FRoundTurnEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<class ARoundCharacter> Character = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 Initiative = 0;
};

USTRUCT(BlueprintType)
struct FRoundTargetingContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<class ARoundCharacter> SourceCharacter = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<class ARoundCharacter> TargetCharacter = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<class ARoundGridTile> TargetTile = nullptr;
};

USTRUCT(BlueprintType)
struct FRoundSkillEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int64 Points = 0;
};

USTRUCT(BlueprintType)
struct FRoundCharacterEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FVector2D StartPosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FString Character;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TArray<FRoundSkillEntry> Skills;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int64 Skillpoints = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int64 CharacterSkillpoints = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int64 CharacterSkillSelections = 0;
};

USTRUCT(BlueprintType)
struct FRoundFormatDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    ERoundFormat Format = ERoundFormat::Format3x16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FString Name = TEXT("New Team");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 Characters = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 Skillpoints = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 FieldSize = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 Terrain = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FVector2D StartPosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 SkillAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TMap<ERoundTerrainType, double> TerrainWeights;
};

USTRUCT(BlueprintType)
struct FRoundFormatTableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    ERoundFormat Format = ERoundFormat::Format3x16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FString Name = TEXT("New Format");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 Characters = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 Skillpoints = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 FieldSize = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 Terrain = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FVector2D StartPosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    int32 SkillAmount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TMap<ERoundTerrainType, double> TerrainWeights;

    FRoundFormatDefinition ToDefinition() const
    {
        FRoundFormatDefinition Result;
        Result.Format = Format;
        Result.Name = Name;
        Result.Characters = Characters;
        Result.Skillpoints = Skillpoints;
        Result.FieldSize = FieldSize;
        Result.Terrain = Terrain;
        Result.StartPosition = StartPosition;
        Result.SkillAmount = SkillAmount;
        Result.TerrainWeights = TerrainWeights;
        return Result;
    }
};

USTRUCT(BlueprintType)
struct FRoundSkillDetails
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<UDataTable> SkillpointDescription = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TMap<FString, double> Values;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    ERoundFieldState FieldState = ERoundFieldState::Any;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<UClass> AbilityClass = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    ERoundDamageType DamageType = ERoundDamageType::Physical;
};

USTRUCT(BlueprintType)
struct FRoundCharacterClassDetails
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    bool Dummy = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TArray<FString> Abilities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<UClass> SkillMenu = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<UTexture2D> InitativeIcon = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FText Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<UClass> CharacterClass = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<UClass> CustomUI = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<UDataTable> Skillpoints = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TMap<FString, double> Details;
};

USTRUCT(BlueprintType)
struct FRoundSkillpointDetails : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FString Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TMap<FString, double> Values;
};

USTRUCT(BlueprintType)
struct FRoundSkillpointsDetails : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TArray<FRoundSkillpointDetails> Skillpoints;
};

USTRUCT(BlueprintType)
struct FRoundSkillpointTier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TArray<FRoundSkillpointDetails> Choices;
};

USTRUCT(BlueprintType)
struct FRoundTeamDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FString Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TArray<FRoundCharacterEntry> Characters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    ERoundFormat Format = ERoundFormat::Format3x16;
};
