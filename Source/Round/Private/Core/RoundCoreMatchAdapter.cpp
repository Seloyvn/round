#include "Core/RoundCoreMatchAdapter.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundGridTile.h"
#include "Actors/RoundTerrainActor.h"
#include "Actors/RoundEngineerDeviceActor.h"
#include "Actors/RoundCharacterEngineer.h"
#include "Actors/RoundCharacterChronomancer.h"
#include "Actors/RoundTurretDeviceActor.h"
#include "Actors/RoundMineDeviceActor.h"
#include "Actors/RoundShieldEmitterDeviceActor.h"
#include "Actors/RoundSpotterBeaconDeviceActor.h"
#include "Abilities/RoundBuildTurretAbility.h"
#include "Abilities/RoundBuildMineAbility.h"
#include "Abilities/RoundBuildShieldEmitterAbility.h"
#include "Abilities/RoundBuildSpotterBeaconAbility.h"
#include "Actors/RoundCharacterPaladin.h"
#include "Actors/RoundCharacter1.h"
#include "Actors/RoundCharacter2.h"
#include "Actors/RoundCharacter3.h"
#include "Actors/RoundCharacter4.h"
#include "Actors/RoundCharacter5.h"
#include "Abilities/RoundSnipeAbility.h"
#include "Effects/RoundModifierEffect.h"
#include "Effects/RoundBasicAttackDamageReductionEffect.h"
#include "Effects/RoundSuppressingFireEffect.h"
#include "Effects/RoundSilenceEffect.h"
#include "Effects/RoundRootEffect.h"
#include "Effects/RoundSlowEffect.h"
#include "Effects/RoundIcebeamEffect.h"
#include "Effects/RoundShieldOathEffect.h"
#include "Effects/RoundSanctifiedGroundAuraEffect.h"
#include "Effects/RoundBindEffect.h"
#include "Effects/RoundTauntEffect.h"
#include "Effects/RoundInvisibilityEffect.h"
#include "Effects/RoundAssassinHuntMarkEffect.h"
#include "Effects/RoundShadowWoundEffect.h"
#include "Effects/RoundDeathMarkEffect.h"
#include "Effects/RoundSharedBreachEffect.h"
#include "Effects/RoundBleedEffect.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"

#include <algorithm>
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

roundcore::DamageType ToCoreDamageType(const ERoundDamageType DamageType)
{
    switch (DamageType)
    {
    case ERoundDamageType::Magical: return roundcore::DamageType::Magical;
    case ERoundDamageType::Fire: return roundcore::DamageType::Fire;
    case ERoundDamageType::Ice: return roundcore::DamageType::Ice;
    case ERoundDamageType::Lightning: return roundcore::DamageType::Lightning;
    case ERoundDamageType::Holy: return roundcore::DamageType::Holy;
    case ERoundDamageType::Ground: return roundcore::DamageType::Ground;
    case ERoundDamageType::TrueDamage: return roundcore::DamageType::TrueDamage;
    case ERoundDamageType::Nature: return roundcore::DamageType::Nature;
    default: return roundcore::DamageType::Physical;
    }
}

ERoundDamageType ToRoundDamageType(const roundcore::DamageType DamageType)
{
    switch (DamageType)
    {
    case roundcore::DamageType::Magical: return ERoundDamageType::Magical;
    case roundcore::DamageType::Fire: return ERoundDamageType::Fire;
    case roundcore::DamageType::Ice: return ERoundDamageType::Ice;
    case roundcore::DamageType::Lightning: return ERoundDamageType::Lightning;
    case roundcore::DamageType::Holy: return ERoundDamageType::Holy;
    case roundcore::DamageType::Ground: return ERoundDamageType::Ground;
    case roundcore::DamageType::TrueDamage: return ERoundDamageType::TrueDamage;
    case roundcore::DamageType::Nature: return ERoundDamageType::Nature;
    default: return ERoundDamageType::Physical;
    }
}

void CopyCoreEffectMetadata(const roundcore::ModifierEffectState& State, FRoundCoreModifierEffectAddition& Addition)
{
    Addition.SourceAbilityId = UTF8_TO_TCHAR(State.sourceAbilityId.c_str());
    Addition.CoreEffectKind = static_cast<int32>(State.kind);
    Addition.CounterA = State.counterA; Addition.CounterB = State.counterB; Addition.CounterC = State.counterC;
    Addition.ValueA = State.valueA; Addition.ValueB = State.valueB; Addition.ValueC = State.valueC;
    Addition.Flags = static_cast<int32>(State.flags);
}

void CopyExtendedCharacterState(const roundcore::CharacterState& State, FRoundCoreCharacterRuntimeUpdate& Update)
{
    Update.bFirstResourceDiscountAvailable = State.firstResourceDiscountAvailable;
    Update.bAttackedThisTurn = State.attackedThisTurn;
    Update.bSpentResourceThisTurn = State.spentResourceThisTurn;
    Update.bStartedTurnWithFullResource = State.startedTurnWithFullResource;
    Update.bHeldPositionFromPreviousTurn = State.heldPositionFromPreviousTurn;
    Update.bUsedFreeBasicAttackThisTurn = State.usedFreeBasicAttackThisTurn;
    Update.bTriggeredEmptyResourceThisTurn = State.triggeredEmptyResourceThisTurn;
    Update.bRefundedLastResourceKillThisTurn = State.refundedLastResourceKillThisTurn;
    Update.bUsedRangeRefundThisTurn = State.usedRangeRefundThisTurn;
    Update.bUsedKillCooldownReductionThisTurn = State.usedKillCooldownReductionThisTurn;
    Update.StationaryShotStacks = State.stationaryShotStacks;
    Update.bHasFreePreparedStep = State.hasFreePreparedStep;
    Update.MovedFieldsThisTurn = State.movedFieldsThisTurn;
    Update.ResourceSpentThisTurnCount = State.resourceSpentThisTurnCount;
    Update.SecondaryCooldownRemaining = State.secondaryCooldownRemaining;
    Update.TurnSequence = State.turnSequence;
    Update.bOpeningRewardUsedThisTurn = State.openingRewardUsedThisTurn;
    Update.bOverflowRewardUsedThisTurn = State.overflowRewardUsedThisTurn;
    Update.bFreeAdjacentMoveAvailable = State.freeAdjacentMoveAvailable;
    Update.bFreeAdjacentMoveUsedThisTurn = State.freeAdjacentMoveUsedThisTurn;
    Update.bNextDamageAfterMoveReady = State.nextDamageAfterMoveReady;
    Update.bNextReductionAfterMoveReady = State.nextReductionAfterMoveReady;
    Update.bStartInvisibilityApplied = State.startInvisibilityApplied;
    Update.ActiveElement = State.activeElement; Update.UsedElementMask = State.usedElementMask;
    Update.LastCastElement = State.lastCastElement; Update.FirstCastElement = State.firstCastElement;
    Update.ElementSwapCooldownRemaining = State.elementSwapCooldownRemaining;
    Update.RockDefenseBonus = State.rockDefenseBonus; Update.RockDefenseRemainingInitiative = State.rockDefenseRemainingInitiative;
    Update.ProtectionRemainingInitiative = State.protectionRemainingInitiative; Update.EchoElement = State.echoElement;
    Update.ElementCycleMask = State.elementCycleMask; Update.bElementCostReductionUsed = State.elementCostReductionUsed;
    Update.bElementSwapRefundUsed = State.elementSwapRefundUsed; Update.bStoredElementEmpowered = State.storedElementEmpowered;
    Update.bProtectionReady = State.protectionReady;
    Update.bFirstBloodDamageReductionAvailable = State.firstBloodDamageReductionAvailable;
    Update.bBasicAttackBloodAvailable = State.basicAttackBloodAvailable;
    Update.bBloodCapTriggerAvailable = State.bloodCapTriggerAvailable;
}
}

bool FRoundCoreMatchAdapter::Rebuild(const ARoundGameState& GameState, FString& OutReason)
{
    OutReason.Reset();
    TMap<const ARoundCharacter*, TArray<roundcore::ModifierEffectState>> PreservedCoreEffects;
    for (const roundcore::CharacterState& State : CoreGame.State().characters)
    {
        ARoundCharacter* const* Owner = CharacterActors.Find(State.id);
        if (!Owner)
        {
            continue;
        }
        PreservedCoreEffects.Add(*Owner, TArray<roundcore::ModifierEffectState>(State.modifierEffects.data(),
            static_cast<int32>(State.modifierEffects.size())));
        for (const roundcore::RecentAuraPreventionState& Prevention : State.recentAuraPreventions)
        {
            if (ARoundCharacter* const* Source = CharacterActors.Find(Prevention.sourceId))
            {
                PreservedAuraPreventions.FindOrAdd(*Owner).FindOrAdd(*Source) = Prevention.globalInitiative;
            }
        }
    }
    TileIds.Reset();
    TileActors.Reset();
    CharacterIds.Reset();
    CharacterActors.Reset();
    AbilityActors.Reset();
    DeviceIds.Reset();
    DeviceActors.Reset();

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
        TileState.blocksMovement = Tile->HasBlockingFieldObject()
            && !Cast<ARoundEngineerDeviceActor>(Tile->OccupyingFieldObject);
        if (const ARoundEngineerDeviceActor* Device = Cast<ARoundEngineerDeviceActor>(Tile->OccupyingFieldObject))
        {
            TileState.fieldObjectHealth = FMath::Max(0, Device->GetCurrentHealth());
        }
        if (TerrainActor && TerrainActor->IsDestructible())
        {
            TileState.terrainObjectHealth = FMath::Max(0, TerrainActor->CurrentHealth);
        }
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
    for (ARoundCharacter* Character : SortedCharacters)
    {
        const roundcore::EntityId CharacterId = NextCharacterId++;
        CharacterIds.Add(Character, CharacterId);
        CharacterActors.Add(CharacterId, Character);
    }
    for (const ARoundCharacter* Character : SortedCharacters)
    {
        const roundcore::EntityId CharacterId = CharacterIds.FindRef(Character);

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
        Definition.stats.moveCost = Stats.MoveCost > 0
            ? Stats.MoveCost
            : FMath::Max(1, FMath::TruncToInt(Character->GetValueFromKey(TEXT("Movecost"))));
        Definition.stats.attackCost = Stats.AttackCost > 0
            ? Stats.AttackCost
            : FMath::Max(0, FMath::TruncToInt(Character->GetValueFromKey(TEXT("AttackCost"))));
        Definition.stats.turnDelay = Character->GetTurnDelay();
        Definition.criticalChanceBasisPoints = FMath::Clamp(
            FMath::RoundToInt(Character->GetCritChance() * 10000.0), 0, 10000);
        Definition.criticalDamageMultiplierBasisPoints = FMath::Max(
            0, FMath::RoundToInt(Character->GetCritModifier() * 10000.0));
        if (const ARoundCharacterPaladin* Paladin = Cast<ARoundCharacterPaladin>(Character))
        {
            // Resolved editor-authored values enter a generic Core aura contract.
            Definition.passiveValues.emplace("CoreIncomingDamageAuraReduction", Paladin->GetAuraDamageReduction());
            Definition.passiveValues.emplace("CoreIncomingDamageAuraRadius", Paladin->GetAuraRadius());
            Definition.passiveValues.emplace(
                "CoreIncomingDamageAuraAffectSelf",
                Paladin->GetPassiveValueFromKey(TEXT("AuraAffectSelf")) > 0.0 ? 1.0 : 0.0);
            Definition.passiveValues.emplace(
                "CoreAuraHealFractionOfPreventedDamage",
                FMath::Max(0.0, Paladin->GetPassiveValueFromKey(TEXT("HealFractionOfAuraPreventedDamage"))));
            Definition.passiveValues.emplace(
                "CoreAuraInitiativeOnPrevent",
                FMath::Max(0.0, Paladin->GetPassiveValueFromKey(TEXT("InitiativeOnAuraPrevent"))));
            Definition.passiveValues.emplace(
                "CoreAuraCooldownReductionOnPrevent",
                FMath::Max(0.0, Paladin->GetPassiveValueFromKey(TEXT("CooldownReductionOnAuraPrevent"))));
        }
        if (const ARoundCharacter1* Character1 = Cast<ARoundCharacter1>(Character))
        {
            static const TCHAR* PassiveKeys[] = {
                TEXT("FirstChainSpendInitiativeRefund"), TEXT("ZeroChainTurnStartGainChain"),
                TEXT("ZeroChainTurnStartInitiativePenalty"), TEXT("IncomingDamageMultiplierFromDebuffedEnemies"),
                TEXT("DamageMultiplierWhileHasChain"), TEXT("ChainOverflowInitiativeGain"),
                TEXT("DebuffDurationMultiplier"), TEXT("CheatDeathHealPerConsumedChain"),
                TEXT("CheatDeathCooldownInitiative"), TEXT("DelayReceivedMultiplier"),
                TEXT("IgnoreEnemyForcedMovementChance"), TEXT("DefenseWhileNoChain"),
                TEXT("StationaryIncomingDamageMultiplier")};
            for (const TCHAR* Key : PassiveKeys)
            {
                Definition.passiveValues.emplace(ToCoreString(Key), Character1->GetPassiveValueFromKey(Key));
            }
        }
        if (const ARoundCharacter2* Character2 = Cast<ARoundCharacter2>(Character))
        {
            static const TCHAR* PassiveKeys[] = {
                TEXT("AmmoRegenAdd"), TEXT("MaxAmmoAdd"), TEXT("RangedDamageMultiplier"),
                TEXT("FirstBasicAttackFreeAmmo"), TEXT("TurnDelayAddWhileFullAmmo"),
                TEXT("DefenseAddWhileHoldingPosition"), TEXT("RefillAmmoOnEmptyTurnStart"),
                TEXT("RefillAmmoInitiativePenalty"), TEXT("EmptyMagazineInitiativeGain"),
                TEXT("FullAmmoTurnStartAbilityCooldownReduction"), TEXT("LastAmmoKillRefund"),
                TEXT("StationaryShotDamagePerStack"), TEXT("StationaryShotMaxStacks"),
                TEXT("AllDamageMultiplierWhileOneAmmo")};
            for (const TCHAR* Key : PassiveKeys)
                Definition.passiveValues.emplace(ToCoreString(Key), Character2->GetPassiveValueFromKey(Key));
            Definition.passiveValues.emplace("CoreAmmoCharacter", 1.0);
            Definition.passiveValues.emplace("BaseAmmoRegen", Character2->GetValueFromKey(TEXT("AmmoRegen")));
        }
        if (const ARoundCharacter3* Character3 = Cast<ARoundCharacter3>(Character))
        {
            static const TCHAR* PassiveKeys[] = {
                TEXT("MaxComboPointsAdd"), TEXT("ComboPointsStartAdd"), TEXT("ComboOnFullHealthHit"),
                TEXT("FirstBasicAttackCostReduction"), TEXT("MoveDistanceForBonuses"),
                TEXT("NextHitDamageMultiplierAfterMove"), TEXT("FreeAdjacentMoveAfterDamage"),
                TEXT("NextIncomingDamageMultiplierAfterMove"), TEXT("ComboOverflowInitiativeGain"),
                TEXT("ComboSpendInitiativeGainPerPoint"), TEXT("ComboOnEmptyTurnStart"),
                TEXT("InvisibilityOnKillDurationInitiative"), TEXT("ComboOnKill"),
                TEXT("HuntMarkDamageMultiplier"), TEXT("HuntMarkDurationInitiative"),
                TEXT("HuntMarkCooldownAfterExpiryInitiative")};
            for (const TCHAR* Key : PassiveKeys)
                Definition.passiveValues.emplace(ToCoreString(Key), Character3->GetPassiveValueFromKey(Key));
            Definition.passiveValues.emplace("CoreComboCharacter", 1.0);
            Definition.passiveValues.emplace("BaseComboMaximum", Character3->GetValueFromKey(TEXT("MaxComboPoints")));
            Definition.passiveValues.emplace("BaseComboStart", Character3->GetValueFromKey(TEXT("ComboPointsStart")));
            Definition.passiveValues.emplace("StartInvisibilityDurationInitiative", Character3->GetValueFromKey(TEXT("StartInvisibilityDurationInitiative")));
        }
        if (const ARoundCharacter4* Character4 = Cast<ARoundCharacter4>(Character))
        {
            static const TCHAR* PassiveKeys[] = {
                TEXT("ElementFireDamageMultiplier"), TEXT("ElementIceRangeAdd"), TEXT("PassiveIceBonusDuration"),
                TEXT("ElementLightningCooldownMultiplier"), TEXT("ElementRockDefensePerEnemyHit"),
                TEXT("ElementRockMaxDefenseStacks"), TEXT("ElementRockDefenseDurationInitiative"),
                TEXT("FirstElementUseCostReduction"), TEXT("DifferentElementInitiativeRefund"),
                TEXT("ElementProtectionIncomingDamageMultiplier"), TEXT("ElementProtectionDurationInitiative"),
                TEXT("DefenseWhileElementStored"), TEXT("ElementEcho"), TEXT("AllElementsOnCompletedCycle"),
                TEXT("ElementMasteryMultiplier"), TEXT("EmpoweredStoredElementMultiplier"),
                TEXT("SameElementEffectMultiplier"), TEXT("EmpowerStoredElementOnMultiHit"),
                TEXT("CooldownReductionWithoutElement"), TEXT("ArcaneShiftPreservesElement")};
            for (const TCHAR* Key : PassiveKeys)
                Definition.passiveValues.emplace(ToCoreString(Key), Character4->GetPassiveValueFromKey(Key));
            Definition.passiveValues.emplace("CoreMageCharacter", 1.0);
        }
        if (const ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(Character))
        {
            static const TCHAR* PassiveKeys[] = {
                TEXT("BloodGainMultiplier"), TEXT("BloodCapAdd"), TEXT("BloodGainFractionFromDamageTaken"),
                TEXT("BloodDefenseThreshold"), TEXT("DefenseWhileHighBlood"), TEXT("FirstBasicAttackBloodGain"),
                TEXT("BloodSpendThresholdForInitiative"), TEXT("InitiativeGainOnLargeBloodSpend"),
                TEXT("InitiativeGainOnBloodSpend"), TEXT("LowHealthThresholdPercent"),
                TEXT("DamageMultiplierWhileLowHealth"), TEXT("IncomingDamageMultiplierWhileLowHealth"),
                TEXT("FirstIncomingDamageMultiplierPerTurn"), TEXT("CooldownReductionOnBloodCap"),
                TEXT("BloodCheatDeathCost"), TEXT("BloodCheatDeathHeal"), TEXT("BloodCheatDeathCooldownInitiative"),
                TEXT("BloodOverflowHealFraction"), TEXT("HighBloodDamageThreshold"), TEXT("HighBloodDamageMultiplier")};
            for (const TCHAR* Key : PassiveKeys)
                Definition.passiveValues.emplace(ToCoreString(Key), Character5->GetPassiveValueFromKey(Key));
            Definition.passiveValues.emplace("CoreBloodCharacter", 1.0);
            double BloodGainFraction = Character5->GetValueFromKey(TEXT("BloodGainFraction"));
            if (BloodGainFraction <= 0.0)
            {
                BloodGainFraction = Character5->GetValueFromKey(TEXT("BloodGainPercent"));
                if (BloodGainFraction > 0.0)
                    BloodGainFraction /= 100.0;
            }
            Definition.passiveValues.emplace("BloodGainFraction", BloodGainFraction > 0.0 ? BloodGainFraction : 0.1);
            Definition.passiveValues.emplace("BloodCap", Character5->GetBloodCap());
        }
        if (const ARoundCharacterEngineer* Engineer = Cast<ARoundCharacterEngineer>(Character))
        {
            static const TCHAR* PassiveKeys[] = {
                TEXT("MaxDevices"), TEXT("MaxDevicesAdd"), TEXT("DeviceHealthMultiplierAdd"),
                TEXT("DevicePlacementRangeAdd"), TEXT("DeviceRepairOnTurnStart"),
                TEXT("InitiativeRefundOnDeviceReplace"), TEXT("DeviceNetworkDefenseRange"),
                TEXT("DefenseNearDevice"), TEXT("DeviceLifetimeMultiplierAdd"),
                TEXT("DefensePerActiveDevice"), TEXT("MaxDefenseFromDevices"),
                TEXT("DeviceTriggerIntervalReduction")};
            for (const TCHAR* Key : PassiveKeys)
                Definition.passiveValues.emplace(ToCoreString(Key), Engineer->GetPassiveValueFromKey(Key));
            Definition.passiveValues.emplace("CoreEngineerCharacter", 1.0);
        }
        if (const ARoundCharacterChronomancer* Chronomancer = Cast<ARoundCharacterChronomancer>(Character))
        {
            static const TCHAR* PassiveKeys[] = {
                TEXT("MaxFragments"), TEXT("FragmentGainAtTurnStartIfEmpty"),
                TEXT("HealPerOverflowFragment"), TEXT("InitiativeGainOnFirstFragmentSpend"),
                TEXT("TimeSiphonCooldownReductionPerFragmentSpent"),
                TEXT("InitiativeGainAtTurnStartIfFull"), TEXT("HealPerFragmentSpent"),
                TEXT("FirstFragmentSpendDiscount"), TEXT("AllCooldownReductionPerOverflowFragment")};
            for (const TCHAR* Key : PassiveKeys)
                Definition.passiveValues.emplace(ToCoreString(Key), Chronomancer->GetPassiveValueFromKey(Key));
            Definition.passiveValues.emplace("CoreChronomancerCharacter", 1.0);
            Definition.passiveValues.emplace("BaseMaxFragments", Chronomancer->GetValueFromKey(TEXT("MaxFragments")));
        }
        roundcore::CharacterState CharacterState;
        CharacterState.id = CharacterId;
        CharacterState.definitionId = DefinitionId;
        CharacterState.team = ToCoreTeam(Character->Team);
        CharacterState.tileId = TileIds.FindRef(Character->CurrentTile);
        CharacterState.health = Character->Attributes->CurrentHealth;
        CharacterState.initiative = Character->Attributes->CurrentInitiative;
        CharacterState.alive = Character->IsAlive();
        // Migrated blockers are represented by modifierEffects so the Core can
        // expire them authoritatively. Preserve the snapshot flag only for
        // blockers whose complete behavior has not moved to Core yet.
        CharacterState.movementBlocked = false;
        for (const ARoundEffectActor* Effect : Character->ActiveEffects)
        {
            if (Effect && Effect->PreventsMovement()
                && !Cast<ARoundRootEffect>(Effect)
                && !Cast<ARoundSuppressingFireEffect>(Effect))
            {
                CharacterState.movementBlocked = true;
                break;
            }
        }
        CharacterState.canTakeTurn = !Character->CharacterRowName.ToString().Contains(TEXT("Puppet"), ESearchCase::IgnoreCase)
            && !GameState.IsCharacterBlockedByObjectiveChannel(Character);
        CharacterState.initiativeGainPerTick = Character->ModifyInitiativeGainFromEffects(1);
        CharacterState.delayedActionInitiative = FMath::Max(0, Character->NativeDelayedActionInitiative);
        if (const ARoundCharacter1* Character1 = Cast<ARoundCharacter1>(Character))
        {
            CharacterState.resourceCurrent = Character1->GetCurrentChainCount();
            CharacterState.resourceMaximum = Character1->GetChainLimit();
            CharacterState.resourceRechargeProgress = FMath::Max(0, Character1->NativeChainRechargeProgress);
            CharacterState.resourceRechargeInterval = Character1->GetChainRechargeTime();
            CharacterState.specialCooldownRemaining = FMath::Max(0, Character1->NativeDamageRechargeCooldownRemaining);
            CharacterState.firstResourceSpendRefundAvailable = !Character1->bGrantedChainSpendRefundThisTurn;
            CharacterState.movedSinceLastTurn = Character1->bMovedThisTurn;
            CharacterState.stationaryGuardActive = Character1->bStationaryGuardActive;
        }
        if (const ARoundCharacter2* Character2 = Cast<ARoundCharacter2>(Character))
        {
            CharacterState.resourceCurrent = Character2->Ammo;
            CharacterState.resourceMaximum = Character2->GetMaxAmmo();
            CharacterState.movedSinceLastTurn = Character2->bMovedThisTurn;
            CharacterState.attackedThisTurn = Character2->bAttackedThisTurn;
            CharacterState.spentResourceThisTurn = Character2->bSpentAmmoThisTurn;
            CharacterState.startedTurnWithFullResource = Character2->bStartedTurnWithFullAmmo;
            CharacterState.heldPositionFromPreviousTurn = Character2->bHeldPositionFromPreviousTurn;
            CharacterState.usedFreeBasicAttackThisTurn = Character2->bUsedFreeBasicAttackThisTurn;
            CharacterState.triggeredEmptyResourceThisTurn = Character2->bTriggeredEmptyMagazineThisTurn;
            CharacterState.refundedLastResourceKillThisTurn = Character2->bRefundedLastAmmoKillThisTurn;
            CharacterState.usedRangeRefundThisTurn = Character2->bUsedSnipeRangeRefundThisTurn;
            CharacterState.usedKillCooldownReductionThisTurn = Character2->bUsedSnipeKillCooldownReductionThisTurn;
            CharacterState.stationaryShotStacks = Character2->StationaryShotStacks;
            CharacterState.hasFreePreparedStep = Character2->bHasFreeConcentrateStep;
        }
        if (const ARoundCharacterChronomancer* Chronomancer = Cast<ARoundCharacterChronomancer>(Character))
        {
            CharacterState.resourceCurrent = Chronomancer->Fragments;
            CharacterState.resourceMaximum = Chronomancer->GetMaxFragments();
            CharacterState.firstResourceDiscountAvailable = !Chronomancer->bUsedFragmentDiscountThisTurn;
            CharacterState.firstResourceSpendRefundAvailable = !Chronomancer->bUsedFirstSpendRefundThisTurn;
        }
        if (const ARoundCharacter3* Character3 = Cast<ARoundCharacter3>(Character))
        {
            CharacterState.resourceCurrent = Character3->TryGetComboPoints();
            CharacterState.resourceMaximum = Character3->GetMaxComboPoints();
            CharacterState.movedFieldsThisTurn = Character3->NativeMovedFieldsThisTurn;
            CharacterState.resourceSpentThisTurnCount = Character3->NativeComboSpentThisTurn;
            CharacterState.secondaryCooldownRemaining = Character3->HuntMarkCooldownInitiativeRemaining;
            CharacterState.turnSequence = Character3->NativeTurnSequence;
            CharacterState.openingRewardUsedThisTurn = Character3->bOpeningCutUsedThisTurn;
            CharacterState.overflowRewardUsedThisTurn = Character3->bComboOverflowInitiativeUsedThisTurn;
            CharacterState.freeAdjacentMoveAvailable = Character3->bFreeRetreatAvailable;
            CharacterState.freeAdjacentMoveUsedThisTurn = Character3->bFreeRetreatUsedThisTurn;
            CharacterState.nextDamageAfterMoveReady = Character3->bDamageAfterMoveReady;
            CharacterState.nextReductionAfterMoveReady = Character3->bDamageReductionAfterMoveReady;
            CharacterState.startInvisibilityApplied = Character3->bNativeStartInvisibilityApplied;
        }
        if (const ARoundCharacter4* Character4 = Cast<ARoundCharacter4>(Character))
        {
            CharacterState.activeElement = static_cast<int32>(Character4->GetActiveElementBonus());
            for (const ERoundMageElementBonus Element : Character4->UsedElementBonusesThisTurn)
                CharacterState.usedElementMask |= 1u << static_cast<uint32>(Element);
            CharacterState.lastCastElement = static_cast<int32>(Character4->LastCastElementBonus);
            CharacterState.firstCastElement = static_cast<int32>(Character4->FirstCastElementBonusThisTurn);
            CharacterState.elementSwapCooldownRemaining = Character4->ElementSwapCooldownTurnsRemaining;
            CharacterState.rockDefenseBonus = Character4->ElementalRockDefenseBonus;
            CharacterState.rockDefenseRemainingInitiative = Character4->ElementalRockDefenseRemainingInitiative;
            CharacterState.protectionRemainingInitiative = Character4->ProtectionRemainingInitiative;
            CharacterState.echoElement = static_cast<int32>(Character4->EchoElement);
            for (const ERoundMageElementBonus Element : Character4->ElementCycle)
                CharacterState.elementCycleMask |= 1u << static_cast<uint32>(Element);
            CharacterState.elementCostReductionUsed = Character4->bElementCostReductionUsedThisTurn;
            CharacterState.elementSwapRefundUsed = Character4->bElementSwapRefundUsedThisTurn;
            CharacterState.storedElementEmpowered = Character4->bStoredElementEmpowered;
            CharacterState.protectionReady = Character4->bProtectionReady;
        }
        if (const ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(Character))
        {
            CharacterState.resourceCurrent = Character5->NativeBlood;
            CharacterState.resourceMaximum = Character5->GetBloodCap();
            CharacterState.specialCooldownRemaining = Character5->BloodCheatDeathCooldownTurnsRemaining;
            CharacterState.firstBloodDamageReductionAvailable = Character5->bFirstDamageReductionAvailableThisTurn;
            CharacterState.basicAttackBloodAvailable = Character5->bBasicAttackBloodAvailableThisTurn;
            CharacterState.bloodCapTriggerAvailable = Character5->bBloodCapTriggerAvailable;
        }
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
            const TMap<FString, double> ResolvedValues = Ability->GetResolvedAbilityValues();
            for (const TPair<FString, double>& Value : ResolvedValues)
            {
                AbilityDefinition.values.emplace(ToCoreString(Value.Key), Value.Value);
            }
            const int32 CoreOperation = FMath::RoundToInt(Ability->GetAbilityValueFromKey(TEXT("CoreOperation")));
            AbilityDefinition.operation = CoreOperation >= 1 && CoreOperation <= 35
                ? static_cast<roundcore::AbilityOperation>(CoreOperation)
                : roundcore::AbilityOperation::Unsupported;
            AbilityDefinition.targetRelation = AbilityDefinition.operation == roundcore::AbilityOperation::SelfModifier
                ? roundcore::AbilityTargetRelation::Self
                : ((AbilityDefinition.operation == roundcore::AbilityOperation::DirectHeal
                    || AbilityDefinition.operation == roundcore::AbilityOperation::TargetModifier)
                    ? roundcore::AbilityTargetRelation::Ally
                    : roundcore::AbilityTargetRelation::Enemy);
            if (ResolvedValues.Contains(TEXT("CoreTargetRelation")))
            {
                AbilityDefinition.targetRelation = static_cast<roundcore::AbilityTargetRelation>(FMath::Clamp(
                    FMath::RoundToInt(Ability->GetAbilityValueFromKey(TEXT("CoreTargetRelation"))), 0, 3));
            }
            AbilityDefinition.initiativeCost = Ability->GetAbilityCost();
            AbilityDefinition.range = Ability->GetAbilityRange();
            AbilityDefinition.amount = (AbilityDefinition.operation == roundcore::AbilityOperation::DirectDamage
                || AbilityDefinition.operation == roundcore::AbilityOperation::GroundAura
                || (CoreOperation >= 6 && CoreOperation <= 34))
                ? Ability->GetAbilityDamage()
                : FMath::Max(0, FMath::RoundToInt(Ability->GetAbilityValueFromKey(TEXT("Heal"))));
            AbilityDefinition.defenseIgnore = FMath::Max(
                0, FMath::RoundToInt(Ability->GetAbilityValueFromKey(TEXT("DefenseIgnore"))));
            AbilityDefinition.damageType = ToCoreDamageType(Ability->GetAbilityDamageType());
            AbilityDefinition.requiresVisibility = Ability->GetAbilityValueFromKey(TEXT("IgnoreVisibility")) <= 0.0;
            Setup.definitions.abilities.emplace(AbilityId, std::move(AbilityDefinition));
            Definition.abilityIds.push_back(AbilityId);

            roundcore::AbilityState AbilityState;
            AbilityState.definitionId = AbilityId;
            AbilityState.cooldownRemaining = FMath::Max(0, Ability->CurrentCooldown);
            AbilityState.cooldownDuration = FMath::Max(0, Ability->GetAbilityCooldown());
            AbilityState.currentCharges = FMath::Max(0, Ability->CurrentCharges);
            AbilityState.maxCharges = FMath::Max(1, Ability->GetAbilityMaxCharges());
            AbilityState.usesCharges = Ability->UsesCharges();
            if (const ARoundSnipeAbility* Snipe = Cast<ARoundSnipeAbility>(Ability))
            {
                AbilityState.readyInitiativeElapsed = Snipe->ReadyInitiativeElapsed;
            }
            CharacterState.abilities.push_back(std::move(AbilityState));
            AbilityActors.Add(AbilityKey, Ability);
        }
        for (const ARoundEffectActor* Effect : Character->ActiveEffects)
        {
            const ARoundModifierEffect* Modifier = Cast<ARoundModifierEffect>(Effect);
            const ARoundBasicAttackDamageReductionEffect* BasicAttackReduction =
                Cast<ARoundBasicAttackDamageReductionEffect>(Effect);
            const ARoundSuppressingFireEffect* SuppressingFire = Cast<ARoundSuppressingFireEffect>(Effect);
            const ARoundSilenceEffect* Silence = Cast<ARoundSilenceEffect>(Effect);
            const ARoundRootEffect* Root = Cast<ARoundRootEffect>(Effect);
            const ARoundSlowEffect* Slow = Cast<ARoundSlowEffect>(Effect);
            const ARoundIcebeamEffect* Icebeam = Cast<ARoundIcebeamEffect>(Effect);
            const ARoundShieldOathEffect* ShieldOath = Cast<ARoundShieldOathEffect>(Effect);
            const ARoundSanctifiedGroundAuraEffect* SanctifiedGround = Cast<ARoundSanctifiedGroundAuraEffect>(Effect);
            const ARoundBindEffect* Bind = Cast<ARoundBindEffect>(Effect);
            const ARoundTauntEffect* Taunt = Cast<ARoundTauntEffect>(Effect);
            const ARoundInvisibilityEffect* Invisibility = Cast<ARoundInvisibilityEffect>(Effect);
            const ARoundAssassinHuntMarkEffect* HuntMark = Cast<ARoundAssassinHuntMarkEffect>(Effect);
            const ARoundShadowWoundEffect* ShadowWound = Cast<ARoundShadowWoundEffect>(Effect);
            const ARoundDeathMarkEffect* DeathMark = Cast<ARoundDeathMarkEffect>(Effect);
            const ARoundSharedBreachEffect* SharedBreach = Cast<ARoundSharedBreachEffect>(Effect);
            const ARoundBleedEffect* Bleed = Cast<ARoundBleedEffect>(Effect);
            if (!Modifier && !BasicAttackReduction && !SuppressingFire && !Silence && !Root && !Slow
                && !ShieldOath && !SanctifiedGround && !Bind && !Taunt && !Invisibility
                && !HuntMark && !ShadowWound && !DeathMark && !SharedBreach && !Bleed)
            {
                continue;
            }
            roundcore::ModifierEffectState EffectState;
            EffectState.id = ToCoreString(Effect->EffectId.IsNone()
                ? Effect->GetClass()->GetName()
                : Effect->EffectId.ToString());
            EffectState.causerId = CharacterIds.FindRef(Effect->CauserCharacter);
            EffectState.remainingInitiative = FMath::Max(0, Effect->RemainingInitiative);
            EffectState.remainingTurns = FMath::Max(0, Effect->RemainingTurns);
            EffectState.isDebuff = Effect->IsDebuff();
            if (Modifier)
            {
                EffectState.incomingDamageMultiplierBasisPoints = FMath::Max(
                    0, FMath::RoundToInt(Modifier->IncomingDamageMultiplier * 10000.0));
                EffectState.outgoingDamageMultiplierBasisPoints = FMath::Max(
                    0, FMath::RoundToInt(Modifier->OutgoingDamageMultiplier * 10000.0));
                EffectState.defenseIgnoreAdd = Modifier->DefenseIgnoreAdd;
                EffectState.healFractionFromDamageDealtBasisPoints = FMath::Max(
                    0, FMath::RoundToInt(Modifier->HealFractionFromDamageDealt * 10000.0));
                EffectState.moveCostAdd = Modifier->MoveCostAdd;
                EffectState.onlyAffectCauserDamage = Modifier->bOnlyAffectCauserDamage;
                EffectState.onlyAgainstRangedSources = Modifier->bOnlyAgainstRangedSources;
                EffectState.blockMovementBonuses = Modifier->bBlockMovementBonuses;
                EffectState.incomingEffectDurationAdd = FMath::Max(0, Modifier->IncomingEffectDurationAdd);
            }
            if (BasicAttackReduction)
            {
                EffectState.kind = roundcore::ModifierEffectKind::BasicAttackWeakness;
                EffectState.basicAttackDamageMultiplierBasisPoints = FMath::Max(
                    0, FMath::RoundToInt(BasicAttackReduction->DamageMultiplier * 10000.0));
                EffectState.consumeOnBasicAttack = true;
            }
            if (Invisibility) EffectState.kind = roundcore::ModifierEffectKind::Invisibility;
            if (HuntMark)
            {
                EffectState.kind = roundcore::ModifierEffectKind::AssassinHuntMark;
                EffectState.incomingDamageMultiplierFromCauserBasisPoints = FMath::Max(
                    0, FMath::RoundToInt(HuntMark->DamageMultiplierFromCauser * 10000.0));
            }
            if (ShadowWound)
            {
                EffectState.kind = roundcore::ModifierEffectKind::ShadowWound;
                EffectState.valueA = FMath::Max(0, ShadowWound->BonusDamage);
                EffectState.valueB = FMath::Max(0, ShadowWound->ComboGain);
            }
            if (DeathMark)
            {
                EffectState.kind = roundcore::ModifierEffectKind::DeathMark;
                EffectState.incomingDamageMultiplierFromCauserBasisPoints = FMath::Max(
                    0, FMath::RoundToInt(DeathMark->DamageMultiplierFromCauser * 10000.0));
                EffectState.defenseIgnoreAdd = FMath::Max(0, DeathMark->DefenseIgnoreFromCauser);
                EffectState.counterA = FMath::Max(0, DeathMark->ComboGainOnTrigger);
                EffectState.valueA = FMath::Max(0, DeathMark->MoveDurationExtension);
                EffectState.valueB = FMath::Max(0, DeathMark->ExecuteCooldownReductionOnMaxCombo);
                EffectState.valueC = FMath::Max(0, DeathMark->DeathMarkCooldownReductionOnExpiry);
                EffectState.counterC = FMath::Max(0, DeathMark->TransferRangeOnKill);
                EffectState.triggerSlowMultiplierBasisPoints = FMath::Max(
                    0, FMath::RoundToInt(DeathMark->LowHealthThreshold * 10000.0));
                EffectState.triggerOutgoingDamageMultiplierBasisPoints = FMath::Max(
                    0, FMath::RoundToInt(DeathMark->LowHealthDamageMultiplierAdd * 10000.0));
                if (DeathMark->bGrantComboEachCauserTurn) EffectState.flags |= 2u;
                if (DeathMark->bPreventInvisibility) EffectState.flags |= 4u;
                if (DeathMark->bTransferred) EffectState.flags |= 16u;
            }
            if (SharedBreach)
            {
                EffectState.kind = roundcore::ModifierEffectKind::SharedBreach;
                EffectState.valueA = FMath::Max(0, SharedBreach->DefenseIgnorePerCharacter);
            }
            if (Bleed)
            {
                EffectState.kind = roundcore::ModifierEffectKind::Bleed;
                EffectState.valueA = FMath::Max(0, FMath::RoundToInt(Bleed->Amount));
            }
            if (SuppressingFire)
            {
                EffectState.moveCostAdd += SuppressingFire->FlatMoveCostAdd;
                EffectState.defenseIgnoreAdd += FMath::Max(0, SuppressingFire->DefenseReduction);
                EffectState.basicAttackCostAdd = FMath::Max(0, SuppressingFire->BasicAttackCostAdd);
                EffectState.basicAttackCostAvailable = SuppressingFire->IsBasicAttackTaxAvailable();
                EffectState.preventsMovement = SuppressingFire->PreventsMovement();
            }
            if (Root)
            {
                EffectState.incomingDamageMultiplierFromCauserBasisPoints = FMath::Max(
                    0, FMath::RoundToInt(Root->IncomingDamageMultiplierFromCauser * 10000.0));
                EffectState.initiativeLossOnExpire = FMath::Max(0, Root->InitiativeLossOnExpire);
                EffectState.preventsMovement = Root->PreventsMovement();
            }
            if (Slow)
            {
                EffectState.moveCostMultiplierBasisPoints = FMath::Max(
                    0, FMath::RoundToInt((Slow->Amount > 0.0 ? 1.0 + Slow->Amount : 1.0) * 10000.0));
                EffectState.initiativeLossOnMove = FMath::Max(0, Slow->InitiativeLossOnMove);
                EffectState.initiativeLossOnExpire = FMath::Max(0, Slow->InitiativeLossOnExpire);
                EffectState.blockMovementBonuses = Slow->bBlockMovementBonuses;
                EffectState.incomingDamageMultiplierFromCauserBasisPoints = FMath::Max(0,
                    FMath::RoundToInt(Slow->IncomingDamageMultiplierFromCauser * 10000.0));
            }
            if (Icebeam)
            {
                EffectState.firstMoveInitiativeLoss = FMath::Max(0, Icebeam->FirstMoveInitiativeLoss);
                EffectState.maximumMovedFields = FMath::Max(0, Icebeam->MaximumMovedFields);
                EffectState.movedFields = FMath::Max(0, Icebeam->MovedFields);
                EffectState.frostbreakDamage = FMath::Max(0, Icebeam->FrostbreakDamage);
                EffectState.firstMovePenaltyAvailable = Icebeam->bFirstMovePenaltyAvailable;
            }
            if (ShieldOath)
            {
                EffectState.redirectPreMitigationBasisPoints = FMath::Clamp(
                    FMath::RoundToInt(ShieldOath->RedirectPreMitigationFraction * 10000.0), 0, 10000);
                EffectState.redirectedDamageMultiplierBasisPoints = FMath::Max(
                    0, FMath::RoundToInt(ShieldOath->RedirectedDamageMultiplier * 10000.0));
                EffectState.protectorHealFractionBasisPoints = FMath::Max(
                    0, FMath::RoundToInt(ShieldOath->PaladinHealFraction * 10000.0));
                EffectState.protectedInitiativePerHit = FMath::Max(0, ShieldOath->ProtectedAllyInitiativePerHit);
            }
            if (SanctifiedGround)
            {
                EffectState.areaCenterTileId = SanctifiedGround->bAuraUsesFixedPosition
                    ? TileIds.FindRef(GameState.GetFieldAt(SanctifiedGround->AuraFixedPosition))
                    : CharacterState.tileId;
                EffectState.areaRadius = FMath::Max(0, SanctifiedGround->Radius);
                EffectState.areaDamage = FMath::Max(0, SanctifiedGround->AuraTriggerDamage);
                EffectState.areaDamageType = ToCoreDamageType(SanctifiedGround->AuraTriggerDamageType);
                EffectState.areaInitiativeLossPerPulse = FMath::Max(0, SanctifiedGround->InitiativeLossPerPulse);
                EffectState.areaFollowsCauser = !SanctifiedGround->bAuraUsesFixedPosition;
            }
            if (Bind)
            {
                EffectState.triggerOnMove = true;
                EffectState.moveCostAdd += 2;
                EffectState.blockMovementBonuses = Bind->bBlockMovementBonusesOnTarget;
                EffectState.triggerDamageOnMove = FMath::Max(0, Bind->TriggerDamage);
                EffectState.triggerDelayOnMove = FMath::Max(0, Bind->DelayAmount);
                EffectState.triggerOutgoingDamageMultiplierBasisPoints = FMath::Max(0,
                    FMath::RoundToInt(Bind->TriggerOutgoingDamageMultiplier * 10000.0));
                EffectState.triggerSlowMultiplierBasisPoints = FMath::Max(10000,
                    FMath::RoundToInt((1.0 + Bind->TriggerSlowAmount) * 10000.0));
                EffectState.triggerSlowDurationInitiative = FMath::Max(0, Bind->TriggerSlowDuration * 40);
                EffectState.triggerAreaRadius = FMath::Max(0, Bind->TriggerAoEInitiativeLossRadius);
                EffectState.triggerAreaInitiativeLoss = FMath::Max(0, Bind->TriggerAoEInitiativeLossAmount);
                EffectState.triggerInitiativeGainForCauser = FMath::Max(0, Bind->TriggerInitiativeGainForCauser);
            }
            if (Taunt)
            {
                EffectState.preferredTargetId = EffectState.causerId;
                EffectState.outgoingDamageMultiplierAgainstOtherTargetsBasisPoints = FMath::Max(0,
                    FMath::RoundToInt((Taunt->Amount > 0.0 ? Taunt->Amount : 1.0) * 10000.0));
                EffectState.initiativeLossWhenIgnoringPreferredTarget = FMath::Max(0, Taunt->InitiativeLossWhenIgnoringCauser);
                EffectState.initiativeGainForCauserWhenAttacked = FMath::Max(0, Taunt->InitiativeGainForCauserWhenAttacked);
                EffectState.incomingDamageMultiplierBasisPoints = FMath::Max(0,
                    FMath::RoundToInt(Taunt->IncomingDamageMultiplier * 10000.0));
                EffectState.incomingDamageMultiplierFromCauserBasisPoints = FMath::Max(0,
                    FMath::RoundToInt(Taunt->IncomingDamageMultiplierFromCauser * 10000.0));
            }
            EffectState.preventsAbilityUse = Silence != nullptr;
            EffectState.expireOnTurnEnd = Effect->bAutoExpireOnTurnEnd;
            EffectState.sourceAbilityId = ToCoreString(Effect->StringState.FindRef(TEXT("CoreSourceAbilityId")));
            if (Effect->IntegerState.Contains(TEXT("CoreEffectKind")))
            {
                EffectState.kind = static_cast<roundcore::ModifierEffectKind>(Effect->IntegerState.FindRef(TEXT("CoreEffectKind")));
                EffectState.counterA = Effect->IntegerState.FindRef(TEXT("CoreCounterA"));
                EffectState.counterB = Effect->IntegerState.FindRef(TEXT("CoreCounterB"));
                EffectState.counterC = Effect->IntegerState.FindRef(TEXT("CoreCounterC"));
                EffectState.valueA = Effect->IntegerState.FindRef(TEXT("CoreValueA"));
                EffectState.valueB = Effect->IntegerState.FindRef(TEXT("CoreValueB"));
                EffectState.valueC = Effect->IntegerState.FindRef(TEXT("CoreValueC"));
                EffectState.flags = static_cast<uint32>(Effect->IntegerState.FindRef(TEXT("CoreFlags")));
            }
            CharacterState.modifierEffects.push_back(std::move(EffectState));
        }
        if (const TArray<roundcore::ModifierEffectState>* PreservedEffects = PreservedCoreEffects.Find(Character))
        {
            for (roundcore::ModifierEffectState& SnapshotEffect : CharacterState.modifierEffects)
            {
                const roundcore::ModifierEffectState* Preserved = PreservedEffects->FindByPredicate(
                    [&SnapshotEffect](const roundcore::ModifierEffectState& Existing) { return Existing.id == SnapshotEffect.id; });
                if (Preserved && Preserved->kind != roundcore::ModifierEffectKind::Generic)
                {
                    SnapshotEffect = *Preserved;
                }
            }
        }
        if (const TMap<const ARoundCharacter*, uint64>* Preventions = PreservedAuraPreventions.Find(Character))
        {
            for (const TPair<const ARoundCharacter*, uint64>& Prevention : *Preventions)
            {
                const roundcore::EntityId SourceId = CharacterIds.FindRef(Prevention.Key);
                if (SourceId != roundcore::InvalidEntityId)
                {
                    CharacterState.recentAuraPreventions.push_back({SourceId, Prevention.Value});
                }
            }
        }
        Setup.definitions.characters.emplace(DefinitionId, std::move(Definition));
        Setup.initialState.characters.push_back(std::move(CharacterState));
    }

    roundcore::EntityId NextDeviceId = 1;
    for (const roundcore::TileState& Tile : Setup.initialState.tiles) NextDeviceId = FMath::Max(NextDeviceId, Tile.id + 1);
    for (const roundcore::CharacterState& Character : Setup.initialState.characters) NextDeviceId = FMath::Max(NextDeviceId, Character.id + 1);
    for (ARoundCharacter* Character : SortedCharacters)
    {
        ARoundCharacterEngineer* Engineer = Cast<ARoundCharacterEngineer>(Character);
        if (!Engineer) continue;
        Engineer->SetCoreDevicesAuthoritative(true);
        for (ARoundEngineerDeviceActor* Device : Engineer->GetActiveDevices())
        {
            if (!Device || !Device->CurrentTile || !TileIds.Contains(Device->CurrentTile)) continue;
            ARoundAbilityActor* SourceAbility = nullptr;
            roundcore::DeviceType DeviceType = roundcore::DeviceType::Turret;
            for (ARoundAbilityActor* Candidate : Engineer->GrantedAbilities)
            {
                if ((Cast<ARoundTurretDeviceActor>(Device) && Candidate->IsA<ARoundBuildTurretAbility>())
                    || (Cast<ARoundMineDeviceActor>(Device) && Candidate->IsA<ARoundBuildMineAbility>())
                    || (Cast<ARoundShieldEmitterDeviceActor>(Device) && Candidate->IsA<ARoundBuildShieldEmitterAbility>())
                    || (Cast<ARoundSpotterBeaconDeviceActor>(Device) && Candidate->IsA<ARoundBuildSpotterBeaconAbility>()))
                { SourceAbility = Candidate; break; }
            }
            FString SourceAbilityKey;
            for (const TPair<FString, ARoundAbilityActor*>& Entry : AbilityActors)
                if (Entry.Value == SourceAbility) { SourceAbilityKey = Entry.Key; break; }
            if (SourceAbilityKey.IsEmpty()) continue;
            if (Cast<ARoundMineDeviceActor>(Device)) DeviceType = roundcore::DeviceType::Mine;
            else if (Cast<ARoundShieldEmitterDeviceActor>(Device)) DeviceType = roundcore::DeviceType::ShieldEmitter;
            else if (Cast<ARoundSpotterBeaconDeviceActor>(Device)) DeviceType = roundcore::DeviceType::SpotterBeacon;

            roundcore::DeviceState State;
            State.id = NextDeviceId++; State.ownerId = CharacterIds.FindRef(Engineer);
            State.tileId = TileIds.FindRef(Device->CurrentTile); State.sourceAbilityId = ToCoreString(SourceAbilityKey);
            State.type = DeviceType; State.health = Device->CurrentHealth; State.maximumHealth = Device->MaxHealth;
            State.remainingInitiative = FMath::Max(1, Device->RemainingInitiative);
            State.triggerIntervalInitiative = FMath::Max(0, Device->TriggerIntervalInitiative);
            State.triggerProgressInitiative = FMath::Max(0, Device->TriggerProgressInitiative);
            if (const ARoundTurretDeviceActor* Turret = Cast<ARoundTurretDeviceActor>(Device))
            {
                State.range = Turret->TurretRange; State.amount = Turret->TurretDamage;
                State.damageType = ToCoreDamageType(Turret->TurretDamageType); State.defenseIgnore = Turret->DefenseIgnore;
                State.initiativeLoss = Turret->InitiativeLossOnHit;
                State.secondaryMultiplierBasisPoints = FMath::RoundToInt(Turret->SecondaryTargetDamageMultiplier * 10000.0);
                State.secondaryAmount = FMath::RoundToInt(Turret->LowHealthThreshold * 10000.0);
                State.multiplierBasisPoints = FMath::RoundToInt(Turret->LowHealthDamageMultiplier * 10000.0);
                State.flags |= 4u;
            }
            else if (const ARoundMineDeviceActor* Mine = Cast<ARoundMineDeviceActor>(Device))
            {
                State.range = Mine->ExplosionRadius; State.amount = Mine->ExplosionDamage;
                State.damageType = ToCoreDamageType(Mine->ExplosionDamageType); State.defenseIgnore = Mine->DefenseIgnore;
                State.initiativeLoss = Mine->InitiativeLossOnHit; State.effectDurationInitiative = Mine->RootDurationInitiative;
                State.armDelayInitiative = Mine->GetRemainingArmDelayInitiative();
                State.enemyRevealInitiative = Mine->GetRemainingEnemyRevealInitiative();
                if (Mine->bPreserveOnManualTrigger) State.flags |= 1u;
            }
            else if (const ARoundShieldEmitterDeviceActor* Emitter = Cast<ARoundShieldEmitterDeviceActor>(Device))
            {
                State.range = Emitter->ShieldRadius; State.amount = Emitter->ShieldAmount;
                State.effectDurationInitiative = Emitter->ShieldDurationInitiative;
                State.secondaryAmount = Emitter->HealAmount; State.initiativeLoss = Emitter->InitiativeGain;
                State.multiplierBasisPoints = FMath::RoundToInt(Emitter->SoloShieldMultiplier * 10000.0);
                if (Emitter->bCleanseOneDebuff) State.flags |= 2u;
            }
            else if (const ARoundSpotterBeaconDeviceActor* Beacon = Cast<ARoundSpotterBeaconDeviceActor>(Device))
            {
                State.range = Beacon->BeaconRadius; State.effectDurationInitiative = Beacon->DebuffDurationInitiative;
                State.multiplierBasisPoints = FMath::RoundToInt(Beacon->DamageTakenMultiplier * 10000.0);
                State.secondaryMultiplierBasisPoints = FMath::RoundToInt(Beacon->OutgoingDamageMultiplier * 10000.0);
                State.initiativeLoss = Beacon->InitiativeLoss; State.moveCostAdd = Beacon->MoveCostAdd;
            }
            Setup.initialState.devices.push_back(State);
            DeviceIds.Add(Device, State.id); DeviceActors.Add(State.id, Device);
            for (roundcore::TileState& Tile : Setup.initialState.tiles) if (Tile.id == State.tileId)
            { Tile.fieldObjectId = State.id; Tile.fieldObjectHealth = State.health; break; }
            Device->ApplyCoreDeviceState(State.remainingInitiative, State.maximumHealth, State.health,
                State.triggerIntervalInitiative, State.triggerProgressInitiative);
        }
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

void FRoundCoreMatchAdapter::SynchronizeDevices()
{
    TSet<roundcore::EntityId> CoreDeviceIds;
    for (const roundcore::DeviceState& State : CoreGame.State().devices) CoreDeviceIds.Add(State.id);
    TArray<roundcore::EntityId> RemovedIds;
    for (const TPair<roundcore::EntityId, ARoundEngineerDeviceActor*>& Entry : DeviceActors)
    {
        if (CoreDeviceIds.Contains(Entry.Key)) continue;
        if (IsValid(Entry.Value)) Entry.Value->RemoveFromField();
        DeviceIds.Remove(Entry.Value); RemovedIds.Add(Entry.Key);
    }
    for (const roundcore::EntityId Id : RemovedIds) DeviceActors.Remove(Id);

    for (const roundcore::DeviceState& State : CoreGame.State().devices)
    {
        ARoundEngineerDeviceActor* Device = DeviceActors.FindRef(State.id);
        ARoundCharacterEngineer* Engineer = Cast<ARoundCharacterEngineer>(CharacterActors.FindRef(State.ownerId));
        ARoundGridTile* Tile = TileActors.FindRef(State.tileId);
        if (!Engineer || !Tile) continue;
        if (!IsValid(Device))
        {
            TSubclassOf<ARoundEngineerDeviceActor> DeviceClass;
            switch (State.type)
            {
            case roundcore::DeviceType::Mine: DeviceClass = Engineer->GetMineDeviceClass(); break;
            case roundcore::DeviceType::ShieldEmitter: DeviceClass = Engineer->GetShieldEmitterDeviceClass(); break;
            case roundcore::DeviceType::SpotterBeacon: DeviceClass = Engineer->GetSpotterBeaconDeviceClass(); break;
            default: DeviceClass = Engineer->GetTurretDeviceClass(); break;
            }
            Device = Engineer->SpawnDeviceAtTile(DeviceClass, Tile);
            if (!Device) continue;
            DeviceActors.Add(State.id, Device); DeviceIds.Add(Device, State.id);
        }
        Device->ApplyCoreDeviceState(State.remainingInitiative, State.maximumHealth, State.health,
            State.triggerIntervalInitiative, State.triggerProgressInitiative);
        if (ARoundTurretDeviceActor* Turret = Cast<ARoundTurretDeviceActor>(Device))
        {
            Turret->TurretRange = State.range; Turret->TurretDamage = State.amount;
            Turret->TurretDamageType = ToRoundDamageType(State.damageType); Turret->DefenseIgnore = State.defenseIgnore;
            Turret->InitiativeLossOnHit = State.initiativeLoss;
            Turret->SecondaryTargetDamageMultiplier = State.secondaryMultiplierBasisPoints / 10000.0;
            Turret->LowHealthThreshold = State.secondaryAmount / 10000.0;
            Turret->LowHealthDamageMultiplier = State.multiplierBasisPoints / 10000.0;
        }
        else if (ARoundMineDeviceActor* Mine = Cast<ARoundMineDeviceActor>(Device))
        {
            Mine->ExplosionRadius = State.range; Mine->ExplosionDamage = State.amount;
            Mine->ExplosionDamageType = ToRoundDamageType(State.damageType); Mine->DefenseIgnore = State.defenseIgnore;
            Mine->InitiativeLossOnHit = State.initiativeLoss; Mine->RootDurationInitiative = State.effectDurationInitiative;
            Mine->bPreserveOnManualTrigger = (State.flags & 1u) != 0u;
            Mine->ApplyCoreMineState(State.armDelayInitiative, State.enemyRevealInitiative);
        }
        else if (ARoundShieldEmitterDeviceActor* Emitter = Cast<ARoundShieldEmitterDeviceActor>(Device))
        {
            Emitter->ShieldRadius = State.range; Emitter->ShieldAmount = State.amount;
            Emitter->ShieldDurationInitiative = State.effectDurationInitiative;
            Emitter->HealAmount = State.secondaryAmount; Emitter->InitiativeGain = State.initiativeLoss;
            Emitter->SoloShieldMultiplier = State.multiplierBasisPoints / 10000.0;
            Emitter->bCleanseOneDebuff = (State.flags & 2u) != 0u;
        }
        else if (ARoundSpotterBeaconDeviceActor* Beacon = Cast<ARoundSpotterBeaconDeviceActor>(Device))
        {
            Beacon->BeaconRadius = State.range; Beacon->DebuffDurationInitiative = State.effectDurationInitiative;
            Beacon->DamageTakenMultiplier = State.multiplierBasisPoints / 10000.0;
            Beacon->OutgoingDamageMultiplier = State.secondaryMultiplierBasisPoints / 10000.0;
            Beacon->InitiativeLoss = State.initiativeLoss; Beacon->MoveCostAdd = State.moveCostAdd;
        }
    }
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

bool FRoundCoreMatchAdapter::IsAbilityLegal(
    const ARoundCharacter* Character,
    const ARoundAbilityActor* Ability,
    const ARoundCharacter* TargetCharacter,
    const ARoundGridTile* TargetTile,
    FString& OutReason) const
{
    const roundcore::EntityId* CharacterId = CharacterIds.Find(Character);
    const roundcore::EntityId* TargetId = CharacterIds.Find(TargetCharacter);
    const roundcore::EntityId* TargetTileId = TileIds.Find(TargetTile);
    FString AbilityKey;
    for (const TPair<FString, ARoundAbilityActor*>& Entry : AbilityActors)
    {
        if (Entry.Value == Ability)
        {
            AbilityKey = Entry.Key;
            break;
        }
    }
    const int32 CoreOperation = FMath::RoundToInt(Ability ? Ability->GetAbilityValueFromKey(TEXT("CoreOperation")) : 0.0);
    const bool bTileTargeted = CoreOperation == 5 || CoreOperation == 13 || CoreOperation == 19
        || CoreOperation == 21 || CoreOperation == 23 || CoreOperation == 24 || CoreOperation == 25
        || (CoreOperation >= 31 && CoreOperation <= 34);
    if (!CharacterId || AbilityKey.IsEmpty() || (bTileTargeted ? !TargetTileId : !TargetId))
    {
        OutReason = TEXT("Ability actor or target is not part of the Core snapshot.");
        return false;
    }
    roundcore::GameAction Action;
    Action.type = roundcore::ActionType::UseAbility;
    Action.actorId = *CharacterId;
    Action.targetEntityId = TargetId ? *TargetId : roundcore::InvalidEntityId;
    Action.targetTileId = TargetTileId ? *TargetTileId : roundcore::InvalidEntityId;
    Action.abilityId = ToCoreString(AbilityKey);
    const roundcore::ActionResult Result = CoreGame.ValidateAction(Action);
    OutReason = Result.success
        ? TEXT("Ability is legal in RoundCore.")
        : FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(Result.errorCode.c_str()), UTF8_TO_TCHAR(Result.message.c_str()));
    return Result.success;
}

bool FRoundCoreMatchAdapter::PreviewBasicAttack(
    const ARoundCharacter* Character,
    const ARoundCharacter* TargetCharacter,
    FRoundCoreBasicAttackPreview& OutPreview,
    FString& OutReason) const
{
    OutPreview = {};
    const roundcore::EntityId* CharacterId = CharacterIds.Find(Character);
    const roundcore::EntityId* TargetId = CharacterIds.Find(TargetCharacter);
    if (!CharacterId || !TargetId)
    {
        OutReason = TEXT("Attacker or target is not part of the Core snapshot.");
        return false;
    }

    roundcore::Game PreviewGame = CoreGame.Clone();
    roundcore::GameAction Action;
    Action.type = roundcore::ActionType::BasicAttack;
    Action.actorId = *CharacterId;
    Action.targetEntityId = *TargetId;
    const roundcore::ActionResult Result = PreviewGame.ApplyAction(Action);
    if (!Result.success)
    {
        OutReason = FString::Printf(
            TEXT("%s: %s"), UTF8_TO_TCHAR(Result.errorCode.c_str()), UTF8_TO_TCHAR(Result.message.c_str()));
        return false;
    }

    OutPreview.bCritical = Result.message.find("Critical") != std::string::npos;
    for (const roundcore::GameEvent& Event : Result.events)
    {
        if (Event.type == roundcore::EventType::InitiativeChanged && Event.sourceId == *CharacterId)
        {
            OutPreview.InitiativeCost += FMath::Max(0, -static_cast<int32>(Event.amount));
        }
        else if (Event.type == roundcore::EventType::DamageApplied && Event.targetId == *TargetId)
        {
            OutPreview.Damage += FMath::Max(0, static_cast<int32>(Event.amount));
        }
        else if (Event.type == roundcore::EventType::CharacterDied && Event.targetId == *TargetId)
        {
            OutPreview.bTargetDied = true;
        }
        else if (Event.type == roundcore::EventType::MatchEnded)
        {
            OutPreview.bMatchEnded = true;
        }
    }
    OutReason = TEXT("Basic attack preview resolved by RoundCore clone.");
    return true;
}

bool FRoundCoreMatchAdapter::ResolveBasicAttackCritical(
    const ARoundCharacter* Character,
    bool& bOutCritical,
    FString& OutReason)
{
    bOutCritical = false;
    const roundcore::EntityId* CharacterId = CharacterIds.Find(Character);
    if (!CharacterId)
    {
        OutReason = TEXT("Attacker is not part of the Core snapshot.");
        return false;
    }
    const auto CharacterState = std::find_if(
        CoreGame.State().characters.begin(),
        CoreGame.State().characters.end(),
        [CharacterId](const roundcore::CharacterState& State) { return State.id == *CharacterId; });
    if (CharacterState == CoreGame.State().characters.end())
    {
        OutReason = TEXT("Attacker state is missing from the Core snapshot.");
        return false;
    }
    const auto Definition = CoreGame.Definitions().characters.find(CharacterState->definitionId);
    if (Definition == CoreGame.Definitions().characters.end())
    {
        OutReason = TEXT("Attacker definition is missing from the Core snapshot.");
        return false;
    }
    bOutCritical = CoreGame.RollBasisPoints(Definition->second.criticalChanceBasisPoints);
    PreservedRandomState = CoreGame.State().randomState;
    OutReason = TEXT("Critical roll resolved by RoundCore.");
    return true;
}

bool FRoundCoreMatchAdapter::ExecuteBasicAttack(
    const ARoundCharacter* Character,
    const ARoundCharacter* TargetCharacter,
    FRoundCoreBasicAttackExecution& OutExecution,
    FString& OutReason)
{
    OutExecution = {};
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
    const roundcore::ActionResult Result = CoreGame.ApplyAction(Action);
    if (Result.success) SynchronizeDevices();
    if (!Result.success)
    {
        OutReason = FString::Printf(TEXT("%s: %s"),
            UTF8_TO_TCHAR(Result.errorCode.c_str()), UTF8_TO_TCHAR(Result.message.c_str()));
        return false;
    }

    OutExecution.bCritical = Result.message.find("Critical") != std::string::npos;
    for (const roundcore::GameEvent& Event : Result.events)
    {
        if (Event.type == roundcore::EventType::DamageApplied && Event.targetId == *TargetId)
        {
            OutExecution.AppliedDamage += FMath::Max(0, static_cast<int32>(Event.amount));
            OutExecution.DamageEvents.Add({CharacterActors.FindRef(Event.sourceId),
                CharacterActors.FindRef(Event.targetId), FMath::Max(0, static_cast<int32>(Event.amount)), false});
        }
        else if (Event.type == roundcore::EventType::DamageApplied)
        {
            OutExecution.DamageEvents.Add({CharacterActors.FindRef(Event.sourceId),
                CharacterActors.FindRef(Event.targetId), FMath::Max(0, static_cast<int32>(Event.amount)), false});
        }
        else if (Event.type == roundcore::EventType::CharacterDied && Event.targetId == *TargetId)
        {
            OutExecution.bTargetDied = true;
            for (FRoundCoreDamageEvent& DamageEvent : OutExecution.DamageEvents)
            {
                if (DamageEvent.Target == CharacterActors.FindRef(Event.targetId)) DamageEvent.bTargetDied = true;
            }
        }
        else if (Event.type == roundcore::EventType::CharacterDied)
        {
            for (FRoundCoreDamageEvent& DamageEvent : OutExecution.DamageEvents)
            {
                if (DamageEvent.Target == CharacterActors.FindRef(Event.targetId)) DamageEvent.bTargetDied = true;
            }
        }
        else if (Event.type == roundcore::EventType::EffectRemoved)
        {
            const roundcore::EntityId OwnerId = Event.targetId != roundcore::InvalidEntityId
                ? Event.targetId : Event.sourceId;
            if (ARoundCharacter* const* Owner = CharacterActors.Find(OwnerId))
            {
                OutExecution.RemovedEffects.Add({*Owner, UTF8_TO_TCHAR(Event.definitionId.c_str())});
            }
        }
        else if (Event.type == roundcore::EventType::EffectAdded)
        {
            const auto owner = std::find_if(CoreGame.State().characters.begin(), CoreGame.State().characters.end(),
                [&Event](const roundcore::CharacterState& state) { return state.id == Event.targetId; });
            if (owner != CoreGame.State().characters.end())
            {
                const auto effect = std::find_if(owner->modifierEffects.begin(), owner->modifierEffects.end(),
                    [&Event](const roundcore::ModifierEffectState& state) { return state.id == Event.definitionId; });
                if (effect != owner->modifierEffects.end())
                {
                    FRoundCoreModifierEffectAddition& addition = OutExecution.AddedModifierEffects.AddDefaulted_GetRef();
                    addition.Character = CharacterActors.FindRef(Event.targetId); addition.Causer = CharacterActors.FindRef(effect->causerId);
                    addition.PreferredTarget = CharacterActors.FindRef(effect->preferredTargetId);
                    addition.EffectId = UTF8_TO_TCHAR(effect->id.c_str()); addition.RemainingInitiative = effect->remainingInitiative;
                    addition.IncomingDamageMultiplierBasisPoints = effect->incomingDamageMultiplierBasisPoints;
                    addition.OutgoingDamageMultiplierBasisPoints = effect->outgoingDamageMultiplierBasisPoints;
                    addition.DefenseIgnoreAdd = effect->defenseIgnoreAdd; addition.MoveCostAdd = effect->moveCostAdd;
                    addition.bIsDebuff = effect->isDebuff; CopyCoreEffectMetadata(*effect, addition);
                }
            }
        }
    }
    for (const roundcore::CharacterState& State : CoreGame.State().characters)
    {
        if (ARoundCharacter* const* Actor = CharacterActors.Find(State.id))
        {
            FRoundCoreCharacterRuntimeUpdate& Update = OutExecution.CharacterUpdates.AddDefaulted_GetRef();
            Update.Character = *Actor; Update.Health = State.health; Update.Initiative = State.initiative;
            Update.bAlive = State.alive; Update.ResourceCurrent = State.resourceCurrent;
            Update.ResourceRechargeProgress = State.resourceRechargeProgress;
            Update.SpecialCooldownRemaining = State.specialCooldownRemaining;
            Update.bFirstResourceSpendRefundAvailable = State.firstResourceSpendRefundAvailable;
            Update.bMovedSinceLastTurn = State.movedSinceLastTurn;
            Update.bStationaryGuardActive = State.stationaryGuardActive;
            CopyExtendedCharacterState(State, Update);
        }
        for (const roundcore::AbilityState& AbilityState : State.abilities)
        {
            const FString AbilityKey = UTF8_TO_TCHAR(AbilityState.definitionId.c_str());
            if (ARoundAbilityActor* const* Ability = AbilityActors.Find(AbilityKey))
            {
                OutExecution.AbilityUpdates.Add({*Ability, AbilityState.cooldownRemaining, AbilityState.currentCharges, AbilityState.readyInitiativeElapsed});
            }
        }
    }
    PreservedRandomState = CoreGame.State().randomState;
    OutReason = TEXT("Basic attack executed by RoundCore.");
    return true;
}

bool FRoundCoreMatchAdapter::ExecuteAbility(
    const ARoundCharacter* Character,
    const ARoundAbilityActor* Ability,
    const ARoundCharacter* TargetCharacter,
    const ARoundGridTile* TargetTile,
    FRoundCoreAbilityExecution& OutExecution,
    FString& OutReason)
{
    OutExecution = {};
    const roundcore::EntityId* CharacterId = CharacterIds.Find(Character);
    const roundcore::EntityId* TargetId = CharacterIds.Find(TargetCharacter);
    const roundcore::EntityId* TargetTileId = TileIds.Find(TargetTile);
    FString AbilityKey;
    for (const TPair<FString, ARoundAbilityActor*>& Entry : AbilityActors)
    {
        if (Entry.Value == Ability)
        {
            AbilityKey = Entry.Key;
            break;
        }
    }
    const int32 CoreOperation = FMath::RoundToInt(Ability ? Ability->GetAbilityValueFromKey(TEXT("CoreOperation")) : 0.0);
    const bool bTileTargeted = CoreOperation == 5 || CoreOperation == 13 || CoreOperation == 19
        || CoreOperation == 21 || CoreOperation == 23 || CoreOperation == 24 || CoreOperation == 25
        || (CoreOperation >= 31 && CoreOperation <= 34);
    if (!CharacterId || AbilityKey.IsEmpty() || (bTileTargeted ? !TargetTileId : !TargetId))
    {
        OutReason = TEXT("Ability actor or target is not part of the Core snapshot.");
        return false;
    }

    roundcore::GameAction Action;
    Action.type = roundcore::ActionType::UseAbility;
    Action.actorId = *CharacterId;
    Action.targetEntityId = TargetId ? *TargetId : roundcore::InvalidEntityId;
    Action.targetTileId = TargetTileId ? *TargetTileId : roundcore::InvalidEntityId;
    Action.abilityId = ToCoreString(AbilityKey);
    const roundcore::ActionResult Result = CoreGame.ApplyAction(Action);
    if (Result.success) SynchronizeDevices();
    if (!Result.success)
    {
        OutReason = FString::Printf(TEXT("%s: %s"),
            UTF8_TO_TCHAR(Result.errorCode.c_str()), UTF8_TO_TCHAR(Result.message.c_str()));
        return false;
    }
    for (const roundcore::TileState& TileState : CoreGame.State().tiles)
    {
        ARoundGridTile* Tile = TileActors.FindRef(TileState.id);
        if (!Tile) continue;
        int32 TerrainType = static_cast<int32>(Tile->TerrainType);
        const std::string Prefix = "Terrain:";
        if (TileState.terrainId.rfind(Prefix, 0) == 0)
        {
            const std::size_t End = TileState.terrainId.find(':', Prefix.size());
            try { TerrainType = std::stoi(TileState.terrainId.substr(Prefix.size(), End - Prefix.size())); }
            catch (...) { TerrainType = static_cast<int32>(Tile->TerrainType); }
        }
        if (TerrainType != static_cast<int32>(Tile->TerrainType)) OutExecution.TerrainUpdates.Add({Tile, TerrainType});
    }

    for (const roundcore::GameEvent& Event : Result.events)
    {
        if (Event.type == roundcore::EventType::DamageApplied
            && Event.targetId == roundcore::InvalidEntityId
            && (Event.definitionId.find(":FieldObject") != std::string::npos
                || Event.definitionId.find(":TerrainObject") != std::string::npos))
        {
            FRoundCoreBoardDamageEvent& BoardDamage = OutExecution.BoardDamageEvents.AddDefaulted_GetRef();
            BoardDamage.Tile = TileActors.FindRef(Event.tileId);
            BoardDamage.Amount = FMath::Max(0, static_cast<int32>(Event.amount));
            BoardDamage.bTerrainObject = Event.definitionId.find(":TerrainObject") != std::string::npos;
        }
        else if (Event.type == roundcore::EventType::DamageApplied)
        {
            const int32 Damage = FMath::Max(0, static_cast<int32>(Event.amount));
            if (TargetId && Event.targetId == *TargetId)
            {
                OutExecution.AppliedDamage += Damage;
            }
            OutExecution.DamageEvents.Add({CharacterActors.FindRef(Event.sourceId),
                CharacterActors.FindRef(Event.targetId), Damage, false});
        }
        else if (Event.type == roundcore::EventType::HealingApplied && TargetId && Event.targetId == *TargetId)
        {
            OutExecution.AppliedHealing += FMath::Max(0, static_cast<int32>(Event.amount));
            if (ARoundCharacter* const* HealedCharacter = CharacterActors.Find(Event.targetId))
            {
                OutExecution.HealingEvents.Add({*HealedCharacter, FMath::Max(0, static_cast<int32>(Event.amount))});
            }
        }
        else if (Event.type == roundcore::EventType::HealingApplied)
        {
            if (ARoundCharacter* const* HealedCharacter = CharacterActors.Find(Event.targetId))
            {
                OutExecution.HealingEvents.Add({*HealedCharacter, FMath::Max(0, static_cast<int32>(Event.amount))});
            }
        }
        else if (Event.type == roundcore::EventType::InitiativeChanged
            && TargetId && Event.targetId == *TargetId && Event.amount < 0)
        {
            OutExecution.TargetInitiativeLoss += -static_cast<int32>(Event.amount);
        }
        else if (Event.type == roundcore::EventType::EffectAdded)
        {
            const auto OwnerState = std::find_if(CoreGame.State().characters.begin(), CoreGame.State().characters.end(),
                [&Event](const roundcore::CharacterState& State) { return State.id == Event.targetId; });
            if (OwnerState != CoreGame.State().characters.end())
            {
                const auto EffectState = std::find_if(OwnerState->modifierEffects.begin(), OwnerState->modifierEffects.end(),
                    [&Event](const roundcore::ModifierEffectState& State) { return State.id == Event.definitionId; });
                ARoundCharacter* const* Owner = CharacterActors.Find(Event.targetId);
                if (EffectState != OwnerState->modifierEffects.end() && Owner)
                {
                    FRoundCoreModifierEffectAddition& Addition = OutExecution.AddedModifierEffects.AddDefaulted_GetRef();
                    Addition.Character = *Owner;
                    Addition.Causer = CharacterActors.FindRef(EffectState->causerId);
                    Addition.PreferredTarget = CharacterActors.FindRef(EffectState->preferredTargetId);
                    Addition.EffectId = UTF8_TO_TCHAR(EffectState->id.c_str());
                    Addition.RemainingInitiative = EffectState->remainingInitiative;
                    Addition.IncomingDamageMultiplierBasisPoints = EffectState->incomingDamageMultiplierBasisPoints;
                    Addition.OutgoingDamageMultiplierBasisPoints = EffectState->outgoingDamageMultiplierBasisPoints;
                    Addition.DefenseIgnoreAdd = EffectState->defenseIgnoreAdd;
                    Addition.HealFractionFromDamageDealtBasisPoints = EffectState->healFractionFromDamageDealtBasisPoints;
                    Addition.MoveCostAdd = EffectState->moveCostAdd;
                    Addition.bPreventsAbilityUse = EffectState->preventsAbilityUse;
                    Addition.RedirectPreMitigationBasisPoints = EffectState->redirectPreMitigationBasisPoints;
                    Addition.RedirectedDamageMultiplierBasisPoints = EffectState->redirectedDamageMultiplierBasisPoints;
                    Addition.ProtectorHealFractionBasisPoints = EffectState->protectorHealFractionBasisPoints;
                    Addition.ProtectedInitiativePerHit = EffectState->protectedInitiativePerHit;
                    Addition.AreaCenterTile = TileActors.FindRef(EffectState->areaCenterTileId);
                    Addition.AreaRadius = EffectState->areaRadius;
                    Addition.AreaDamage = EffectState->areaDamage;
                    Addition.AreaDamageType = ToRoundDamageType(EffectState->areaDamageType);
                    Addition.AreaInitiativeLossPerPulse = EffectState->areaInitiativeLossPerPulse;
                    Addition.bAreaFollowsCauser = EffectState->areaFollowsCauser;
                    Addition.bIsDebuff = EffectState->isDebuff;
                    Addition.bIsBind = EffectState->triggerOnMove;
                    Addition.bIsTaunt = EffectState->kind == roundcore::ModifierEffectKind::Generic
                        && EffectState->preferredTargetId != roundcore::InvalidEntityId;
                    Addition.MoveCostMultiplierBasisPoints = EffectState->moveCostMultiplierBasisPoints;
                    Addition.InitiativeLossOnMove = EffectState->initiativeLossOnMove;
                    Addition.InitiativeLossOnExpire = EffectState->initiativeLossOnExpire;
                    Addition.bBlockMovementBonuses = EffectState->blockMovementBonuses;
                    Addition.bOnlyAffectCauserDamage = EffectState->onlyAffectCauserDamage;
                    Addition.IncomingDamageMultiplierFromCauserBasisPoints = EffectState->incomingDamageMultiplierFromCauserBasisPoints;
                    Addition.IncomingEffectDurationAdd = EffectState->incomingEffectDurationAdd;
                    Addition.TriggerDamage = EffectState->triggerDamageOnMove;
                    Addition.TriggerDelay = EffectState->triggerDelayOnMove;
                    Addition.TriggerOutgoingDamageMultiplierBasisPoints = EffectState->triggerOutgoingDamageMultiplierBasisPoints;
                    Addition.TriggerSlowMultiplierBasisPoints = EffectState->triggerSlowMultiplierBasisPoints;
                    Addition.TriggerSlowDurationInitiative = EffectState->triggerSlowDurationInitiative;
                    Addition.TriggerAreaRadius = EffectState->triggerAreaRadius;
                    Addition.TriggerAreaInitiativeLoss = EffectState->triggerAreaInitiativeLoss;
                    Addition.TriggerInitiativeGainForCauser = EffectState->triggerInitiativeGainForCauser;
                    Addition.InitiativeLossWhenIgnoringCauser = EffectState->initiativeLossWhenIgnoringPreferredTarget;
                    Addition.InitiativeGainForCauserWhenAttacked = EffectState->initiativeGainForCauserWhenAttacked;
                    Addition.OutgoingDamageMultiplierAgainstOtherTargetsBasisPoints = EffectState->outgoingDamageMultiplierAgainstOtherTargetsBasisPoints;
                    CopyCoreEffectMetadata(*EffectState, Addition);
                }
            }
        }
        else if (Event.type == roundcore::EventType::CharacterDied)
        {
            if (TargetId && Event.targetId == *TargetId) OutExecution.bTargetDied = true;
            for (FRoundCoreDamageEvent& DamageEvent : OutExecution.DamageEvents)
            {
                if (DamageEvent.Target == CharacterActors.FindRef(Event.targetId)) DamageEvent.bTargetDied = true;
            }
        }
        else if (Event.type == roundcore::EventType::EffectRemoved)
        {
            const roundcore::EntityId OwnerId = Event.targetId != roundcore::InvalidEntityId
                ? Event.targetId : Event.sourceId;
            if (ARoundCharacter* const* Owner = CharacterActors.Find(OwnerId))
            {
                OutExecution.RemovedEffects.Add({*Owner, UTF8_TO_TCHAR(Event.definitionId.c_str())});
            }
        }
    }
    for (const roundcore::CharacterState& State : CoreGame.State().characters)
    {
        if (ARoundCharacter* const* Actor = CharacterActors.Find(State.id))
        {
            FRoundCoreCharacterRuntimeUpdate& Update = OutExecution.CharacterUpdates.AddDefaulted_GetRef();
            Update.Character = *Actor; Update.Health = State.health; Update.Initiative = State.initiative;
            Update.Tile = TileActors.FindRef(State.tileId);
            Update.bAlive = State.alive; Update.ResourceCurrent = State.resourceCurrent;
            Update.ResourceRechargeProgress = State.resourceRechargeProgress;
            Update.SpecialCooldownRemaining = State.specialCooldownRemaining;
            Update.bFirstResourceSpendRefundAvailable = State.firstResourceSpendRefundAvailable;
            Update.bMovedSinceLastTurn = State.movedSinceLastTurn;
            Update.bStationaryGuardActive = State.stationaryGuardActive;
            CopyExtendedCharacterState(State, Update);
        }
        for (const roundcore::AbilityState& AbilityState : State.abilities)
        {
            const FString RuntimeAbilityKey = UTF8_TO_TCHAR(AbilityState.definitionId.c_str());
            if (ARoundAbilityActor* const* RuntimeAbility = AbilityActors.Find(RuntimeAbilityKey))
            {
                OutExecution.AbilityUpdates.Add({*RuntimeAbility, AbilityState.cooldownRemaining, AbilityState.currentCharges, AbilityState.readyInitiativeElapsed});
            }
        }
    }
    PreservedRandomState = CoreGame.State().randomState;
    OutReason = TEXT("Ability executed by RoundCore.");
    return true;
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
    const auto CharacterState = std::find_if(CoreGame.State().characters.begin(), CoreGame.State().characters.end(),
        [CharacterId](const roundcore::CharacterState& State) { return State.id == *CharacterId; });
    if (CharacterState == CoreGame.State().characters.end())
    {
        OutReason = TEXT("Character state is missing from the Core snapshot.");
        return false;
    }
    for (const roundcore::EntityId TileId : CoreGame.TeamVisibleTileIds(CharacterState->team))
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
    FRoundCoreTurnExecution& OutExecution,
    FString& OutReason)
{
    OutExecution = {};

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
    if (Result.success) SynchronizeDevices();
    if (!Result.success)
    {
        OutReason = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(Result.errorCode.c_str()), UTF8_TO_TCHAR(Result.message.c_str()));
        return false;
    }

    for (const roundcore::GameEvent& Event : Result.events)
    {
        if (Event.type == roundcore::EventType::GlobalInitiativeAdvanced)
        {
            OutExecution.GlobalInitiativeAdvance += static_cast<int32>(Event.amount);
        }
        else if (Event.type == roundcore::EventType::DamageApplied)
        {
            roundcore::DamageType CoreDamageType = roundcore::DamageType::Physical;
            const auto SourceState = std::find_if(CoreGame.State().characters.begin(), CoreGame.State().characters.end(),
                [&Event](const roundcore::CharacterState& State) { return State.id == Event.sourceId; });
            if (SourceState != CoreGame.State().characters.end())
            {
                const auto AreaState = std::find_if(SourceState->modifierEffects.begin(), SourceState->modifierEffects.end(),
                    [&Event](const roundcore::ModifierEffectState& Effect) { return Effect.id == Event.definitionId; });
                if (AreaState != SourceState->modifierEffects.end()) CoreDamageType = AreaState->areaDamageType;
            }
            OutExecution.DamageEvents.Add({CharacterActors.FindRef(Event.sourceId),
                CharacterActors.FindRef(Event.targetId), FMath::Max(0, static_cast<int32>(Event.amount)),
                false, ToRoundDamageType(CoreDamageType)});
        }
        else if (Event.type == roundcore::EventType::CharacterDied)
        {
            for (FRoundCoreDamageEvent& DamageEvent : OutExecution.DamageEvents)
            {
                if (DamageEvent.Target == CharacterActors.FindRef(Event.targetId)) DamageEvent.bTargetDied = true;
            }
        }
        else if (Event.type == roundcore::EventType::InitiativeChanged
            && Event.amount < 0 && !Event.definitionId.empty())
        {
            if (ARoundCharacter* const* Target = CharacterActors.Find(Event.targetId))
            {
                OutExecution.AppliedDelays.Emplace(*Target, -static_cast<int32>(Event.amount));
            }
        }
        else if (Event.type == roundcore::EventType::EffectRemoved)
        {
            if (ARoundCharacter* const* Owner = CharacterActors.Find(Event.targetId))
            {
                OutExecution.RemovedEffects.Add({*Owner, UTF8_TO_TCHAR(Event.definitionId.c_str())});
            }
        }
        else if (Event.type == roundcore::EventType::EffectAdded)
        {
            const auto OwnerState = std::find_if(CoreGame.State().characters.begin(), CoreGame.State().characters.end(),
                [&Event](const roundcore::CharacterState& State) { return State.id == Event.targetId; });
            if (OwnerState != CoreGame.State().characters.end())
            {
                const auto Effect = std::find_if(OwnerState->modifierEffects.begin(), OwnerState->modifierEffects.end(),
                    [&Event](const roundcore::ModifierEffectState& State) { return State.id == Event.definitionId; });
                if (Effect != OwnerState->modifierEffects.end())
                {
                    FRoundCoreModifierEffectAddition& Addition = OutExecution.AddedModifierEffects.AddDefaulted_GetRef();
                    Addition.Character = CharacterActors.FindRef(Event.targetId);
                    Addition.Causer = CharacterActors.FindRef(Effect->causerId);
                    Addition.PreferredTarget = CharacterActors.FindRef(Effect->preferredTargetId);
                    Addition.EffectId = UTF8_TO_TCHAR(Effect->id.c_str());
                    Addition.RemainingInitiative = Effect->remainingInitiative;
                    Addition.OutgoingDamageMultiplierBasisPoints = Effect->outgoingDamageMultiplierBasisPoints;
                    Addition.MoveCostMultiplierBasisPoints = Effect->moveCostMultiplierBasisPoints;
                    Addition.bIsDebuff = Effect->isDebuff;
                    CopyCoreEffectMetadata(*Effect, Addition);
                }
            }
        }
    }
    for (const roundcore::CharacterState& State : CoreGame.State().characters)
    {
        if (ARoundCharacter* const* Character = CharacterActors.Find(State.id))
        {
            FRoundCoreCharacterRuntimeUpdate& Update = OutExecution.CharacterUpdates.AddDefaulted_GetRef();
            Update.Character = *Character; Update.Health = State.health; Update.Initiative = State.initiative;
            Update.bAlive = State.alive; Update.ResourceCurrent = State.resourceCurrent;
            Update.ResourceRechargeProgress = State.resourceRechargeProgress;
            Update.SpecialCooldownRemaining = State.specialCooldownRemaining;
            Update.bFirstResourceSpendRefundAvailable = State.firstResourceSpendRefundAvailable;
            Update.bMovedSinceLastTurn = State.movedSinceLastTurn;
            Update.bStationaryGuardActive = State.stationaryGuardActive;
            CopyExtendedCharacterState(State, Update);
        }
        for (const roundcore::AbilityState& AbilityState : State.abilities)
        {
            const FString AbilityKey = UTF8_TO_TCHAR(AbilityState.definitionId.c_str());
            if (ARoundAbilityActor* const* Ability = AbilityActors.Find(AbilityKey))
            {
                FRoundCoreAbilityRuntimeUpdate& Update = OutExecution.AbilityUpdates.AddDefaulted_GetRef();
                Update.Ability = *Ability;
                Update.Cooldown = AbilityState.cooldownRemaining;
                Update.Charges = AbilityState.currentCharges;
                Update.ReadyInitiativeElapsed = AbilityState.readyInitiativeElapsed;
            }
        }
    }
    if (ARoundCharacter* const* Active = CharacterActors.Find(CoreGame.State().activeCharacterId))
    {
        OutExecution.ActiveCharacter = *Active;
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
    FRoundCoreMoveExecution& OutExecution,
    FString& OutReason)
{
    OutExecution = {};
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
    if (Result.success) SynchronizeDevices();
    if (!Result.success)
    {
        OutReason = FString::Printf(TEXT("%s: %s"), UTF8_TO_TCHAR(Result.errorCode.c_str()), UTF8_TO_TCHAR(Result.message.c_str()));
        return false;
    }

    for (const roundcore::GameEvent& Event : Result.events)
    {
        if (Event.type == roundcore::EventType::CharacterMoved && Event.sourceId == *CharacterId)
        {
            OutExecution.MovementCost = static_cast<int32>(Event.amount);
        }
        else if (Event.type == roundcore::EventType::DamageApplied)
        {
            OutExecution.DamageEvents.Add({CharacterActors.FindRef(Event.sourceId), CharacterActors.FindRef(Event.targetId),
                FMath::Max(0, static_cast<int32>(Event.amount)), false});
        }
        else if (Event.type == roundcore::EventType::CharacterDied)
        {
            for (FRoundCoreDamageEvent& Damage : OutExecution.DamageEvents)
            {
                if (Damage.Target == CharacterActors.FindRef(Event.targetId)) Damage.bTargetDied = true;
            }
        }
        else if (Event.type == roundcore::EventType::EffectRemoved)
        {
            if (ARoundCharacter* const* Owner = CharacterActors.Find(Event.targetId))
            {
                OutExecution.RemovedEffects.Add({*Owner, UTF8_TO_TCHAR(Event.definitionId.c_str())});
            }
        }
        else if (Event.type == roundcore::EventType::EffectAdded)
        {
            const auto OwnerState = std::find_if(CoreGame.State().characters.begin(), CoreGame.State().characters.end(),
                [&Event](const roundcore::CharacterState& State) { return State.id == Event.targetId; });
            if (OwnerState != CoreGame.State().characters.end())
            {
                const auto Effect = std::find_if(OwnerState->modifierEffects.begin(), OwnerState->modifierEffects.end(),
                    [&Event](const roundcore::ModifierEffectState& State) { return State.id == Event.definitionId; });
                if (Effect != OwnerState->modifierEffects.end())
                {
                    FRoundCoreModifierEffectAddition& Addition = OutExecution.AddedModifierEffects.AddDefaulted_GetRef();
                    Addition.Character = CharacterActors.FindRef(Event.targetId);
                    Addition.Causer = CharacterActors.FindRef(Effect->causerId);
                    Addition.PreferredTarget = CharacterActors.FindRef(Effect->preferredTargetId);
                    Addition.EffectId = UTF8_TO_TCHAR(Effect->id.c_str());
                    Addition.RemainingInitiative = Effect->remainingInitiative;
                    Addition.OutgoingDamageMultiplierBasisPoints = Effect->outgoingDamageMultiplierBasisPoints;
                    Addition.MoveCostMultiplierBasisPoints = Effect->moveCostMultiplierBasisPoints;
                    Addition.bIsDebuff = Effect->isDebuff;
                    CopyCoreEffectMetadata(*Effect, Addition);
                }
            }
        }
    }
    for (const roundcore::CharacterState& State : CoreGame.State().characters)
    {
        if (ARoundCharacter* const* Actor = CharacterActors.Find(State.id))
        {
            FRoundCoreCharacterRuntimeUpdate& Update = OutExecution.CharacterUpdates.AddDefaulted_GetRef();
            Update.Character = *Actor; Update.Tile = TileActors.FindRef(State.tileId);
            Update.Health = State.health; Update.Initiative = State.initiative; Update.bAlive = State.alive;
            Update.ResourceCurrent = State.resourceCurrent; Update.ResourceRechargeProgress = State.resourceRechargeProgress;
            Update.SpecialCooldownRemaining = State.specialCooldownRemaining;
            Update.bFirstResourceSpendRefundAvailable = State.firstResourceSpendRefundAvailable;
            Update.bMovedSinceLastTurn = State.movedSinceLastTurn; Update.bStationaryGuardActive = State.stationaryGuardActive;
            CopyExtendedCharacterState(State, Update);
            if (State.id == *CharacterId) OutExecution.Initiative = State.initiative;
        }
    }
    PreservedRandomState = CoreGame.State().randomState;
    OutReason = TEXT("Movement executed by RoundCore.");
    return true;
}
