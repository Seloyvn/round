#include "Game/RoundGameState.h"
#include "UObject/UnrealType.h"
#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter1.h"
#include "Actors/RoundCharacter2.h"
#include "Actors/RoundCharacter3.h"
#include "Actors/RoundCharacter4.h"
#include "Actors/RoundCharacter5.h"
#include "Actors/RoundCharacter6.h"
#include "Actors/RoundCharacterChronomancer.h"
#include "Actors/RoundCharacterDruid.h"
#include "Actors/RoundAbilityActor.h"
#include "Abilities/RoundSnipeAbility.h"
#include "Actors/RoundGridTile.h"
#include "Actors/RoundFieldObjectActor.h"
#include "Actors/RoundObjectiveActor.h"
#include "Actors/RoundObjectiveRewardActor.h"
#include "Actors/RoundTerrainActor.h"
#include "Actors/RoundTeamActor.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundObjectiveRewardData.h"
#include "Data/RoundAbilityData.h"
#include "Data/RoundCharacterData.h"
#include "Data/RoundGameDataRegistry.h"
#include "Data/RoundGameDataRegistryLookup.h"
#include "Data/RoundCharacterRegistryLookup.h"
#include "Data/RoundCharacterProgressionData.h"
#include "Data/RoundPresentationProfileData.h"
#include "Data/RoundSkillProgressionData.h"
#include "Data/RoundTerrainData.h"
#include "Data/RoundTerrainRegionData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Effects/RoundBindEffect.h"
#include "Effects/RoundTauntEffect.h"
#include "Effects/RoundAuraEffect.h"
#include "Effects/RoundModifierEffect.h"
#include "Effects/RoundObjectiveStatRewardEffect.h"
#include "Effects/RoundSilenceEffect.h"
#include "Effects/RoundShieldOathEffect.h"
#include "Effects/RoundSanctifiedGroundAuraEffect.h"
#include "Effects/RoundSlowEffect.h"
#include "Effects/RoundInvisibilityEffect.h"
#include "Interfaces/RoundBoardDamageableInterface.h"
#include "Triggers/RoundAimFieldTriggerActor.h"
#include "Game/RoundGameInstance.h"
#include "Game/RoundPlayerPawn.h"
#include "Core/RoundCoreMatchAdapter.h"
#include "round/core/Package.h"
#include "Algo/Reverse.h"
#include "Algo/Sort.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "JsonObjectConverter.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/DefaultValueHelper.h"
#include "Internationalization/Regex.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/SavePackage.h"

#include <exception>

DEFINE_LOG_CATEGORY_STATIC(LogRoundAuraRuntime, Log, All);

namespace
{
void ApplyCharacter2CoreState(const FRoundCoreCharacterRuntimeUpdate& Update)
{
    if (ARoundCharacter2* Character2 = Cast<ARoundCharacter2>(Update.Character))
    {
        Character2->Ammo = Update.ResourceCurrent;
        Character2->bAttackedThisTurn = Update.bAttackedThisTurn;
        Character2->bSpentAmmoThisTurn = Update.bSpentResourceThisTurn;
        Character2->bMovedThisTurn = Update.bMovedSinceLastTurn;
        Character2->bStartedTurnWithFullAmmo = Update.bStartedTurnWithFullResource;
        Character2->bHeldPositionFromPreviousTurn = Update.bHeldPositionFromPreviousTurn;
        Character2->bUsedFreeBasicAttackThisTurn = Update.bUsedFreeBasicAttackThisTurn;
        Character2->bTriggeredEmptyMagazineThisTurn = Update.bTriggeredEmptyResourceThisTurn;
        Character2->bRefundedLastAmmoKillThisTurn = Update.bRefundedLastResourceKillThisTurn;
        Character2->bUsedSnipeRangeRefundThisTurn = Update.bUsedRangeRefundThisTurn;
        Character2->bUsedSnipeKillCooldownReductionThisTurn = Update.bUsedKillCooldownReductionThisTurn;
        Character2->StationaryShotStacks = Update.StationaryShotStacks;
        Character2->bHasFreeConcentrateStep = Update.bHasFreePreparedStep;
    }
}

void ApplyCharacter3CoreState(const FRoundCoreCharacterRuntimeUpdate& Update)
{
    if (ARoundCharacter3* Character3 = Cast<ARoundCharacter3>(Update.Character))
    {
        Character3->ApplyCoreAssassinState(Update.ResourceCurrent, Update.MovedFieldsThisTurn,
            Update.ResourceSpentThisTurnCount, Update.SecondaryCooldownRemaining, Update.TurnSequence,
            Update.bOpeningRewardUsedThisTurn, Update.bOverflowRewardUsedThisTurn,
            Update.bFreeAdjacentMoveAvailable, Update.bFreeAdjacentMoveUsedThisTurn,
            Update.bNextDamageAfterMoveReady, Update.bNextReductionAfterMoveReady,
            Update.bStartInvisibilityApplied);
    }
}

void ApplyCharacter4CoreState(const FRoundCoreCharacterRuntimeUpdate& Update)
{
    if (ARoundCharacter4* Character4 = Cast<ARoundCharacter4>(Update.Character))
    {
        Character4->ApplyCoreMageState(Update.ActiveElement, Update.UsedElementMask, Update.LastCastElement,
            Update.FirstCastElement, Update.ElementSwapCooldownRemaining, Update.RockDefenseBonus,
            Update.RockDefenseRemainingInitiative, Update.ProtectionRemainingInitiative,
            Update.EchoElement, Update.ElementCycleMask, Update.bElementCostReductionUsed,
            Update.bElementSwapRefundUsed, Update.bStoredElementEmpowered, Update.bProtectionReady);
    }
}

void ApplyChronomancerCoreState(const FRoundCoreCharacterRuntimeUpdate& Update)
{
    if (ARoundCharacterChronomancer* Chronomancer = Cast<ARoundCharacterChronomancer>(Update.Character))
    {
        Chronomancer->ApplyCoreChronomancerState(Update.ResourceCurrent,
            Update.bFirstResourceDiscountAvailable, Update.bFirstResourceSpendRefundAvailable);
    }
}

void PresentCoreGenericModifier(UWorld* World, const FRoundCoreModifierEffectAddition& Addition,
    const FString& DisplayName, const FString& Description)
{
    if (!World || !Addition.Character)
    {
        return;
    }
    const TArray<TObjectPtr<ARoundEffectActor>> ExistingEffects = Addition.Character->ActiveEffects;
    for (ARoundEffectActor* ExistingEffect : ExistingEffects)
    {
        if (ExistingEffect && ExistingEffect->EffectId.ToString() == Addition.EffectId)
        {
            ExistingEffect->RemoveEffect();
        }
    }
    if (Addition.CoreEffectKind == static_cast<int32>(roundcore::ModifierEffectKind::Invisibility))
    {
        if (ARoundInvisibilityEffect* Effect = World->SpawnActor<ARoundInvisibilityEffect>())
        {
            Effect->EffectId = FName(*Addition.EffectId);
            Effect->DisplayName = TEXT("Unsichtbar");
            Effect->Description = TEXT("Kann ohne Sichtaufdeckung nicht gesehen werden.");
            Effect->CauserCharacter = Addition.Causer;
            Effect->SetDurationInitiative(Addition.RemainingInitiative, false);
            Addition.Character->AddEffect(Effect);
            Effect->StringState.FindOrAdd(TEXT("CoreSourceAbilityId")) = Addition.SourceAbilityId;
            Effect->IntegerState.FindOrAdd(TEXT("CoreEffectKind")) = Addition.CoreEffectKind;
            Effect->IntegerState.FindOrAdd(TEXT("CoreFlags")) = Addition.Flags;
        }
        return;
    }
    ARoundModifierEffect* Effect = World->SpawnActor<ARoundModifierEffect>();
    if (!Effect)
    {
        return;
    }
    Effect->EffectId = FName(*Addition.EffectId);
    Effect->DisplayName = DisplayName;
    Effect->Description = Description;
    Effect->IncomingDamageMultiplier = Addition.IncomingDamageMultiplierBasisPoints / 10000.0;
    Effect->OutgoingDamageMultiplier = Addition.OutgoingDamageMultiplierBasisPoints / 10000.0;
    Effect->DefenseIgnoreAdd = Addition.DefenseIgnoreAdd;
    Effect->HealFractionFromDamageDealt = Addition.HealFractionFromDamageDealtBasisPoints / 10000.0;
    Effect->MoveCostAdd = Addition.MoveCostAdd;
    Effect->IncomingEffectDurationAdd = Addition.IncomingEffectDurationAdd;
    Effect->bOnlyAffectCauserDamage = Addition.bOnlyAffectCauserDamage;
    Effect->CauserCharacter = Addition.Causer;
    Effect->bTreatAsBuff = !Addition.bIsDebuff;
    Effect->bTreatAsDebuff = Addition.bIsDebuff;
    Effect->SetDurationInitiative(Addition.RemainingInitiative, false);
    Addition.Character->AddEffect(Effect);
    Effect->StringState.FindOrAdd(TEXT("CoreSourceAbilityId")) = Addition.SourceAbilityId;
    Effect->StringState.FindOrAdd(TEXT("CorePreferredTarget")) = Addition.PreferredTarget
        ? Addition.PreferredTarget->GetPathName() : FString();
    Effect->IntegerState.FindOrAdd(TEXT("CoreEffectKind")) = Addition.CoreEffectKind;
    Effect->IntegerState.FindOrAdd(TEXT("CoreCounterA")) = Addition.CounterA;
    Effect->IntegerState.FindOrAdd(TEXT("CoreCounterB")) = Addition.CounterB;
    Effect->IntegerState.FindOrAdd(TEXT("CoreCounterC")) = Addition.CounterC;
    Effect->IntegerState.FindOrAdd(TEXT("CoreValueA")) = Addition.ValueA;
    Effect->IntegerState.FindOrAdd(TEXT("CoreValueB")) = Addition.ValueB;
    Effect->IntegerState.FindOrAdd(TEXT("CoreValueC")) = Addition.ValueC;
    Effect->IntegerState.FindOrAdd(TEXT("CoreFlags")) = Addition.Flags;
}

void RemoveCoreAimPresentation(ARoundCharacter* Character, const FString& EffectId)
{
    if (!Character || !Character->GetWorld()) return;
    for (TActorIterator<ARoundAimFieldTriggerActor> It(Character->GetWorld()); It; ++It)
    {
        if (It->CauserCharacter == Character && It->bPresentationOnly && It->CoreEffectId == EffectId)
        {
            It->RemoveTrigger();
            break;
        }
    }
}
}

ARoundGameState* ARoundGameState::GetRoundGameState(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (const UWorld* World = WorldContextObject->GetWorld())
    {
        return World->GetGameState<ARoundGameState>();
    }

    return nullptr;
}

namespace
{
constexpr float RoundPresentationTurnStartDurationSeconds = 0.08f;
constexpr float RoundPresentationTurnEndDurationSeconds = 0.05f;
constexpr float RoundPresentationMoveStepDurationSeconds = 0.10f;
constexpr float RoundPresentationAttackDurationSeconds = 0.16f;
constexpr float RoundPresentationAbilityDurationSeconds = 0.20f;
constexpr float RoundPresentationDamageDurationSeconds = 0.14f;
constexpr float RoundPresentationHealDurationSeconds = 0.14f;
constexpr float RoundPresentationDelayDurationSeconds = 0.06f;
constexpr float RoundPresentationDeathDurationSeconds = 0.12f;
constexpr float RoundPresentationAttackLungeDistance = 65.0f;
constexpr float RoundPresentationMoveHopHeight = 34.0f;
constexpr float RoundPresentationDamageScalePunch = 0.18f;
constexpr float RoundPresentationHealScalePunch = 0.12f;
constexpr float RoundPresentationTurnPulseDurationSeconds = 0.30f;
constexpr float RoundPresentationBeamDurationSeconds = 0.26f;
constexpr float RoundPresentationImpactPulseDurationSeconds = 0.28f;
constexpr float RoundFloatingTextDurationSeconds = 0.85f;
constexpr float RoundFloatingTextRiseHeight = 95.0f;
constexpr float RoundPresentationCylinderHeight = 200.0f;

int64 SetPackedTierSelectionForTraining(const int64 PackedValue, const int32 TierIndex, const int32 SelectionValue)
{
    const int64 Shift = TierIndex * 2;
    const int64 Cleared = PackedValue & ~(static_cast<int64>(0x3) << Shift);
    return Cleared | ((static_cast<int64>(SelectionValue) & 0x3) << Shift);
}

int32 CountSelectedTiersForTraining(const int64 PackedValue)
{
    int32 Count = 0;
    for (int32 TierIndex = 0; TierIndex < 4; ++TierIndex)
    {
        if (((PackedValue >> (TierIndex * 2)) & 0x3) == 0)
        {
            break;
        }

        ++Count;
    }

    return Count;
}

FString NormalizeTrainingCharacterKey(FString Value)
{
    Value = Value.TrimStartAndEnd();
    Value.ReplaceInline(TEXT("BP_"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("_C"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT(" "), TEXT(""));
    Value.ReplaceInline(TEXT("_"), TEXT(""));
    Value.ReplaceInline(TEXT("-"), TEXT(""));

    FString Result;
    Result.Reserve(Value.Len());
    for (const TCHAR Character : Value)
    {
        if (FChar::IsAlnum(Character))
        {
            Result.AppendChar(FChar::ToLower(Character));
        }
    }

    return Result;
}

URoundCharacterData* LoadTrainingCharacterDataById(const FString& CharacterId)
{
    return RoundCharacterRegistry::FindCharacterDataById(FName(*CharacterId));
}

int32 GetTrainingPassiveTierCount(const URoundCharacterData* CharacterData)
{
    if (!CharacterData)
    {
        return 0;
    }

    if (CharacterData->CharacterProgression)
    {
        int32 Count = 0;
        for (const FRoundSkillpointTier& Tier : CharacterData->CharacterProgression->Tiers)
        {
            if (Tier.Choices.Num() <= 0 || Count >= 4)
            {
                break;
            }

            ++Count;
        }
        return Count;
    }

    if (CharacterData->SkillpointsTable)
    {
        TArray<FRoundSkillpointsDetails*> Rows;
        CharacterData->SkillpointsTable->GetAllRows(TEXT("TrainingPassiveTierCount"), Rows);
        if (Rows.Num() > 0)
        {
            return FMath::Clamp(Rows[0]->Skillpoints.Num() / 3, 0, 4);
        }
    }

    return 0;
}

int32 GetTrainingPassiveChoiceCount(const URoundCharacterData* CharacterData, const int32 TierIndex)
{
    if (!CharacterData || TierIndex < 0 || TierIndex >= 4)
    {
        return 0;
    }

    if (CharacterData->CharacterProgression && CharacterData->CharacterProgression->Tiers.IsValidIndex(TierIndex))
    {
        return FMath::Clamp(CharacterData->CharacterProgression->Tiers[TierIndex].Choices.Num(), 0, 3);
    }

    if (CharacterData->SkillpointsTable)
    {
        TArray<FRoundSkillpointsDetails*> Rows;
        CharacterData->SkillpointsTable->GetAllRows(TEXT("TrainingPassiveChoiceCount"), Rows);
        if (Rows.Num() > 0)
        {
            const FRoundSkillpointsDetails* Details = Rows[0];
            int32 Count = 0;
            for (int32 ChoiceIndex = 0; ChoiceIndex < 3; ++ChoiceIndex)
            {
                if (Details->Skillpoints.IsValidIndex((TierIndex * 3) + ChoiceIndex))
                {
                    ++Count;
                }
            }
            return Count;
        }
    }

    return 0;
}

int32 GetTrainingAbilityTierCount(const URoundAbilityData* AbilityData)
{
    if (!AbilityData || !AbilityData->SkillProgression)
    {
        return 0;
    }

    return FMath::Clamp(AbilityData->SkillProgression->Levels.Num() / 3, 0, 4);
}

int32 GetTrainingAbilityChoiceCount(const URoundAbilityData* AbilityData, const int32 TierIndex)
{
    if (!AbilityData || !AbilityData->SkillProgression || TierIndex < 0 || TierIndex >= 4)
    {
        return 0;
    }

    int32 Count = 0;
    for (int32 ChoiceIndex = 0; ChoiceIndex < 3; ++ChoiceIndex)
    {
        if (AbilityData->SkillProgression->Levels.IsValidIndex((TierIndex * 3) + ChoiceIndex))
        {
            ++Count;
        }
    }
    return Count;
}

FRoundCharacterEntry BuildRandomTrainingCharacterEntry(
    const FString& CharacterId,
    const FVector2D& StartOffset,
    const int32 SkillBudget,
    const int32 AllowedAbilityCount,
    FRandomStream& RandomStream)
{
    (void)AllowedAbilityCount;

    FRoundCharacterEntry Result;
    Result.Character = CharacterId;
    Result.StartPosition = StartOffset;

    URoundCharacterData* CharacterData = LoadTrainingCharacterDataById(CharacterId);
    if (!CharacterData)
    {
        return Result;
    }

    struct FSkillChannel
    {
        int32 CurrentTier = 0;
        int32 MaxTiers = 0;
        TFunction<int32(int32)> GetChoiceCount;
        int64* TargetMask = nullptr;
    };

    int64 PassiveMask = 0;
    const int32 AbilityCountToExpose = CharacterData->Abilities.Num();
    TArray<int64> AbilityMasks;
    AbilityMasks.Init(0, AbilityCountToExpose);

    TArray<FSkillChannel> Channels;
    Channels.Reserve(1 + CharacterData->Abilities.Num());

    const int32 PassiveTierCount = GetTrainingPassiveTierCount(CharacterData);
    if (PassiveTierCount > 0)
    {
        Channels.Add(FSkillChannel{
            0,
            PassiveTierCount,
            [CharacterData](const int32 TierIndex) { return GetTrainingPassiveChoiceCount(CharacterData, TierIndex); },
            &PassiveMask
        });
    }

    for (int32 AbilityIndex = 0; AbilityIndex < AbilityCountToExpose; ++AbilityIndex)
    {
        URoundAbilityData* AbilityData = CharacterData->Abilities[AbilityIndex];
        const int32 TierCount = GetTrainingAbilityTierCount(AbilityData);
        if (TierCount <= 0)
        {
            continue;
        }

        Channels.Add(FSkillChannel{
            0,
            TierCount,
            [AbilityData](const int32 TierIndex) { return GetTrainingAbilityChoiceCount(AbilityData, TierIndex); },
            &AbilityMasks[AbilityIndex]
        });
    }

    int32 RemainingBudget = FMath::Max(0, SkillBudget);
    while (RemainingBudget > 0)
    {
        TArray<int32> AvailableChannels;
        for (int32 ChannelIndex = 0; ChannelIndex < Channels.Num(); ++ChannelIndex)
        {
            const FSkillChannel& Channel = Channels[ChannelIndex];
            if (Channel.CurrentTier < Channel.MaxTiers && Channel.GetChoiceCount(Channel.CurrentTier) > 0)
            {
                AvailableChannels.Add(ChannelIndex);
            }
        }

        if (AvailableChannels.Num() <= 0)
        {
            break;
        }

        FSkillChannel& Channel = Channels[AvailableChannels[RandomStream.RandRange(0, AvailableChannels.Num() - 1)]];
        const int32 ChoiceCount = Channel.GetChoiceCount(Channel.CurrentTier);
        const int32 Selection = RandomStream.RandRange(1, FMath::Max(1, ChoiceCount));
        *Channel.TargetMask = SetPackedTierSelectionForTraining(*Channel.TargetMask, Channel.CurrentTier, Selection);
        ++Channel.CurrentTier;
        --RemainingBudget;
    }

    Result.CharacterSkillSelections = PassiveMask;
    Result.CharacterSkillpoints = CountSelectedTiersForTraining(PassiveMask);
    Result.Skillpoints = 0;

    for (int32 AbilityIndex = 0; AbilityIndex < AbilityCountToExpose; ++AbilityIndex)
    {
        const URoundAbilityData* AbilityData = CharacterData->Abilities[AbilityIndex];
        if (!AbilityData)
        {
            continue;
        }

        FRoundSkillEntry SkillEntry;
        SkillEntry.Name = AbilityData->AbilityId.ToString();
        SkillEntry.Points = AbilityMasks[AbilityIndex];
        Result.Skills.Add(MoveTemp(SkillEntry));
        Result.Skillpoints += CountSelectedTiersForTraining(AbilityMasks[AbilityIndex]);
    }

    return Result;
}

FRoundTeamDefinition BuildRandomTrainingTeamDefinition(
    const FString& TeamName,
    const FString& CharacterId,
    const FVector2D& StartOffset,
    const FRoundFormatDefinition& FormatDefinition,
    FRandomStream& RandomStream)
{
    FRoundTeamDefinition TeamDefinition;
    TeamDefinition.Name = TeamName;
    TeamDefinition.Format = ERoundFormat::Formattest;
    TeamDefinition.Characters.Add(BuildRandomTrainingCharacterEntry(
        CharacterId,
        StartOffset,
        FormatDefinition.Skillpoints,
        FormatDefinition.SkillAmount,
        RandomStream));
    return TeamDefinition;
}

FRoundFormatDefinition BuildRandomTrainingFormatDefinition(const URoundGameInstance* GameInstance)
{
    FRoundFormatDefinition FormatDefinition;
    if (GameInstance && GameInstance->TryGetFormatDefinition(ERoundFormat::Formattest, FormatDefinition))
    {
        return FormatDefinition;
    }

    if (URoundGameInstance::TryGetBuiltInFormatDefinition(ERoundFormat::Formattest, FormatDefinition))
    {
        return FormatDefinition;
    }

    FormatDefinition.Format = ERoundFormat::Formattest;
    FormatDefinition.Name = TEXT("Formattest");
    FormatDefinition.Characters = 1;
    FormatDefinition.Skillpoints = 16;
    FormatDefinition.FieldSize = 19;
    FormatDefinition.Terrain = 0;
    FormatDefinition.StartPosition = FVector2D::ZeroVector;
    FormatDefinition.SkillAmount = 4;
    FormatDefinition.TerrainWeights.Add(ERoundTerrainType::Ground, 1.0);
    return FormatDefinition;
}

FString ResolveGeneratedTrainingStateExportPath(const FString& RequestedPath, const int32 Index, const int32 Seed)
{
    FString Directory = RequestedPath.TrimStartAndEnd();
    if (Directory.IsEmpty())
    {
        Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SimStates"), TEXT("GeneratedTraining"));
    }
    else if (FPaths::IsRelative(Directory))
    {
        Directory = FPaths::Combine(FPaths::ProjectDir(), Directory);
    }

    IFileManager::Get().MakeDirectory(*Directory, true);
    return FPaths::Combine(Directory, FString::Printf(TEXT("generated_train_%04d_seed_%d.json"), Index, Seed));
}

FString ResolveReplayImportPath(const FString& RequestedPath)
{
    if (RequestedPath.IsEmpty())
    {
        return FString();
    }

    return FPaths::IsRelative(RequestedPath)
        ? FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), RequestedPath)
        : RequestedPath;
}

TAutoConsoleVariable<FString> CVarRoundPythonExe(
    TEXT("round.python.exe"),
    TEXT("C:/Users/marco/AppData/Local/Programs/Python/Python313/python.exe"),
    TEXT("Path to the Python executable used for live action selection."));

TAutoConsoleVariable<FString> CVarRoundPythonActionScript(
    TEXT("round.python.action_script"),
    TEXT("C:/Users/marco/Documents/Unreal Projects/round/Python/scripts/live_action.py"),
    TEXT("Path to the Python script that selects one live action."));

TAutoConsoleVariable<FString> CVarRoundPythonPolicy(
    TEXT("round.python.policy"),
    TEXT("latest"),
    TEXT("Policy name passed to the live Python action selector."));

TAutoConsoleVariable<float> CVarRoundPythonAutoplayDelaySeconds(
    TEXT("round.python.autoplay_delay_seconds"),
    0.3f,
    TEXT("Delay between automatic live python actions in seconds for non-EndTurn actions."));

TAutoConsoleVariable<float> CVarRoundPythonAutoplayEndTurnDelaySeconds(
    TEXT("round.python.autoplay_endturn_delay_seconds"),
    1.0f,
    TEXT("Delay after automatic live python EndTurn actions in seconds."));

struct FCubeCoord
{
    double X = 0.0;
    double Y = 0.0;
    double Z = 0.0;
};

bool IsFieldFree(const ARoundGridTile* Field)
{
    return Field && Field->IsWalkable();
}

double GetRuntimeGlobalDamageEscalationMultiplierForInitiative(const int32 GlobalInitiativeElapsed)
{
    if (GlobalInitiativeElapsed < 10000)
    {
        return 1.0;
    }

    const int32 ExtraThresholdsReached = 1 + FMath::FloorToInt(static_cast<float>(GlobalInitiativeElapsed - 10000) / 1000.0f);
    return 1.0 + (0.1 * static_cast<double>(ExtraThresholdsReached));
}

int32 GetTerrainPriority(const ERoundTerrainType TerrainType)
{
    switch (TerrainType)
    {
    case ERoundTerrainType::Ground:
    case ERoundTerrainType::Meadow:
        return 0;
    case ERoundTerrainType::Forest:
        return 1;
    case ERoundTerrainType::DenseForest:
        return 2;
    case ERoundTerrainType::Thorns:
        return 3;
    case ERoundTerrainType::Grove:
        return 3;
    case ERoundTerrainType::Street:
        return 4;
    case ERoundTerrainType::Fire:
        return 5;
    case ERoundTerrainType::Rocks:
        return 6;
    case ERoundTerrainType::Rubble:
        return 7;
    case ERoundTerrainType::Wall:
        return 8;
    case ERoundTerrainType::Mountain:
        return 9;
    default:
        return MAX_int32;
    }
}

template <typename TValueType>
void ShuffleArrayInPlace(TArray<TValueType>& Values, FRandomStream& RandomStream)
{
    for (int32 Index = Values.Num() - 1; Index > 0; --Index)
    {
        const int32 SwapIndex = RandomStream.RandRange(0, Index);
        Values.Swap(Index, SwapIndex);
    }
}

TArray<FVector2D> GetHexNeighborOffsets()
{
    return {
        FVector2D(1.f, 0.f),
        FVector2D(1.f, -1.f),
        FVector2D(0.f, -1.f),
        FVector2D(-1.f, 0.f),
        FVector2D(-1.f, 1.f),
        FVector2D(0.f, 1.f)
    };
}

TMap<ERoundTerrainType, float> BuildFallbackTerrainWeights()
{
    TMap<ERoundTerrainType, float> Result;
    Result.Add(ERoundTerrainType::Ground, 0.55f);
    Result.Add(ERoundTerrainType::Forest, 0.12f);
    Result.Add(ERoundTerrainType::Street, 0.10f);
    Result.Add(ERoundTerrainType::Fire, 0.08f);
    Result.Add(ERoundTerrainType::Rocks, 0.10f);
    Result.Add(ERoundTerrainType::Rubble, 0.05f);
    return Result;
}

FString NormalizeCharacterKey(FString Value)
{
    Value = Value.ToLower();
    Value.ReplaceInline(TEXT("_"), TEXT(""));
    Value.ReplaceInline(TEXT("bp"), TEXT(""));
    Value.ReplaceInline(TEXT("charcter"), TEXT("character"));
    return Value;
}

FCubeCoord AxialToCube(const FVector2D& Axial)
{
    return { Axial.X, -Axial.X - Axial.Y, Axial.Y };
}

int32 GetAxialHexDistanceFromOrigin(const FVector2D& Axial)
{
    const int32 X = FMath::RoundToInt(Axial.X);
    const int32 Y = FMath::RoundToInt(Axial.Y);
    const int32 Z = -X - Y;
    return FMath::Max3(FMath::Abs(X), FMath::Abs(Y), FMath::Abs(Z));
}

int32 GetAxialHexDistance(const FVector2D& A, const FVector2D& B)
{
    return GetAxialHexDistanceFromOrigin(A - B);
}

void ResolveTerrainGenerationTeams(
    const URoundGameInstance* RoundGameInstance,
    const FRoundTeamDefinition*& OutTeamOneDefinition,
    const FRoundTeamDefinition*& OutTeamTwoDefinition)
{
    OutTeamOneDefinition = nullptr;
    OutTeamTwoDefinition = nullptr;

    if (!RoundGameInstance)
    {
        return;
    }

    if (RoundGameInstance->LoadedTeams.IsValidIndex(0))
    {
        OutTeamOneDefinition = &RoundGameInstance->LoadedTeams[0];
    }
    else
    {
        OutTeamOneDefinition = &RoundGameInstance->StrTeam;
    }

    if (RoundGameInstance->LoadedTeams.IsValidIndex(1))
    {
        OutTeamTwoDefinition = &RoundGameInstance->LoadedTeams[1];
    }
}

TArray<FVector2D> BuildProtectedTerrainSpawnPositions(const ARoundGameState& GameState)
{
    TArray<FVector2D> Result;

    const URoundGameInstance* RoundGameInstance = GameState.GetGameInstance<URoundGameInstance>();
    if (!RoundGameInstance)
    {
        return Result;
    }

    const FRoundTeamDefinition* TeamOneDefinition = nullptr;
    const FRoundTeamDefinition* TeamTwoDefinition = nullptr;
    ResolveTerrainGenerationTeams(RoundGameInstance, TeamOneDefinition, TeamTwoDefinition);

    ERoundFormat ActiveFormat = ERoundFormat::Format3x16;
    if (TeamOneDefinition)
    {
        ActiveFormat = TeamOneDefinition->Format;
    }
    else
    {
        ActiveFormat = RoundGameInstance->StrTeam.Format;
    }

    FRoundFormatDefinition ActiveFormatDefinition;
    if (!RoundGameInstance->TryGetFormatDefinition(ActiveFormat, ActiveFormatDefinition))
    {
        return Result;
    }

    const auto AddProtectedPositions = [&Result](
        const FRoundTeamDefinition* TeamDefinition,
        const FVector2D& TeamAnchorPosition,
        const FVector2D& PositionMultiplier)
    {
        if (!TeamDefinition)
        {
            return;
        }

        for (const FRoundCharacterEntry& CharacterEntry : TeamDefinition->Characters)
        {
            Result.AddUnique(TeamAnchorPosition + (CharacterEntry.StartPosition * PositionMultiplier));
        }
    };

    AddProtectedPositions(TeamOneDefinition, ActiveFormatDefinition.StartPosition, FVector2D(1.f, 1.f));
    AddProtectedPositions(TeamTwoDefinition, ActiveFormatDefinition.StartPosition * -1.f, FVector2D(-1.f, -1.f));
    return Result;
}

TArray<FVector2D> BuildTerrainStartRegionCenters(const ARoundGameState& GameState)
{
    TArray<FVector2D> Result;
    const URoundGameInstance* RoundGameInstance = GameState.GetGameInstance<URoundGameInstance>();
    if (!RoundGameInstance)
    {
        return Result;
    }

    const FRoundTeamDefinition* TeamOneDefinition = nullptr;
    const FRoundTeamDefinition* TeamTwoDefinition = nullptr;
    ResolveTerrainGenerationTeams(RoundGameInstance, TeamOneDefinition, TeamTwoDefinition);

    const ERoundFormat ActiveFormat = TeamOneDefinition
        ? TeamOneDefinition->Format
        : RoundGameInstance->StrTeam.Format;
    FRoundFormatDefinition ActiveFormatDefinition;
    if (!RoundGameInstance->TryGetFormatDefinition(ActiveFormat, ActiveFormatDefinition))
    {
        return Result;
    }

    // Start regions belong to the team anchors defined by the format. Character
    // formation offsets must not change their size or move their center.
    Result.Add(ActiveFormatDefinition.StartPosition);
    Result.AddUnique(ActiveFormatDefinition.StartPosition * -1.f);
    return Result;
}

bool IsWithinProtectedTerrainArea(const FVector2D& Position, const TArray<FVector2D>& ProtectedSpawnPositions, const int32 ProtectionRadius = 2)
{
    for (const FVector2D& ProtectedPosition : ProtectedSpawnPositions)
    {
        if (GetAxialHexDistance(Position, ProtectedPosition) <= ProtectionRadius)
        {
            return true;
        }
    }

    return false;
}

TArray<ARoundGridTile*> CollectGroundFields(const TArray<TObjectPtr<ARoundGridTile>>& Fields, const TArray<FVector2D>& ProtectedSpawnPositions)
{
    TArray<ARoundGridTile*> Result;
    Result.Reserve(Fields.Num());
    for (ARoundGridTile* Field : Fields)
    {
        if (Field
            && Field->TerrainType == ERoundTerrainType::Ground
            && !IsWithinProtectedTerrainArea(Field->GridPosition, ProtectedSpawnPositions))
        {
            Result.Add(Field);
        }
    }
    return Result;
}

ARoundGridTile* ChooseRandomField(ARoundGameState& GameState, const TArray<ARoundGridTile*>& Fields)
{
    if (Fields.Num() == 0)
    {
        return nullptr;
    }

    return Fields[GameState.GetRandomRange(0, Fields.Num() - 1)];
}

int32 ApplyCompactTerrainBlob(
    ARoundGameState& GameState,
    const ERoundTerrainType TerrainType,
    const int32 ClusterSize,
    const TArray<FVector2D>& ProtectedSpawnPositions)
{
    TArray<ARoundGridTile*> GroundFields = CollectGroundFields(GameState.Fields, ProtectedSpawnPositions);
    ARoundGridTile* SeedField = ChooseRandomField(GameState, GroundFields);
    if (!SeedField)
    {
        return 0;
    }

    TArray<ARoundGridTile*> Frontier;
    TSet<TObjectPtr<ARoundGridTile>> Seen;
    Frontier.Add(SeedField);
    Seen.Add(SeedField);

    int32 AppliedCount = 0;
    while (Frontier.Num() > 0 && AppliedCount < ClusterSize)
    {
        int32 BestDistance = MAX_int32;
        TArray<int32> CandidateIndices;
        for (int32 Index = 0; Index < Frontier.Num(); ++Index)
        {
            ARoundGridTile* Candidate = Frontier[Index];
            if (!Candidate || Candidate->TerrainType != ERoundTerrainType::Ground)
            {
                continue;
            }

            const int32 Distance = Candidate->GetHexDistanceTo(SeedField);
            if (Distance < BestDistance)
            {
                BestDistance = Distance;
                CandidateIndices.Reset();
                CandidateIndices.Add(Index);
            }
            else if (Distance == BestDistance)
            {
                CandidateIndices.Add(Index);
            }
        }

        if (CandidateIndices.Num() == 0)
        {
            break;
        }

        const int32 ChosenCandidateIndex = CandidateIndices[GameState.GetRandomRange(0, CandidateIndices.Num() - 1)];
        ARoundGridTile* CurrentField = Frontier[ChosenCandidateIndex];
        Frontier.RemoveAtSwap(ChosenCandidateIndex);
        if (!CurrentField || CurrentField->TerrainType != ERoundTerrainType::Ground)
        {
            continue;
        }

        GameState.ApplyTerrainTypeToField(CurrentField, TerrainType);
        ++AppliedCount;

        for (ARoundGridTile* Neighbor : GameState.GetNeighborFields(CurrentField))
        {
            if (!Neighbor
                || Seen.Contains(Neighbor)
                || Neighbor->TerrainType != ERoundTerrainType::Ground
                || IsWithinProtectedTerrainArea(Neighbor->GridPosition, ProtectedSpawnPositions))
            {
                continue;
            }

            Seen.Add(Neighbor);
            Frontier.Add(Neighbor);
        }
    }

    return AppliedCount;
}

int32 ApplyWildConnectedTerrain(
    ARoundGameState& GameState,
    const ERoundTerrainType TerrainType,
    const int32 ClusterSize,
    const TArray<FVector2D>& ProtectedSpawnPositions)
{
    TArray<ARoundGridTile*> GroundFields = CollectGroundFields(GameState.Fields, ProtectedSpawnPositions);
    ARoundGridTile* SeedField = ChooseRandomField(GameState, GroundFields);
    if (!SeedField)
    {
        return 0;
    }

    TArray<ARoundGridTile*> Frontier;
    TSet<TObjectPtr<ARoundGridTile>> Seen;
    Frontier.Add(SeedField);
    Seen.Add(SeedField);

    int32 AppliedCount = 0;
    while (Frontier.Num() > 0 && AppliedCount < ClusterSize)
    {
        const int32 RandomIndex = GameState.GetRandomRange(0, Frontier.Num() - 1);
        ARoundGridTile* CurrentField = Frontier[RandomIndex];
        Frontier.RemoveAtSwap(RandomIndex);
        if (!CurrentField || CurrentField->TerrainType != ERoundTerrainType::Ground)
        {
            continue;
        }

        GameState.ApplyTerrainTypeToField(CurrentField, TerrainType);
        ++AppliedCount;

        TArray<ARoundGridTile*> Neighbors = GameState.GetNeighborFields(CurrentField);
        for (int32 Index = Neighbors.Num() - 1; Index > 0; --Index)
        {
            const int32 SwapIndex = GameState.GetRandomRange(0, Index);
            Neighbors.Swap(Index, SwapIndex);
        }

        for (ARoundGridTile* Neighbor : Neighbors)
        {
            if (!Neighbor
                || Seen.Contains(Neighbor)
                || Neighbor->TerrainType != ERoundTerrainType::Ground
                || IsWithinProtectedTerrainArea(Neighbor->GridPosition, ProtectedSpawnPositions))
            {
                continue;
            }

            Seen.Add(Neighbor);
            Frontier.Add(Neighbor);
        }
    }

    return AppliedCount;
}

int32 ApplyStreetTerrainPath(
    ARoundGameState& GameState,
    const ERoundTerrainType TerrainType,
    const int32 ClusterSize,
    const TArray<FVector2D>& ProtectedSpawnPositions)
{
    TArray<ARoundGridTile*> GroundFields = CollectGroundFields(GameState.Fields, ProtectedSpawnPositions);
    ARoundGridTile* CurrentField = ChooseRandomField(GameState, GroundFields);
    if (!CurrentField)
    {
        return 0;
    }

    TSet<TObjectPtr<ARoundGridTile>> UsedFields;
    FVector2D PreferredDirection = FVector2D::ZeroVector;
    int32 AppliedCount = 0;

    while (CurrentField && AppliedCount < ClusterSize)
    {
        if (!UsedFields.Contains(CurrentField) && CurrentField->TerrainType == ERoundTerrainType::Ground)
        {
            GameState.ApplyTerrainTypeToField(CurrentField, TerrainType);
            UsedFields.Add(CurrentField);
            ++AppliedCount;
        }

        if (AppliedCount >= ClusterSize)
        {
            break;
        }

        TArray<ARoundGridTile*> NeighborCandidates;
        for (ARoundGridTile* Neighbor : GameState.GetNeighborFields(CurrentField))
        {
            if (Neighbor
                && Neighbor->TerrainType == ERoundTerrainType::Ground
                && !UsedFields.Contains(Neighbor)
                && !IsWithinProtectedTerrainArea(Neighbor->GridPosition, ProtectedSpawnPositions))
            {
                NeighborCandidates.Add(Neighbor);
            }
        }

        if (NeighborCandidates.Num() == 0)
        {
            break;
        }

        ARoundGridTile* NextField = nullptr;
        if (!PreferredDirection.IsNearlyZero() && GameState.GetRandomRange(0, 99) < 65)
        {
            float BestScore = -FLT_MAX;
            TArray<ARoundGridTile*> BestCandidates;
            for (ARoundGridTile* Candidate : NeighborCandidates)
            {
                const FVector2D CandidateDirection = Candidate->GridPosition - CurrentField->GridPosition;
                const float Score = FVector2D::DotProduct(CandidateDirection, PreferredDirection);
                if (Score > BestScore + KINDA_SMALL_NUMBER)
                {
                    BestScore = Score;
                    BestCandidates.Reset();
                    BestCandidates.Add(Candidate);
                }
                else if (FMath::IsNearlyEqual(Score, BestScore))
                {
                    BestCandidates.Add(Candidate);
                }
            }

            if (BestCandidates.Num() > 0)
            {
                NextField = BestCandidates[GameState.GetRandomRange(0, BestCandidates.Num() - 1)];
            }
        }

        if (!NextField)
        {
            NextField = NeighborCandidates[GameState.GetRandomRange(0, NeighborCandidates.Num() - 1)];
        }

        PreferredDirection = NextField->GridPosition - CurrentField->GridPosition;
        CurrentField = NextField;
    }

    return AppliedCount;
}

FCubeCoord CubeLerp(const FCubeCoord& A, const FCubeCoord& B, const double Alpha)
{
    return {
        FMath::Lerp(A.X, B.X, Alpha),
        FMath::Lerp(A.Y, B.Y, Alpha),
        FMath::Lerp(A.Z, B.Z, Alpha)
    };
}

FCubeCoord CubeRound(const FCubeCoord& Cube)
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

    return {
        static_cast<double>(RoundedX),
        static_cast<double>(RoundedY),
        static_cast<double>(RoundedZ)
    };
}

FVector2D CubeToAxial(const FCubeCoord& Cube)
{
    return FVector2D(static_cast<float>(Cube.X), static_cast<float>(Cube.Z));
}

FVector GetLegacyHexWorldLocation(const TSubclassOf<ARoundGridTile>& FieldClass, const FVector2D& GridPosition)
{
    const ARoundGridTile* DefaultTile = FieldClass ? FieldClass->GetDefaultObject<ARoundGridTile>() : nullptr;
    const float WorldX = GridPosition.X * 150.0f;
    const float WorldY = (GridPosition.Y * 170.0f) + (GridPosition.X * 85.0f);
    const float WorldZ = DefaultTile ? DefaultTile->GetActorLocation().Z : 0.0f;
    return FVector(WorldX, WorldY, WorldZ);
}

FString BuildIsoUtcTimestamp()
{
    return FDateTime::UtcNow().ToIso8601();
}

FString BuildMatchRecordId()
{
    return FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
}

void ApplyPresentationProfileDataAsset(const URoundPresentationProfileData* ProfileData, FRoundPresentationProfile& InOutProfile)
{
    if (!ProfileData)
    {
        return;
    }

    InOutProfile.TextColor = ProfileData->TextColor;
    InOutProfile.TextHeightOffset = ProfileData->TextHeightOffset;
    InOutProfile.BeamThicknessScale = ProfileData->BeamThicknessScale;
    InOutProfile.PulseStartScale = ProfileData->PulseStartScale;
    InOutProfile.PulseEndScale = ProfileData->PulseEndScale;
    InOutProfile.SoundVolumeMultiplier = ProfileData->SoundVolumeMultiplier;
    InOutProfile.SoundPitchMultiplier = ProfileData->SoundPitchMultiplier;
    InOutProfile.SoundMaxDurationSeconds = ProfileData->SoundMaxDurationSeconds;
    InOutProfile.ScalePunch = ProfileData->ScalePunch;
    InOutProfile.TextOverride = ProfileData->TextOverride;
    InOutProfile.SoundOverride = ProfileData->SoundOverride;
}
}

ARoundGameState::ARoundGameState()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    TeamActorClass = ARoundTeamActor::StaticClass();
}

ARoundGameState::~ARoundGameState() = default;

void ARoundGameState::BeginPlay()
{
    Super::BeginPlay();

    InitializePresentationProfiles();

    if (!HasAuthority())
    {
        return;
    }

    EnsureInitialFields();
    GenerateInitialTerrain();

    int32 LiveTeamActorCount = 0;
    if (GetWorld())
    {
        for (TActorIterator<ARoundTeamActor> It(GetWorld()); It; ++It)
        {
            if (*It)
            {
                ++LiveTeamActorCount;
            }
        }
    }

    if (bCreateTeamsOnBeginPlay
        && GetNetMode() == NM_Standalone
        && LiveTeamActorCount == 0
        && RegisteredCharacters.Num() == 0)
    {
        CreateTeams();
    }

    if (!CurrentTurnCharacter)
    {
        SetNextTurn();

        if (CurrentTurnCharacter && CurrentTurnCharacter->RemainInitiative() <= 0)
        {
            UE_LOG(LogTemp, Log, TEXT("BeginPlay initial turn had no usable initiative - advancing once more. Character=%s Initiative=%d"),
                *GetNameSafe(CurrentTurnCharacter),
                CurrentTurnCharacter->RemainInitiative());
            SetNextTurn();
        }
    }

    InitializeMatchRecording();
    ScheduleInitialVisionRefresh();
}

void ARoundGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundGameState, CurrentTurnCharacter);
    DOREPLIFETIME(ARoundGameState, MatchGlobalInitiativeElapsed);
    DOREPLIFETIME(ARoundGameState, MatchRandomSeed);
}

void ARoundGameState::OnRep_CurrentTurnCharacter()
{
    OnTurnChanged.Broadcast(CurrentTurnCharacter);
    OnBoardStateChanged.Broadcast();
}

void ARoundGameState::OnRep_MatchRandomSeed()
{
    bMatchRandomStreamInitialized = false;
    InitializeMatchRandomSeed();
}

void ARoundGameState::RegisterField(ARoundGridTile* Field)
{
    if (!Field)
    {
        return;
    }

    Fields.AddUnique(Field);
}

void ARoundGameState::RegisterCharacter(ARoundCharacter* Character)
{
    if (!Character)
    {
        return;
    }

    RegisteredCharacters.AddUnique(Character);
    RebuildTurnOrder();
}

TArray<ARoundGridTile*> ARoundGameState::GetFields() const
{
    TArray<ARoundGridTile*> Result;
    if (Fields.Num() > 0)
    {
        Result.Reserve(Fields.Num());
        for (ARoundGridTile* Field : Fields)
        {
            Result.Add(Field);
        }
        return Result;
    }

    if (GetWorld())
    {
        for (TActorIterator<ARoundGridTile> It(GetWorld()); It; ++It)
        {
            if (ARoundGridTile* Field = *It)
            {
                Result.Add(Field);
            }
        }
    }

    return Result;
}

ARoundGridTile* ARoundGameState::GetField(const FVector2D& Position) const
{
    const TArray<ARoundGridTile*> CandidateFields = GetFields();

    const auto FindByGridCoordinate = [&CandidateFields](const FIntPoint& Coordinate) -> ARoundGridTile*
    {
        for (ARoundGridTile* Field : CandidateFields)
        {
            if (Field && Field->GridCoordinate == Coordinate)
            {
                return Field;
            }
        }

        return nullptr;
    };

    const auto FindByGridPosition = [&CandidateFields](const FVector2D& CandidatePosition) -> ARoundGridTile*
    {
        for (ARoundGridTile* Field : CandidateFields)
        {
            if (Field && Field->MatchesGridPosition(CandidatePosition))
            {
                return Field;
            }
        }

        return nullptr;
    };

    if (ARoundGridTile* ExactField = FindByGridPosition(Position))
    {
        return ExactField;
    }

    const FIntPoint RoundedCoordinate(FMath::RoundToInt(Position.X), FMath::RoundToInt(Position.Y));
    if (ARoundGridTile* RoundedField = FindByGridCoordinate(RoundedCoordinate))
    {
        return RoundedField;
    }

    const FVector2D OffsetPosition = Position + InitialFieldStartPosition;
    if (!InitialFieldStartPosition.IsNearlyZero())
    {
        if (ARoundGridTile* OffsetField = FindByGridPosition(OffsetPosition))
        {
            return OffsetField;
        }

        const FIntPoint RoundedOffsetCoordinate(FMath::RoundToInt(OffsetPosition.X), FMath::RoundToInt(OffsetPosition.Y));
        if (ARoundGridTile* RoundedOffsetField = FindByGridCoordinate(RoundedOffsetCoordinate))
        {
            return RoundedOffsetField;
        }

        const FVector2D InverseOffsetPosition = Position - InitialFieldStartPosition;
        if (ARoundGridTile* InverseOffsetField = FindByGridPosition(InverseOffsetPosition))
        {
            return InverseOffsetField;
        }

        const FIntPoint RoundedInverseOffsetCoordinate(FMath::RoundToInt(InverseOffsetPosition.X), FMath::RoundToInt(InverseOffsetPosition.Y));
        if (ARoundGridTile* RoundedInverseOffsetField = FindByGridCoordinate(RoundedInverseOffsetCoordinate))
        {
            return RoundedInverseOffsetField;
        }
    }

    for (ARoundGridTile* Field : CandidateFields)
    {
        if (Field && Field->GridPosition.Equals(Position, 0.25f))
        {
            return Field;
        }
    }

    return nullptr;
}

ARoundGridTile* ARoundGameState::GetFieldAt(const FVector2D& Position) const
{
    return GetField(Position);
}

const URoundTerrainData* ARoundGameState::GetTerrainData(const ERoundTerrainType TerrainType) const
{
    if (TerrainRegistryData)
    {
        return TerrainRegistryData->GetTerrainData(TerrainType);
    }

    if (GameDataRegistry && GameDataRegistry->TerrainRegistry.ToSoftObjectPath().IsValid())
    {
        if (URoundTerrainRegistryData* Registry = GameDataRegistry->TerrainRegistry.LoadSynchronous())
        {
            return Registry->GetTerrainData(TerrainType);
        }
    }

    if (URoundTerrainRegistryData* Registry = RoundGameDataRegistryLookup::ResolveTerrainRegistry())
    {
        return Registry->GetTerrainData(TerrainType);
    }

    return nullptr;
}

TMap<ERoundTerrainType, float> ARoundGameState::BuildDefaultTerrainWeights() const
{
    TMap<ERoundTerrainType, float> Result;
    if (TerrainRegistryData)
    {
        for (const TPair<ERoundTerrainType, TObjectPtr<URoundTerrainData>>& Pair : TerrainRegistryData->TerrainDataByType)
        {
            if (Pair.Value && Pair.Value->DefaultWeight > 0.0f)
            {
                Result.Add(Pair.Key, Pair.Value->DefaultWeight);
            }
        }
    }

    if (Result.Num() == 0)
    {
        return BuildFallbackTerrainWeights();
    }

    return Result;
}

ARoundGridTile* ARoundGameState::GetCloseFreeField(ARoundGridTile* From) const
{
    if (!From)
    {
        return nullptr;
    }

    int32 BestDistance = MAX_int32;
    ARoundGridTile* BestField = nullptr;

    for (ARoundGridTile* Field : Fields)
    {
        if (!IsFieldFree(Field))
        {
            continue;
        }

        const int32 Distance = Field->GetHexDistanceTo(From);
        if (Distance <= 0)
        {
            continue;
        }

        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            BestField = Field;
        }
    }

    return BestField;
}

ARoundGridTile* ARoundGameState::GetCloseFreeFieldFrom(ARoundGridTile* Target, ARoundGridTile* From) const
{
    if (!Target)
    {
        return nullptr;
    }

    int32 ClosestToTargetDistance = MAX_int32;
    int32 ClosestToFromDistance = MAX_int32;
    ARoundGridTile* ClosestField = nullptr;

    for (ARoundGridTile* Field : Fields)
    {
        if (!IsFieldFree(Field))
        {
            continue;
        }

        const int32 DistanceToTarget = Field->GetHexDistanceTo(Target);
        if (DistanceToTarget <= 0)
        {
            continue;
        }

        const int32 DistanceToFrom = From ? Field->GetHexDistanceTo(From) : DistanceToTarget;

        const bool bBetterTargetDistance = DistanceToTarget < ClosestToTargetDistance;
        const bool bSameTargetDistanceButCloserToFrom =
            DistanceToTarget == ClosestToTargetDistance && DistanceToFrom < ClosestToFromDistance;

        if (bBetterTargetDistance || bSameTargetDistanceButCloserToFrom)
        {
            ClosestToTargetDistance = DistanceToTarget;
            ClosestToFromDistance = DistanceToFrom;
            ClosestField = Field;
        }
    }

    return ClosestField;
}

ARoundCharacter* ARoundGameState::SpawnCharacter(TSubclassOf<ARoundCharacter> CharacterClass, const FVector2D& Position, UObject* TeamObject, int64 Skills, int64 InCharacterPassiveSelections, const TArray<FRoundSkillEntry>& InAbilitySkillSelections, FName RowName)
{
    if (!GetWorld() || !CharacterClass)
    {
        return nullptr;
    }

    ARoundGridTile* TargetField = GetField(Position);
    UE_LOG(LogTemp, Log, TEXT("SpawnCharacter request - Row=%s Class=%s Position=%s Team=%s InitialFieldStart=%s ResolvedField=%s"),
        *RowName.ToString(),
        *GetNameSafe(CharacterClass),
        *Position.ToString(),
        *GetNameSafe(TeamObject),
        *InitialFieldStartPosition.ToString(),
        *GetNameSafe(TargetField));
    if (!TargetField)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnCharacter failed - no target field for Position=%s"), *Position.ToString());
        return nullptr;
    }

    UE_LOG(LogTemp, Log, TEXT("SpawnCharacter field resolved - Row=%s Field=%s Grid=%s Coord=(%d,%d) Loc=%s"),
        *RowName.ToString(),
        *GetNameSafe(TargetField),
        *TargetField->GridPosition.ToString(),
        TargetField->GridCoordinate.X,
        TargetField->GridCoordinate.Y,
        *TargetField->GetActorLocation().ToString());

    const FTransform SpawnTransform(FRotator::ZeroRotator, TargetField->GetActorLocation());
    ARoundCharacter* SpawnedCharacter = GetWorld()->SpawnActorDeferred<ARoundCharacter>(
        CharacterClass,
        SpawnTransform,
        nullptr,
        nullptr,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

    if (!SpawnedCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnCharacter failed - SpawnActor returned null for Class=%s Field=%s FieldLoc=%s"),
            *GetNameSafe(CharacterClass),
            *GetNameSafe(TargetField),
            *TargetField->GetActorLocation().ToString());
        return nullptr;
    }

    SpawnedCharacter->SetReplicates(true);
    SpawnedCharacter->bAlwaysRelevant = true;
    SpawnedCharacter->bOnlyRelevantToOwner = false;
    SpawnedCharacter->SetReplicateMovement(true);
    SpawnedCharacter->ForceNetUpdate();

      SpawnedCharacter->CharacterRowName = RowName;
      SpawnedCharacter->CharacterPassiveSelections = InCharacterPassiveSelections;
      SpawnedCharacter->AbilitySkillSelections = InAbilitySkillSelections;
      SpawnedCharacter->CurrentTile = TargetField;
      TargetField->OccupyingCharacter = SpawnedCharacter;

      const TSoftObjectPtr<URoundCharacterData> CharacterDataAsset = RoundCharacterRegistry::FindCharacterDataAssetById(RowName);
      if (CharacterDataAsset.ToSoftObjectPath().IsValid())
      {
          SpawnedCharacter->CharacterDataAsset = CharacterDataAsset;
      }
      else if (RowName == FName(TEXT("Character6Puppet")))
      {
          SpawnedCharacter->CharacterDataAsset = TSoftObjectPtr<URoundCharacterData>(
              FSoftObjectPath(TEXT("/Game/TopDown/Characters/Character_6/Puppet/Character6PuppetDetails.Character6PuppetDetails")));
      }

      if (ARoundTeamActor* TeamActor = Cast<ARoundTeamActor>(TeamObject))
      {
        SpawnedCharacter->SetOwningTeamActor(TeamActor);
    }

      TrySetObjectProperty(SpawnedCharacter, TEXT("Field"), TargetField);
      TrySetObjectProperty(SpawnedCharacter, TEXT("Team"), TeamObject);
      TrySetInt64Property(SpawnedCharacter, TEXT("Skills"), Skills);
      TrySetInt64Property(SpawnedCharacter, TEXT("CharacterPassiveSelections"), InCharacterPassiveSelections);
      if (const FArrayProperty* SkillSelectionsProperty = FindFProperty<FArrayProperty>(SpawnedCharacter->GetClass(), TEXT("AbilitySkillSelections")))
      {
          FScriptArrayHelper Helper(SkillSelectionsProperty, SkillSelectionsProperty->ContainerPtrToValuePtr<void>(SpawnedCharacter));
          Helper.Resize(InAbilitySkillSelections.Num());
          for (int32 Index = 0; Index < InAbilitySkillSelections.Num(); ++Index)
          {
              const FRoundSkillEntry& SourceEntry = InAbilitySkillSelections[Index];
              *reinterpret_cast<FRoundSkillEntry*>(Helper.GetRawPtr(Index)) = SourceEntry;
          }
      }
      TrySetNameProperty(SpawnedCharacter, TEXT("RowName"), RowName);
      UGameplayStatics::FinishSpawningActor(SpawnedCharacter, SpawnTransform);
      SpawnedCharacter->InitializeCharacter();
      if (ARoundCharacter6* Character6 = Cast<ARoundCharacter6>(SpawnedCharacter))
      {
          Character6->EnsurePuppetCharacter();
      }

    RegisterCharacter(SpawnedCharacter);
    RecordCharacterSpawned(SpawnedCharacter);
    SpawnedCharacter->ForceNetUpdate();
    OnBoardStateChanged.Broadcast();
    return SpawnedCharacter;
}

ARoundCharacter* ARoundGameState::SpawnCharacterByRowName(const FName RowName, const FVector2D& Position, UObject* TeamObject, const int64 Skills, const int64 InCharacterPassiveSelections, const TArray<FRoundSkillEntry>& InAbilitySkillSelections)
{
    if (const TSubclassOf<ARoundCharacter>* CharacterClass = CharacterRowClasses.Find(RowName))
    {
        return SpawnCharacter(*CharacterClass, Position, TeamObject, Skills, InCharacterPassiveSelections, InAbilitySkillSelections, RowName);
    }

    const FString RequestedName = NormalizeCharacterKey(RowName.ToString());
    for (const TPair<FName, TSubclassOf<ARoundCharacter>>& Pair : CharacterRowClasses)
    {
        if (NormalizeCharacterKey(Pair.Key.ToString()) == RequestedName)
        {
            return SpawnCharacter(Pair.Value, Position, TeamObject, Skills, InCharacterPassiveSelections, InAbilitySkillSelections, Pair.Key);
        }
    }

    if (const TSubclassOf<ARoundCharacter> RegistryCharacterClass = RoundCharacterRegistry::FindCharacterClassById(RowName))
    {
        UE_LOG(LogTemp, Log, TEXT("SpawnCharacterByRowName resolved via CharacterRegistry - Row=%s RequestedKey=%s Class=%s"),
            *RowName.ToString(),
            *RequestedName,
            *GetNameSafe(RegistryCharacterClass));
        return SpawnCharacter(RegistryCharacterClass, Position, TeamObject, Skills, InCharacterPassiveSelections, InAbilitySkillSelections, RowName);
    }

    UE_LOG(LogTemp, Warning, TEXT("SpawnCharacterByRowName failed - Row=%s RequestedKey=%s AvailableClasses=%d RegistryClass=None"),
        *RowName.ToString(),
        *RequestedName,
        CharacterRowClasses.Num());
    return nullptr;
}

AActor* ARoundGameState::SpawnTrap(TSubclassOf<AActor> TrapClass, int32 Duration, ARoundGridTile* Field, ARoundCharacter* OwnerCharacter)
{
    if (!GetWorld() || !TrapClass || !Field)
    {
        return nullptr;
    }

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Field->GetActorLocation());

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = OwnerCharacter;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParameters.Instigator = OwnerCharacter;

    AActor* TrapActor = GetWorld()->SpawnActor<AActor>(TrapClass, SpawnTransform, SpawnParameters);
    if (!TrapActor)
    {
        return nullptr;
    }

    TrySetIntProperty(TrapActor, TEXT("Duration"), Duration);
    TrySetObjectProperty(TrapActor, TEXT("Field"), Field);
    TrySetObjectProperty(TrapActor, TEXT("Character"), OwnerCharacter);
    return TrapActor;
}

bool ARoundGameState::TryCreateTeamsFromPlayerSelections()
{
    if (!HasAuthority() || !bCreateTeamsOnBeginPlay || Teams.Num() > 0 || RegisteredCharacters.Num() > 0)
    {
        return Teams.Num() > 0;
    }

    ARoundPlayerPawn* PlayerOnePawn = Cast<ARoundPlayerPawn>(Player1);
    ARoundPlayerPawn* PlayerTwoPawn = Cast<ARoundPlayerPawn>(Player2);
    if (!PlayerOnePawn || !PlayerTwoPawn
        || PlayerOnePawn->TeamDefinition.Characters.IsEmpty()
        || PlayerTwoPawn->TeamDefinition.Characters.IsEmpty())
    {
        return false;
    }

    CreateTeams();
    if (Teams.IsValidIndex(0))
    {
        PlayerOnePawn->SetTeamActor(Cast<ARoundTeamActor>(Teams[0]));
    }
    if (Teams.IsValidIndex(1))
    {
        PlayerTwoPawn->SetTeamActor(Cast<ARoundTeamActor>(Teams[1]));
    }

    RebuildTurnOrder();
    if (!CurrentTurnCharacter)
    {
        SetNextTurn();
    }
    SetVision();
    return Teams.Num() >= 2;
}

void ARoundGameState::CreateTeams()
{
    Teams.Empty();
    UE_LOG(LogTemp, Log, TEXT("CreateTeams begin - ExistingLoadedTeams=%d Player1=%s Player2=%s"),
        GetGameInstance<URoundGameInstance>() ? GetGameInstance<URoundGameInstance>()->LoadedTeams.Num() : -1,
        *GetNameSafe(Player1),
        *GetNameSafe(Player2));

    if (!GetWorld() || !TeamActorClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateTeams skipped - World=%s TeamActorClass=%s"), GetWorld() ? TEXT("Yes") : TEXT("No"), *GetNameSafe(TeamActorClass));
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    const URoundGameInstance* RoundGameInstance = GetGameInstance<URoundGameInstance>();
    const FRoundTeamDefinition* TeamOneDefinition = nullptr;
    const FRoundTeamDefinition* TeamTwoDefinition = nullptr;

    if (RoundGameInstance)
    {
        if (RoundGameInstance->LoadedTeams.IsValidIndex(0))
        {
            TeamOneDefinition = &RoundGameInstance->LoadedTeams[0];
        }
        else
        {
            TeamOneDefinition = &RoundGameInstance->StrTeam;
        }

        if (RoundGameInstance->LoadedTeams.IsValidIndex(1))
        {
            TeamTwoDefinition = &RoundGameInstance->LoadedTeams[1];
        }
    }

    ARoundPlayerPawn* PlayerOnePawn = Cast<ARoundPlayerPawn>(Player1);
    ARoundPlayerPawn* PlayerTwoPawn = Cast<ARoundPlayerPawn>(Player2);
    if (PlayerOnePawn && !PlayerOnePawn->TeamDefinition.Characters.IsEmpty())
    {
        TeamOneDefinition = &PlayerOnePawn->TeamDefinition;
    }
    if (PlayerTwoPawn && !PlayerTwoPawn->TeamDefinition.Characters.IsEmpty())
    {
        TeamTwoDefinition = &PlayerTwoPawn->TeamDefinition;
    }

    UE_LOG(LogTemp, Log, TEXT("CreateTeams definitions - TeamOne=%s Characters=%d TeamTwo=%s Characters=%d"),
        TeamOneDefinition ? *TeamOneDefinition->Name : TEXT("None"),
        TeamOneDefinition ? TeamOneDefinition->Characters.Num() : -1,
        TeamTwoDefinition ? *TeamTwoDefinition->Name : TEXT("None"),
        TeamTwoDefinition ? TeamTwoDefinition->Characters.Num() : -1);

    // The selected definitions can live on the player pawns. SetTeamActor copies the
    // (initially empty) definition from a freshly spawned team actor back to its pawn,
    // so references into the pawn must not be kept across that call.
    const TOptional<FRoundTeamDefinition> TeamOneDefinitionCopy = TeamOneDefinition
        ? TOptional<FRoundTeamDefinition>(*TeamOneDefinition)
        : TOptional<FRoundTeamDefinition>();
    const TOptional<FRoundTeamDefinition> TeamTwoDefinitionCopy = TeamTwoDefinition
        ? TOptional<FRoundTeamDefinition>(*TeamTwoDefinition)
        : TOptional<FRoundTeamDefinition>();

    FVector2D TeamOneBoardPosition = FVector2D::ZeroVector;
    FVector2D TeamTwoBoardPosition = FVector2D::ZeroVector;

    ERoundFormat ActiveFormat = ERoundFormat::Format3x16;
    if (TeamOneDefinition)
    {
        ActiveFormat = TeamOneDefinition->Format;
    }
    else if (RoundGameInstance)
    {
        ActiveFormat = RoundGameInstance->StrTeam.Format;
    }

    FRoundFormatDefinition ActiveFormatDefinition;
    if (RoundGameInstance && RoundGameInstance->TryGetFormatDefinition(ActiveFormat, ActiveFormatDefinition))
    {
        const FVector2D TeamOneAnchorPosition = ActiveFormatDefinition.StartPosition;
        const FVector2D TeamTwoAnchorPosition = ActiveFormatDefinition.StartPosition * -1.f;
        UE_LOG(LogTemp, Log, TEXT("CreateTeams format - FormatEnum=%d Name=%s StartPosition=%s FieldSize=%d Terrain=%d Characters=%d InitialFieldStart=%s"),
            static_cast<int32>(ActiveFormat),
            *ActiveFormatDefinition.Name,
            *ActiveFormatDefinition.StartPosition.ToString(),
            ActiveFormatDefinition.FieldSize,
            ActiveFormatDefinition.Terrain,
            ActiveFormatDefinition.Characters,
            *InitialFieldStartPosition.ToString());
        UE_LOG(LogTemp, Log, TEXT("CreateTeams anchors - TeamOneRequested=%s TeamTwoRequested=%s"),
            *TeamOneAnchorPosition.ToString(),
            *TeamTwoAnchorPosition.ToString());

        if (ARoundGridTile* TeamOneAnchorField = GetField(TeamOneAnchorPosition))
        {
            TeamOneBoardPosition = TeamOneAnchorField->GridPosition;
            UE_LOG(LogTemp, Log, TEXT("CreateTeams TeamOne anchor resolved - Field=%s Grid=%s Coord=(%d,%d)"),
                *GetNameSafe(TeamOneAnchorField),
                *TeamOneAnchorField->GridPosition.ToString(),
                TeamOneAnchorField->GridCoordinate.X,
                TeamOneAnchorField->GridCoordinate.Y);
        }
        else
        {
            TeamOneBoardPosition = TeamOneAnchorPosition;
            UE_LOG(LogTemp, Warning, TEXT("CreateTeams TeamOne anchor unresolved - using raw position %s"), *TeamOneBoardPosition.ToString());
        }

        if (ARoundGridTile* TeamTwoAnchorField = GetField(TeamTwoAnchorPosition))
        {
            TeamTwoBoardPosition = TeamTwoAnchorField->GridPosition;
            UE_LOG(LogTemp, Log, TEXT("CreateTeams TeamTwo anchor resolved - Field=%s Grid=%s Coord=(%d,%d)"),
                *GetNameSafe(TeamTwoAnchorField),
                *TeamTwoAnchorField->GridPosition.ToString(),
                TeamTwoAnchorField->GridCoordinate.X,
                TeamTwoAnchorField->GridCoordinate.Y);
        }
        else
        {
            TeamTwoBoardPosition = TeamTwoAnchorPosition;
            UE_LOG(LogTemp, Warning, TEXT("CreateTeams TeamTwo anchor unresolved - using raw position %s"), *TeamTwoBoardPosition.ToString());
        }

        UE_LOG(LogTemp, Log, TEXT("CreateTeams final board positions - TeamOne=%s TeamTwo=%s"),
            *TeamOneBoardPosition.ToString(),
            *TeamTwoBoardPosition.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateTeams format lookup failed - RoundGameInstance=%s ActiveFormat=%d"),
            RoundGameInstance ? TEXT("Yes") : TEXT("No"),
            static_cast<int32>(ActiveFormat));
    }

    if (AActor* TeamOne = GetWorld()->SpawnActor<AActor>(TeamActorClass, TeamOneSpawnTransform, SpawnParameters))
    {
        if (ARoundTeamActor* RoundTeam = Cast<ARoundTeamActor>(TeamOne))
        {
            RoundTeam->TeamId = ERoundTeam::TeamA;
            if (TeamOneDefinitionCopy.IsSet())
            {
                RoundTeam->SpawnCharacters(TeamOneDefinitionCopy.GetValue(), TeamOneBoardPosition, FVector2D(1.f, 1.f));
            }

            if (ARoundPlayerPawn* PlayerPawn = Cast<ARoundPlayerPawn>(Player1))
            {
                PlayerPawn->SetTeamActor(RoundTeam);
            }
        }
        Teams.Add(TeamOne);
    }

    if (AActor* TeamTwo = GetWorld()->SpawnActor<AActor>(TeamActorClass, TeamTwoSpawnTransform, SpawnParameters))
    {
        if (ARoundTeamActor* RoundTeam = Cast<ARoundTeamActor>(TeamTwo))
        {
            RoundTeam->TeamId = ERoundTeam::TeamB;
            if (TeamTwoDefinitionCopy.IsSet())
            {
                RoundTeam->SpawnCharacters(TeamTwoDefinitionCopy.GetValue(), TeamTwoBoardPosition, FVector2D(-1.f, -1.f));
            }

            if (ARoundPlayerPawn* PlayerPawn = Cast<ARoundPlayerPawn>(Player2))
            {
                PlayerPawn->SetTeamActor(RoundTeam);
            }
        }
        Teams.Add(TeamTwo);
    }

    SetVision();
    SetNextTurn();
}

void ARoundGameState::CreateFieldFromParameters(const int32 FieldSize, const FVector2D StartPosition, const int32 SkillAmount, const int32 TerrainAmount, const TMap<ERoundTerrainType, float>& TerrainWeights)
{
    if (!HasAuthority() || FieldSize <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateFieldFromParameters skipped - HasAuthority=%d FieldSize=%d"), HasAuthority() ? 1 : 0, FieldSize);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("CreateFieldFromParameters - FieldSize=%d SkillAmount=%d TerrainAmount=%d TerrainWeights=%d Start=%s"),
        FieldSize,
        SkillAmount,
        TerrainAmount,
        TerrainWeights.Num(),
        *StartPosition.ToString());

    TMap<ERoundTerrainType, float> EffectiveTerrainWeights = TerrainWeights;
    if (EffectiveTerrainWeights.Num() == 0 && TerrainAmount > 0)
    {
        EffectiveTerrainWeights = BuildDefaultTerrainWeights();
        UE_LOG(LogTemp, Log, TEXT("CreateFieldFromParameters - no TerrainWeights supplied, using native defaults"));
    }

    InitialFieldSize = FieldSize;
    // The format StartPosition is a team-anchor concept, not the center of the
    // generated board. The old BP board was generated around the origin and the
    // start offset was only used later for team spawn anchors.
    InitialFieldStartPosition = FVector2D::ZeroVector;
    InitialSkillAmount = SkillAmount;
    InitialTerrainAmount = TerrainAmount;
    InitialTerrainWeights = EffectiveTerrainWeights;

    ResetBoardFields();

    const TArray<FVector2D> Positions = BuildHexSpiralPositions(FVector2D::ZeroVector, FieldSize);
    for (const FVector2D& Position : Positions)
    {
        SpawnFieldAt(Position, ERoundTerrainType::Ground);
    }

    if (!GenerateTerrainRegions())
    {
        TerrainTypes.Reset();
        const int32 EffectiveTerrainAmount = FMath::Max(0, TerrainAmount);
        TerrainTypes.Reserve(EffectiveTerrainAmount);
        for (int32 Index = 0; Index < EffectiveTerrainAmount; ++Index)
        {
            TerrainTypes.Add(GetWeightedTerrainType(EffectiveTerrainWeights));
        }

        TerrainTypes = OrderTerrainTypes(TerrainTypes);

        for (const ERoundTerrainType TerrainType : TerrainTypes)
        {
            CreateTerrain(TerrainType);
        }
    }

    bFinishedGenerating = true;
    OnBoardStateChanged.Broadcast();
}

bool ARoundGameState::GenerateTerrainRegions()
{
    URoundTerrainRegionRegistryData* Registry = TerrainRegionRegistryData;
    if (!Registry)
    {
        Registry = RoundGameDataRegistryLookup::ResolveTerrainRegionRegistry();
        TerrainRegionRegistryData = Registry;
    }
    if (!Registry || Fields.Num() == 0)
    {
        return false;
    }

    FString ValidationErrors;
    if (!Registry->Validate(ValidationErrors))
    {
        UE_LOG(LogTemp, Error, TEXT("Terrain region registry is invalid:\n%s"), *ValidationErrors);
        return false;
    }

    URoundTerrainRegionData* StartRegion = Registry->GetRegion(Registry->StartRegionId);
    if (!StartRegion)
    {
        return false;
    }

    const TArray<FVector2D> StartRegionCenters = BuildTerrainStartRegionCenters(*this);
    TSet<TObjectPtr<ARoundGridTile>> StartFields;
    TArray<ARoundGridTile*> SeedCandidates;
    for (ARoundGridTile* Field : Fields)
    {
        if (!Field) continue;
        if (IsWithinProtectedTerrainArea(Field->GridPosition, StartRegionCenters, Registry->StartRegionRadius))
        {
            StartFields.Add(Field);
        }
        else
        {
            SeedCandidates.Add(Field);
        }
    }

    TArray<URoundTerrainRegionData*> RegionChoices;
    for (const TPair<FName, TObjectPtr<URoundTerrainRegionData>>& Pair : Registry->Regions)
    {
        if (Pair.Value && Pair.Value->bCanGenerate && Pair.Key != Registry->StartRegionId)
        {
            RegionChoices.Add(Pair.Value);
        }
    }
    RegionChoices.Sort([](const URoundTerrainRegionData& A, const URoundTerrainRegionData& B)
    {
        return A.RegionId.LexicalLess(B.RegionId);
    });
    if (RegionChoices.Num() == 0) return false;

    TArray<TArray<ARoundGridTile*>> RegionFields;
    TArray<FName> AssignedTypes;
    TMap<TObjectPtr<ARoundGridTile>, int32> FieldRegion;
    TSet<TObjectPtr<ARoundGridTile>> UnassignedFields;
    for (ARoundGridTile* Field : SeedCandidates) UnassignedFields.Add(Field);

    TMap<TObjectPtr<ARoundGridTile>, int32> BoardEdgeDepth;
    TArray<ARoundGridTile*> EdgeQueue;
    for (ARoundGridTile* Field : Fields)
    {
        if (Field && GetNeighborFields(Field).Num() < 6)
        {
            BoardEdgeDepth.Add(Field, 0);
            EdgeQueue.Add(Field);
        }
    }
    for (int32 QueueIndex = 0; QueueIndex < EdgeQueue.Num(); ++QueueIndex)
    {
        ARoundGridTile* Current = EdgeQueue[QueueIndex];
        const int32 NextDepth = BoardEdgeDepth.FindRef(Current) + 1;
        for (ARoundGridTile* Neighbor : GetNeighborFields(Current))
        {
            if (Neighbor && !BoardEdgeDepth.Contains(Neighbor))
            {
                BoardEdgeDepth.Add(Neighbor, NextDepth);
                EdgeQueue.Add(Neighbor);
            }
        }
    }
    TMap<FName, int32> RegionOccurrenceCounts;

    const TArray<FVector2D>& NeighborOffsets = GetHexNeighborOffsets();
    while (UnassignedFields.Num() > 0)
    {
        TArray<ARoundGridTile*> FrontierSeeds;
        for (ARoundGridTile* Field : UnassignedFields)
        {
            bool bTouchesAssignedArea = false;
            for (ARoundGridTile* Neighbor : GetNeighborFields(Field))
            {
                if (StartFields.Contains(Neighbor) || FieldRegion.Contains(Neighbor))
                {
                    bTouchesAssignedArea = true;
                    break;
                }
            }
            if (bTouchesAssignedArea) FrontierSeeds.Add(Field);
        }
        if (FrontierSeeds.Num() == 0)
        {
            for (ARoundGridTile* Field : UnassignedFields) FrontierSeeds.Add(Field);
        }
        ARoundGridTile* Seed = FrontierSeeds[GetRandomRange(0, FrontierSeeds.Num() - 1)];

        TSet<FName> AdjacentTypes;
        for (ARoundGridTile* Neighbor : GetNeighborFields(Seed))
        {
            if (StartFields.Contains(Neighbor))
            {
                AdjacentTypes.Add(Registry->StartRegionId);
            }
            else if (const int32* NeighborRegion = FieldRegion.Find(Neighbor); NeighborRegion && AssignedTypes.IsValidIndex(*NeighborRegion))
            {
                AdjacentTypes.Add(AssignedTypes[*NeighborRegion]);
            }
        }

        TArray<double> Weights;
        double TotalWeight = 0.0;
        for (URoundTerrainRegionData* Candidate : RegionChoices)
        {
            double Weight = FMath::Max(0.0, Candidate->BaseWeight);
            if (Candidate->MaximumOccurrences > 0 && RegionOccurrenceCounts.FindRef(Candidate->RegionId) >= Candidate->MaximumOccurrences)
            {
                Weight = 0.0;
            }
            const int32 SeedEdgeDepth = BoardEdgeDepth.FindRef(Seed);
            if ((Candidate->Placement == ERoundTerrainRegionPlacement::EdgeOnly && SeedEdgeDepth > Candidate->EdgeDistance)
                || (Candidate->Placement == ERoundTerrainRegionPlacement::InteriorOnly && SeedEdgeDepth < Candidate->EdgeDistance))
            {
                Weight = 0.0;
            }
            for (const FName NeighborType : AdjacentTypes)
            {
                Weight *= Registry->GetAdjacencyWeight(Candidate->RegionId, NeighborType);
            }
            Weights.Add(Weight);
            TotalWeight += Weight;
        }

        bool bHasRequiredCandidate = false;
        for (int32 ChoiceIndex = 0; ChoiceIndex < RegionChoices.Num(); ++ChoiceIndex)
        {
            const URoundTerrainRegionData* Candidate = RegionChoices[ChoiceIndex];
            if (Weights[ChoiceIndex] > 0.0 && RegionOccurrenceCounts.FindRef(Candidate->RegionId) < Candidate->MinimumOccurrences)
            {
                bHasRequiredCandidate = true;
            }
        }
        if (bHasRequiredCandidate)
        {
            TotalWeight = 0.0;
            for (int32 ChoiceIndex = 0; ChoiceIndex < RegionChoices.Num(); ++ChoiceIndex)
            {
                const URoundTerrainRegionData* Candidate = RegionChoices[ChoiceIndex];
                if (RegionOccurrenceCounts.FindRef(Candidate->RegionId) >= Candidate->MinimumOccurrences) Weights[ChoiceIndex] = 0.0;
                TotalWeight += Weights[ChoiceIndex];
            }
        }

        if (TotalWeight <= UE_SMALL_NUMBER)
        {
            UE_LOG(LogTemp, Warning, TEXT("No fully compatible terrain type at %s; falling back to base region weights."), *Seed->GridPosition.ToString());
            TotalWeight = 0.0;
            for (int32 ChoiceIndex = 0; ChoiceIndex < RegionChoices.Num(); ++ChoiceIndex)
            {
                const URoundTerrainRegionData* Candidate = RegionChoices[ChoiceIndex];
                const int32 SeedEdgeDepth = BoardEdgeDepth.FindRef(Seed);
                const bool bAtMaximum = Candidate->MaximumOccurrences > 0
                    && RegionOccurrenceCounts.FindRef(Candidate->RegionId) >= Candidate->MaximumOccurrences;
                const bool bWrongPlacement =
                    (Candidate->Placement == ERoundTerrainRegionPlacement::EdgeOnly && SeedEdgeDepth > Candidate->EdgeDistance)
                    || (Candidate->Placement == ERoundTerrainRegionPlacement::InteriorOnly && SeedEdgeDepth < Candidate->EdgeDistance);
                Weights[ChoiceIndex] = (bAtMaximum || bWrongPlacement)
                    ? 0.0
                    : FMath::Max(0.0, Candidate->BaseWeight);
                TotalWeight += Weights[ChoiceIndex];
            }
            if (TotalWeight <= UE_SMALL_NUMBER)
            {
                UE_LOG(LogTemp, Error, TEXT("No terrain region remains valid at %s after enforcing placement and occurrence limits."), *Seed->GridPosition.ToString());
                return false;
            }
        }

        const double Roll = (static_cast<double>(GetRandomRange(0, 1000000)) / 1000000.0) * TotalWeight;
        double Accumulated = 0.0;
        int32 SelectedIndex = Weights.Num() - 1;
        for (int32 ChoiceIndex = 0; ChoiceIndex < Weights.Num(); ++ChoiceIndex)
        {
            Accumulated += Weights[ChoiceIndex];
            if (Roll <= Accumulated)
            {
                SelectedIndex = ChoiceIndex;
                break;
            }
        }
        URoundTerrainRegionData* SelectedRegion = RegionChoices[SelectedIndex];
        const int32 RegionIndex = RegionFields.Num();
        const int32 TargetSize = GetRandomRange(
            FMath::Max(1, SelectedRegion->RegionSizeMin),
            FMath::Max(SelectedRegion->RegionSizeMin, SelectedRegion->RegionSizeMax));

        const FIntPoint SeedCoordinate(FMath::RoundToInt(Seed->GridPosition.X), FMath::RoundToInt(Seed->GridPosition.Y));
        TSet<FIntPoint> Shape;
        TArray<FIntPoint> ShapeFrontier{SeedCoordinate};
        Shape.Add(SeedCoordinate);
        while (Shape.Num() < TargetSize && ShapeFrontier.Num() > 0)
        {
            const int32 FrontierIndex = GetRandomRange(0, ShapeFrontier.Num() - 1);
            const FIntPoint Current = ShapeFrontier[FrontierIndex];
            TArray<FIntPoint> OnMapCandidates;
            TArray<FIntPoint> OffMapCandidates;
            for (const FVector2D& Offset : NeighborOffsets)
            {
                const FIntPoint Candidate(Current.X + FMath::RoundToInt(Offset.X), Current.Y + FMath::RoundToInt(Offset.Y));
                if (Shape.Contains(Candidate)) continue;
                ARoundGridTile* CandidateField = GetField(FVector2D(Candidate.X, Candidate.Y));
                if (CandidateField)
                {
                    if (UnassignedFields.Contains(CandidateField)) OnMapCandidates.Add(Candidate);
                }
                else
                {
                    OffMapCandidates.Add(Candidate);
                }
            }

            if (OnMapCandidates.Num() == 0 && OffMapCandidates.Num() == 0)
            {
                ShapeFrontier.RemoveAtSwap(FrontierIndex);
                continue;
            }
            const bool bChooseOffMap = OffMapCandidates.Num() > 0
                && (OnMapCandidates.Num() == 0 || GetRandomFraction(0.0f, 1.0f) < Registry->OffMapGrowthWeight);
            const TArray<FIntPoint>& Candidates = bChooseOffMap ? OffMapCandidates : OnMapCandidates;
            const FIntPoint Chosen = Candidates[GetRandomRange(0, Candidates.Num() - 1)];
            Shape.Add(Chosen);
            ShapeFrontier.Add(Chosen);
        }

        TArray<ARoundGridTile*> NewRegionFields;
        for (const FIntPoint Coordinate : Shape)
        {
            if (ARoundGridTile* Field = GetField(FVector2D(Coordinate.X, Coordinate.Y)); Field && UnassignedFields.Contains(Field))
            {
                NewRegionFields.Add(Field);
            }
        }
        if (NewRegionFields.Num() == 0) NewRegionFields.Add(Seed);

        const int32 MinimumUsefulBoardSize = FMath::Max(1, SelectedRegion->RegionSizeMin / 4);
        if (NewRegionFields.Num() < MinimumUsefulBoardSize && RegionFields.Num() > 0)
        {
            TMap<int32, int32> SharedBorders;
            for (ARoundGridTile* Field : NewRegionFields)
            {
                for (ARoundGridTile* Neighbor : GetNeighborFields(Field))
                {
                    if (const int32* ExistingRegion = FieldRegion.Find(Neighbor))
                    {
                        SharedBorders.FindOrAdd(*ExistingRegion) += 1;
                    }
                }
            }

            int32 MergeRegion = INDEX_NONE;
            int32 BestSharedBorder = -1;
            for (const TPair<int32, int32>& Pair : SharedBorders)
            {
                if (Pair.Value > BestSharedBorder)
                {
                    BestSharedBorder = Pair.Value;
                    MergeRegion = Pair.Key;
                }
            }
            if (RegionFields.IsValidIndex(MergeRegion))
            {
                for (ARoundGridTile* Field : NewRegionFields)
                {
                    UnassignedFields.Remove(Field);
                    FieldRegion.Add(Field, MergeRegion);
                    RegionFields[MergeRegion].Add(Field);
                }
                UE_LOG(LogTemp, Log, TEXT("Merged clipped terrain remainder Type=%s BoardFields=%d IntoNode=%d Remaining=%d"),
                    *SelectedRegion->RegionId.ToString(), NewRegionFields.Num(), MergeRegion, UnassignedFields.Num());
                continue;
            }
        }

        for (ARoundGridTile* Field : NewRegionFields)
        {
            UnassignedFields.Remove(Field);
            FieldRegion.Add(Field, RegionIndex);
        }
        RegionFields.Add(MoveTemp(NewRegionFields));
        AssignedTypes.Add(SelectedRegion->RegionId);
        RegionOccurrenceCounts.FindOrAdd(SelectedRegion->RegionId) += 1;
        UE_LOG(LogTemp, Log, TEXT("Placed terrain region Node=%d Type=%s TargetSize=%d BoardFields=%d Remaining=%d"),
            RegionIndex, *SelectedRegion->RegionId.ToString(), TargetSize, RegionFields[RegionIndex].Num(), UnassignedFields.Num());
    }

    auto ApplyLayers = [this, &BoardEdgeDepth, Registry](const TArray<ARoundGridTile*>& Tiles, const URoundTerrainRegionData* RegionData, const int32 RegionIndex)
    {
        if (!RegionData || Tiles.Num() == 0) return;
        TSet<TObjectPtr<ARoundGridTile>> RegionSet;
        TSet<TObjectPtr<ARoundGridTile>> ClaimedByLayer;
        for (ARoundGridTile* Tile : Tiles)
        {
            RegionSet.Add(Tile);
        }

        TMap<FName, TArray<TObjectPtr<ARoundGridTile>>> LayerClaims;
        TMap<TObjectPtr<ARoundGridTile>, int32> RegionEdgeDepth;
        TArray<ARoundGridTile*> RegionEdgeQueue;
        for (ARoundGridTile* Tile : Tiles)
        {
            bool bBoundary = false;
            for (ARoundGridTile* Neighbor : GetNeighborFields(Tile)) if (!RegionSet.Contains(Neighbor)) { bBoundary = true; break; }
            if (bBoundary) { RegionEdgeDepth.Add(Tile, 0); RegionEdgeQueue.Add(Tile); }
        }
        for (int32 QueueIndex = 0; QueueIndex < RegionEdgeQueue.Num(); ++QueueIndex)
        {
            ARoundGridTile* Current = RegionEdgeQueue[QueueIndex];
            for (ARoundGridTile* Neighbor : GetNeighborFields(Current))
            {
                if (RegionSet.Contains(Neighbor) && !RegionEdgeDepth.Contains(Neighbor))
                {
                    RegionEdgeDepth.Add(Neighbor, RegionEdgeDepth.FindRef(Current) + 1);
                    RegionEdgeQueue.Add(Neighbor);
                }
            }
        }

        auto ApplyChosen = [this, &ClaimedByLayer, &LayerClaims](const FRoundTerrainRegionLayer& Layer, const TArray<ARoundGridTile*>& Chosen)
        {
            TArray<TObjectPtr<ARoundGridTile>>& Claims = LayerClaims.FindOrAdd(Layer.LayerId);
            for (ARoundGridTile* Tile : Chosen)
            {
                if (!Tile) continue;
                ApplyTerrainTypeToField(Tile, Layer.TerrainType);
                ClaimedByLayer.Add(Tile);
                Claims.AddUnique(Tile);
            }
        };

        if (RegionData->Structures.Num() > 0)
        {
            double TotalStructureWeight = 0.0;
            for (const FRoundTerrainRegionStructure& Candidate : RegionData->Structures)
            {
                TotalStructureWeight += FMath::Max(0.0, Candidate.Weight);
            }

            const FRoundTerrainRegionStructure* Structure = nullptr;
            double Roll = MatchRandomStream.FRandRange(0.0, TotalStructureWeight);
            for (const FRoundTerrainRegionStructure& Candidate : RegionData->Structures)
            {
                Roll -= FMath::Max(0.0, Candidate.Weight);
                if (Roll <= 0.0) { Structure = &Candidate; break; }
            }
            if (!Structure) Structure = &RegionData->Structures.Last();

            FVector2D AveragePosition = FVector2D::ZeroVector;
            for (ARoundGridTile* Tile : Tiles) AveragePosition += Tile->GridPosition;
            AveragePosition /= static_cast<double>(Tiles.Num());
            ARoundGridTile* CentreTile = Tiles[0];
            for (ARoundGridTile* Tile : Tiles)
            {
                if (FVector2D::DistSquared(Tile->GridPosition, AveragePosition) < FVector2D::DistSquared(CentreTile->GridPosition, AveragePosition))
                {
                    CentreTile = Tile;
                }
            }

            // Keep one connected lane from every neighbouring region to the
            // centre. Macro obstacles may shape those lanes but never erase them.
            TMap<FName, TArray<ARoundGridTile*>> BoundaryByNeighbour;
            for (ARoundGridTile* Tile : Tiles)
            {
                for (ARoundGridTile* Neighbour : GetNeighborFields(Tile))
                {
                    if (Neighbour && !RegionSet.Contains(Neighbour))
                    {
                        BoundaryByNeighbour.FindOrAdd(Neighbour->TerrainRegionId).AddUnique(Tile);
                    }
                }
            }
            TSet<TObjectPtr<ARoundGridTile>> ReservedPassages;
            ReservedPassages.Add(CentreTile);
            for (const TPair<FName, TArray<ARoundGridTile*>>& Pair : BoundaryByNeighbour)
            {
                if (Pair.Value.Num() == 0) continue;
                ARoundGridTile* Connector = Pair.Value[GetRandomRange(0, Pair.Value.Num() - 1)];
                TMap<TObjectPtr<ARoundGridTile>, TObjectPtr<ARoundGridTile>> CameFrom;
                TArray<ARoundGridTile*> Queue{Connector};
                CameFrom.Add(Connector, nullptr);
                for (int32 Index = 0; Index < Queue.Num() && !CameFrom.Contains(CentreTile); ++Index)
                {
                    for (ARoundGridTile* Neighbour : GetNeighborFields(Queue[Index]))
                    {
                        if (RegionSet.Contains(Neighbour) && !CameFrom.Contains(Neighbour))
                        {
                            CameFrom.Add(Neighbour, Queue[Index]);
                            Queue.Add(Neighbour);
                        }
                    }
                }
                for (ARoundGridTile* Current = CentreTile; Current; Current = CameFrom.FindRef(Current))
                {
                    ReservedPassages.Add(Current);
                    if (Current == Connector) break;
                }
            }
            for (int32 WidthStep = 1; WidthStep < Structure->PassageWidth; ++WidthStep)
            {
                TSet<TObjectPtr<ARoundGridTile>> Expanded = ReservedPassages;
                for (ARoundGridTile* PassageTile : ReservedPassages)
                    for (ARoundGridTile* Neighbour : GetNeighborFields(PassageTile))
                        if (RegionSet.Contains(Neighbour)) Expanded.Add(Neighbour);
                ReservedPassages = MoveTemp(Expanded);
            }

            const int32 TargetStructureFields = FMath::Clamp(
                FMath::RoundToInt(Tiles.Num() * Structure->Coverage), 1, Tiles.Num());
            TArray<ARoundGridTile*> Available;
            for (ARoundGridTile* Tile : Tiles)
                if (Tile && !ReservedPassages.Contains(Tile)) Available.Add(Tile);
            TArray<ARoundGridTile*> StructureFields;

            const int32 Axis = GetRandomRange(0, 2);
            const auto AxisValue = [Axis](const FVector2D& Position)
            {
                if (Axis == 0) return Position.X;
                if (Axis == 1) return Position.Y;
                return Position.X + Position.Y;
            };
            const double CentreAxis = AxisValue(CentreTile->GridPosition);

            if (Structure->Template == ERoundTerrainStructureTemplate::Scattered)
            {
                const int32 ClusterCount = GetRandomRange(Structure->ClusterCountMin, Structure->ClusterCountMax);
                TSet<TObjectPtr<ARoundGridTile>> Used;
                for (int32 ClusterIndex = 0; ClusterIndex < ClusterCount && StructureFields.Num() < TargetStructureFields; ++ClusterIndex)
                {
                    TArray<ARoundGridTile*> Seeds;
                    for (ARoundGridTile* Tile : Available) if (!Used.Contains(Tile)) Seeds.Add(Tile);
                    if (Seeds.Num() == 0) break;
                    TArray<ARoundGridTile*> Frontier{Seeds[GetRandomRange(0, Seeds.Num() - 1)]};
                    const int32 ClusterTarget = FMath::Max(1, (TargetStructureFields - StructureFields.Num()) / (ClusterCount - ClusterIndex));
                    int32 Applied = 0;
                    while (Frontier.Num() > 0 && Applied < ClusterTarget)
                    {
                        const int32 Pick = GetRandomRange(0, Frontier.Num() - 1);
                        ARoundGridTile* Tile = Frontier[Pick];
                        Frontier.RemoveAtSwap(Pick);
                        if (!Tile || Used.Contains(Tile) || ReservedPassages.Contains(Tile)) continue;
                        Used.Add(Tile); StructureFields.Add(Tile); ++Applied;
                        for (ARoundGridTile* Neighbour : GetNeighborFields(Tile))
                            if (RegionSet.Contains(Neighbour) && !Used.Contains(Neighbour) && !ReservedPassages.Contains(Neighbour)) Frontier.AddUnique(Neighbour);
                    }
                }
            }
            else
            {
                const int32 RingRadius = FMath::Max(2, CentreTile->GetHexDistanceTo(Tiles[GetRandomRange(0, Tiles.Num() - 1)]) / 2);
                Available.Sort([&](const ARoundGridTile& A, const ARoundGridTile& B)
                {
                    auto Score = [&](const ARoundGridTile& Tile)
                    {
                        const double AxisDistance = FMath::Abs(AxisValue(Tile.GridPosition) - CentreAxis);
                        switch (Structure->Template)
                        {
                        case ERoundTerrainStructureTemplate::Split:
                        case ERoundTerrainStructureTemplate::Chokepoint: return AxisDistance;
                        case ERoundTerrainStructureTemplate::Corridor: return FMath::Abs(AxisDistance - static_cast<double>(Structure->PassageWidth + 1));
                        case ERoundTerrainStructureTemplate::Ring: return FMath::Abs(static_cast<double>(CentreTile->GetHexDistanceTo(&Tile) - RingRadius));
                        case ERoundTerrainStructureTemplate::Open: return -static_cast<double>(CentreTile->GetHexDistanceTo(&Tile));
                        case ERoundTerrainStructureTemplate::Landmark: return static_cast<double>(CentreTile->GetHexDistanceTo(&Tile));
                        default: return AxisDistance;
                        }
                    };
                    return Score(A) < Score(B);
                });
                Available.SetNum(FMath::Min(TargetStructureFields, Available.Num()));
                StructureFields = MoveTemp(Available);
            }

            for (ARoundGridTile* Tile : StructureFields)
            {
                ApplyTerrainTypeToField(Tile, Structure->TerrainType);
                ClaimedByLayer.Add(Tile);
            }
            UE_LOG(LogTemp, Log, TEXT("Generated macro terrain structure Region=%s Node=%d Template=%d Terrain=%d Fields=%d ReservedPassages=%d."),
                *RegionData->RegionId.ToString(), RegionIndex, static_cast<int32>(Structure->Template),
                static_cast<int32>(Structure->TerrainType), StructureFields.Num(), ReservedPassages.Num());
        }

        for (const FRoundTerrainRegionLayer& Layer : RegionData->Layers)
        {
            if (Layer.Coverage <= 0.0) continue;
            const int32 TargetCount = FMath::Clamp(FMath::RoundToInt(Tiles.Num() * Layer.Coverage), 1, Tiles.Num());
            TArray<ARoundGridTile*> Chosen;

            if (Layer.Pattern == ERoundTerrainLayerPattern::MapEdgeBand)
            {
                for (ARoundGridTile* Tile : Tiles)
                {
                    const int32 Depth = BoardEdgeDepth.FindRef(Tile);
                    if (Depth >= Layer.DepthMin && Depth <= Layer.DepthMax) Chosen.Add(Tile);
                }
            }
            else if (Layer.Pattern == ERoundTerrainLayerPattern::RegionBorder || Layer.Pattern == ERoundTerrainLayerPattern::RegionCore)
            {
                for (ARoundGridTile* Tile : Tiles)
                {
                    const int32 Depth = RegionEdgeDepth.FindRef(Tile);
                    const bool bMatches = Layer.Pattern == ERoundTerrainLayerPattern::RegionBorder
                        ? Depth >= Layer.DepthMin && Depth <= Layer.DepthMax
                        : Depth >= Layer.DepthMin;
                    if (bMatches) Chosen.Add(Tile);
                }
            }
            else if (Layer.Pattern == ERoundTerrainLayerPattern::AdjacentToLayer)
            {
                if (const TArray<TObjectPtr<ARoundGridTile>>* Source = LayerClaims.Find(Layer.SourceLayerId))
                {
                    TSet<TObjectPtr<ARoundGridTile>> SourceSet;
                    for (ARoundGridTile* SourceTile : *Source) SourceSet.Add(SourceTile);
                    TSet<TObjectPtr<ARoundGridTile>> Candidates;
                    for (ARoundGridTile* SourceTile : *Source)
                        for (ARoundGridTile* Neighbor : GetNeighborFields(SourceTile))
                            if (Neighbor && !SourceSet.Contains(Neighbor)) Candidates.Add(Neighbor);
                    for (ARoundGridTile* Tile : Candidates) Chosen.Add(Tile);
                }
            }
            else if (Layer.Pattern == ERoundTerrainLayerPattern::Crossing)
            {
                if (const TArray<TObjectPtr<ARoundGridTile>>* Source = LayerClaims.Find(Layer.SourceLayerId))
                {
                    TSet<TObjectPtr<ARoundGridTile>> SourceSet;
                    for (ARoundGridTile* SourceTile : *Source) SourceSet.Add(SourceTile);
                    TArray<int32> ValidIndices;
                    const TArray<FVector2D> OppositeOffsets = GetHexNeighborOffsets();
                    for (int32 SourceIndex = 1; SourceIndex + 1 < Source->Num(); ++SourceIndex)
                    {
                        ARoundGridTile* SourceTile = (*Source)[SourceIndex];
                        bool bValidSides = !Layer.bRequireWalkableOppositeSides;
                        if (Layer.bRequireWalkableOppositeSides && SourceTile)
                        {
                            for (int32 DirectionIndex = 0; DirectionIndex < 3; ++DirectionIndex)
                            {
                                ARoundGridTile* SideA = GetField(SourceTile->GridPosition + OppositeOffsets[DirectionIndex]);
                                ARoundGridTile* SideB = GetField(SourceTile->GridPosition + OppositeOffsets[DirectionIndex + 3]);
                                if (SideA && SideB && !SourceSet.Contains(SideA) && !SourceSet.Contains(SideB)
                                    && SideA->IsMoveable() && SideB->IsMoveable())
                                {
                                    bValidSides = true;
                                    break;
                                }
                            }
                        }
                        if (bValidSides) ValidIndices.Add(SourceIndex);
                    }

                    if (Layer.MaximumUncrossedPathFields > 0)
                    {
                        const int32 MinimumIndexDistance = Layer.MinimumCrossingGap + 1;
                        const int32 MaximumIndexDistance = Layer.MaximumUncrossedPathFields + 1;
                        int32 LastChosenIndex = -1;
                        while (LastChosenIndex + MaximumIndexDistance < Source->Num() - 1)
                        {
                            const int32 MinIndex = FMath::Max(1, LastChosenIndex + MinimumIndexDistance);
                            const int32 MaxIndex = FMath::Min(Source->Num() - 2, LastChosenIndex + MaximumIndexDistance);
                            int32 BestIndex = INDEX_NONE;
                            for (const int32 CandidateIndex : ValidIndices)
                            {
                                if (CandidateIndex >= MinIndex && CandidateIndex <= MaxIndex) BestIndex = CandidateIndex;
                            }
                            if (BestIndex == INDEX_NONE)
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Crossing omitted for layer %s: no field with two walkable opposite sides in path range %d-%d."),
                                    *Layer.LayerId.ToString(), MinIndex, MaxIndex);
                                break;
                            }
                            Chosen.Add((*Source)[BestIndex]);
                            LastChosenIndex = BestIndex;
                        }
                    }
                    else
                    {
                        ShuffleArrayInPlace(ValidIndices, MatchRandomStream);
                        ValidIndices.SetNum(FMath::Min(TargetCount, ValidIndices.Num()));
                        ValidIndices.Sort();
                        int32 LastChosenIndex = -1000000;
                        for (const int32 CandidateIndex : ValidIndices)
                        {
                            if (CandidateIndex - LastChosenIndex > Layer.MinimumCrossingGap)
                            {
                                Chosen.Add((*Source)[CandidateIndex]);
                                LastChosenIndex = CandidateIndex;
                            }
                        }
                    }

                    FString CrossingIndices;
                    for (ARoundGridTile* CrossingTile : Chosen)
                    {
                        const int32 CrossingIndex = Source->IndexOfByKey(CrossingTile);
                        CrossingIndices += CrossingIndices.IsEmpty()
                            ? FString::FromInt(CrossingIndex)
                            : FString::Printf(TEXT(",%d"), CrossingIndex);
                    }
                    UE_LOG(LogTemp, Log, TEXT("Generated crossing layer %s: PathFields=%d Crossings=%d Indices=[%s] MinimumGap=%d MaximumUncrossed=%d."),
                        *Layer.LayerId.ToString(), Source->Num(), Chosen.Num(), *CrossingIndices,
                        Layer.MinimumCrossingGap, Layer.MaximumUncrossedPathFields);
                }
            }
            else if (Layer.Pattern == ERoundTerrainLayerPattern::Path)
            {
                const bool bUsesExplicitEndpoints = Layer.PathStartRegionIds.Num() > 0 || Layer.PathEndRegionIds.Num() > 0;
                TArray<ARoundGridTile*> StartCandidates;
                TArray<ARoundGridTile*> EndCandidates;
                if (bUsesExplicitEndpoints)
                {
                    for (ARoundGridTile* Tile : Fields)
                    {
                        if (!Tile) continue;
                        bool bRegionBoundary = false;
                        for (ARoundGridTile* Neighbor : GetNeighborFields(Tile))
                        {
                            if (Neighbor && Neighbor->TerrainRegionId != Tile->TerrainRegionId) { bRegionBoundary = true; break; }
                        }
                        if (!bRegionBoundary) continue;
                        if (Layer.PathStartRegionIds.Contains(Tile->TerrainRegionId)) StartCandidates.Add(Tile);
                        if (Layer.PathEndRegionIds.Contains(Tile->TerrainRegionId)) EndCandidates.Add(Tile);
                    }
                }
                else
                {
                    StartCandidates = RegionEdgeQueue;
                    EndCandidates = RegionEdgeQueue;
                }

                ARoundGridTile* Start = nullptr;
                ARoundGridTile* End = nullptr;
                int32 BestDistance = Layer.PathMinimumLength - 1;
                for (ARoundGridTile* StartCandidate : StartCandidates)
                {
                    for (ARoundGridTile* EndCandidate : EndCandidates)
                    {
                        if (!StartCandidate || !EndCandidate || StartCandidate == EndCandidate) continue;
                        if (StartCandidate->TerrainRegionId == EndCandidate->TerrainRegionId) continue;
                        const int32 Distance = StartCandidate->GetHexDistanceTo(EndCandidate);
                        if (Distance > BestDistance)
                        {
                            Start = StartCandidate;
                            End = EndCandidate;
                            BestDistance = Distance;
                        }
                    }
                }

                if (Start && End)
                {
                    TMap<TObjectPtr<ARoundGridTile>, TObjectPtr<ARoundGridTile>> CameFrom;
                    TArray<ARoundGridTile*> Queue{Start};
                    CameFrom.Add(Start, nullptr);
                    for (int32 Index = 0; Index < Queue.Num() && !CameFrom.Contains(End); ++Index)
                    {
                        for (ARoundGridTile* Neighbor : GetNeighborFields(Queue[Index]))
                        {
                            const bool bAllowed = bUsesExplicitEndpoints
                                ? Neighbor && Neighbor->TerrainRegionId != Registry->StartRegionId
                                : RegionSet.Contains(Neighbor);
                            if (bAllowed && !CameFrom.Contains(Neighbor)) { CameFrom.Add(Neighbor, Queue[Index]); Queue.Add(Neighbor); }
                        }
                    }
                    if (CameFrom.Contains(End))
                    {
                        for (ARoundGridTile* Current = End; Current; Current = CameFrom.FindRef(Current)) Chosen.Add(Current);
                        Algo::Reverse(Chosen);
                        UE_LOG(LogTemp, Log, TEXT("Generated explicit path layer %s from region %s to %s with %d fields."),
                            *Layer.LayerId.ToString(), *Start->TerrainRegionId.ToString(), *End->TerrainRegionId.ToString(), Chosen.Num());
                    }
                }
                else if (bUsesExplicitEndpoints)
                {
                    UE_LOG(LogTemp, Warning, TEXT("Skipped path layer %s: no valid configured start/end region pair with minimum length %d."),
                        *Layer.LayerId.ToString(), Layer.PathMinimumLength);
                }
            }
            else if (Layer.Pattern == ERoundTerrainLayerPattern::Scatter)
            {
                TArray<ARoundGridTile*> Available = Tiles;
                ShuffleArrayInPlace(Available, MatchRandomStream);
                for (ARoundGridTile* Tile : Available)
                    if (!ClaimedByLayer.Contains(Tile) && Chosen.Num() < TargetCount) Chosen.Add(Tile);
            }
            else
            {
                int32 Remaining = TargetCount;
                while (Remaining > 0)
                {
                    TArray<ARoundGridTile*> Available;
                    for (ARoundGridTile* Tile : Tiles) if (Tile && !ClaimedByLayer.Contains(Tile)) Available.Add(Tile);
                    if (Available.Num() == 0) break;
                    const int32 PatchTarget = FMath::Min(Remaining, GetRandomRange(FMath::Max(1, Layer.PatchSizeMin), FMath::Max(Layer.PatchSizeMin, Layer.PatchSizeMax)));
                    TArray<ARoundGridTile*> Frontier{Available[GetRandomRange(0, Available.Num() - 1)]};
                    TSet<TObjectPtr<ARoundGridTile>> Seen;
                    int32 Applied = 0;
                    while (Frontier.Num() > 0 && Applied < PatchTarget)
                    {
                        ARoundGridTile* Tile = Frontier.Pop(false);
                        if (!Tile || Seen.Contains(Tile) || ClaimedByLayer.Contains(Tile)) continue;
                        Seen.Add(Tile); Chosen.Add(Tile); ++Applied;
                        for (ARoundGridTile* Neighbor : GetNeighborFields(Tile)) if (RegionSet.Contains(Neighbor) && !Seen.Contains(Neighbor)) Frontier.Add(Neighbor);
                    }
                    if (Applied == 0) break;
                    Remaining -= Applied;
                }
            }
            if (Chosen.Num() > TargetCount
                && Layer.Pattern != ERoundTerrainLayerPattern::MapEdgeBand
                && Layer.Pattern != ERoundTerrainLayerPattern::Path
                && Layer.Pattern != ERoundTerrainLayerPattern::Crossing)
            {
                Chosen.SetNum(TargetCount);
            }
            ApplyChosen(Layer, Chosen);
        }
    };

    TArray<ARoundGridTile*> StartTiles;
    for (ARoundGridTile* Tile : StartFields) StartTiles.Add(Tile);
    for (ARoundGridTile* Tile : StartTiles)
    {
        Tile->TerrainRegionId = StartRegion->RegionId;
        Tile->TerrainRegionIndex = INDEX_NONE;
        ApplyTerrainTypeToField(Tile, StartRegion->BaseTerrainType);
    }
    for (int32 RegionIndex = 0; RegionIndex < RegionFields.Num(); ++RegionIndex)
    {
        const URoundTerrainRegionData* RegionData = Registry->GetRegion(AssignedTypes[RegionIndex]);
        if (!RegionData) continue;
        for (ARoundGridTile* Tile : RegionFields[RegionIndex])
        {
            Tile->TerrainRegionId = RegionData->RegionId;
            Tile->TerrainRegionIndex = RegionIndex;
            ApplyTerrainTypeToField(Tile, RegionData->BaseTerrainType);
        }
    }
    ApplyLayers(StartTiles, StartRegion, INDEX_NONE);

    TArray<int32> DeferredStructuralRegions;
    for (int32 RegionIndex = 0; RegionIndex < RegionFields.Num(); ++RegionIndex)
    {
        const URoundTerrainRegionData* RegionData = Registry->GetRegion(AssignedTypes[RegionIndex]);
        const bool bHasExplicitPath = RegionData && RegionData->Layers.ContainsByPredicate([](const FRoundTerrainRegionLayer& Layer)
        {
            return Layer.Pattern == ERoundTerrainLayerPattern::Path
                && (Layer.PathStartRegionIds.Num() > 0 || Layer.PathEndRegionIds.Num() > 0);
        });
        if (bHasExplicitPath)
        {
            DeferredStructuralRegions.Add(RegionIndex);
            continue;
        }
        ApplyLayers(RegionFields[RegionIndex], RegionData, RegionIndex);
        UE_LOG(LogTemp, Log, TEXT("Generated terrain region Node=%d Type=%s Fields=%d"), RegionIndex, *AssignedTypes[RegionIndex].ToString(), RegionFields[RegionIndex].Num());
    }
    for (const int32 RegionIndex : DeferredStructuralRegions)
    {
        ApplyLayers(RegionFields[RegionIndex], Registry->GetRegion(AssignedTypes[RegionIndex]), RegionIndex);
        UE_LOG(LogTemp, Log, TEXT("Generated structural terrain region Node=%d Type=%s Fields=%d"), RegionIndex, *AssignedTypes[RegionIndex].ToString(), RegionFields[RegionIndex].Num());
    }

    TArray<ARoundGridTile*> GlobalFeatureTiles;
    for (ARoundGridTile* Tile : Fields) if (Tile) GlobalFeatureTiles.Add(Tile);
    for (const TPair<FName, TObjectPtr<URoundTerrainRegionData>>& Pair : Registry->Regions)
    {
        const URoundTerrainRegionData* FeatureData = Pair.Value;
        if (!FeatureData || !FeatureData->bGlobalFeature) continue;
        ApplyLayers(GlobalFeatureTiles, FeatureData, INDEX_NONE);
        UE_LOG(LogTemp, Log, TEXT("Generated global terrain feature Type=%s BoardFields=%d"),
            *FeatureData->RegionId.ToString(), GlobalFeatureTiles.Num());
    }

    if (Registry->bEnsureWalkableConnectivity)
    {
        auto BuildWalkableComponents = [this]()
        {
            TArray<TArray<ARoundGridTile*>> Components;
            TSet<TObjectPtr<ARoundGridTile>> Visited;
            for (ARoundGridTile* Seed : Fields)
            {
                if (!Seed || !Seed->IsMoveable() || Visited.Contains(Seed)) continue;
                TArray<ARoundGridTile*> Component;
                TArray<ARoundGridTile*> Queue{Seed};
                Visited.Add(Seed);
                for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
                {
                    ARoundGridTile* Current = Queue[QueueIndex];
                    Component.Add(Current);
                    for (ARoundGridTile* Neighbor : GetNeighborFields(Current))
                    {
                        if (Neighbor && Neighbor->IsMoveable() && !Visited.Contains(Neighbor))
                        {
                            Visited.Add(Neighbor);
                            Queue.Add(Neighbor);
                        }
                    }
                }
                Components.Add(MoveTemp(Component));
            }
            Components.Sort([](const TArray<ARoundGridTile*>& A, const TArray<ARoundGridTile*>& B)
            {
                return A.Num() > B.Num();
            });
            return Components;
        };

        int32 CarvedFields = 0;
        for (int32 ConnectionAttempt = 0; ConnectionAttempt < Fields.Num(); ++ConnectionAttempt)
        {
            const TArray<TArray<ARoundGridTile*>> Components = BuildWalkableComponents();
            if (Components.Num() <= 1) break;

            TSet<TObjectPtr<ARoundGridTile>> ConnectedSet;
            for (ARoundGridTile* Tile : Components[0]) ConnectedSet.Add(Tile);

            TMap<TObjectPtr<ARoundGridTile>, int32> Cost;
            TMap<TObjectPtr<ARoundGridTile>, TObjectPtr<ARoundGridTile>> Previous;
            TSet<TObjectPtr<ARoundGridTile>> Closed;
            TArray<ARoundGridTile*> Open;
            for (ARoundGridTile* Tile : Components[0])
            {
                Cost.Add(Tile, 0);
                Open.Add(Tile);
            }

            ARoundGridTile* Reached = nullptr;
            while (Open.Num() > 0 && !Reached)
            {
                int32 BestIndex = 0;
                for (int32 Index = 1; Index < Open.Num(); ++Index)
                {
                    if (Cost.FindRef(Open[Index]) < Cost.FindRef(Open[BestIndex])) BestIndex = Index;
                }
                ARoundGridTile* Current = Open[BestIndex];
                Open.RemoveAtSwap(BestIndex);
                if (Closed.Contains(Current)) continue;
                Closed.Add(Current);

                if (Current->IsMoveable() && !ConnectedSet.Contains(Current))
                {
                    Reached = Current;
                    break;
                }

                for (ARoundGridTile* Neighbor : GetNeighborFields(Current))
                {
                    if (!Neighbor || Closed.Contains(Neighbor)) continue;
                    const int32 NewCost = Cost.FindRef(Current) + (Neighbor->IsMoveable() ? 0 : 1);
                    const int32* OldCost = Cost.Find(Neighbor);
                    if (!OldCost || NewCost < *OldCost)
                    {
                        Cost.Add(Neighbor, NewCost);
                        Previous.Add(Neighbor, Current);
                        Open.Add(Neighbor);
                    }
                }
            }

            if (!Reached) break;
            for (ARoundGridTile* PathTile = Reached; PathTile && !ConnectedSet.Contains(PathTile); PathTile = Previous.FindRef(PathTile))
            {
                if (!PathTile->IsMoveable())
                {
                    ApplyTerrainTypeToField(PathTile, Registry->ConnectivityPassTerrainType);
                    ++CarvedFields;
                }
            }
        }

        const int32 RemainingComponents = BuildWalkableComponents().Num();
        UE_LOG(LogTemp, Log, TEXT("Terrain connectivity validation - CarvedFields=%d WalkableComponents=%d"), CarvedFields, RemainingComponents);
    }

    if (Registry->bEnsureStartPositionsConnected)
    {
        const TArray<FVector2D> StartCenters = BuildTerrainStartRegionCenters(*this);
        auto FindClosestField = [this](const FVector2D& Position)
        {
            ARoundGridTile* Best = nullptr;
            int32 BestDistance = MAX_int32;
            for (ARoundGridTile* Field : Fields)
            {
                if (!Field) continue;
                const int32 Distance = GetAxialHexDistance(Field->GridPosition, Position);
                if (Distance < BestDistance)
                {
                    Best = Field;
                    BestDistance = Distance;
                }
            }
            return Best;
        };

        if (StartCenters.Num() >= 2)
        {
            ARoundGridTile* StartA = FindClosestField(StartCenters[0]);
            ARoundGridTile* StartB = FindClosestField(StartCenters[1]);
            if (StartA && StartB)
            {
                TSet<TObjectPtr<ARoundGridTile>> Reachable;
                TArray<ARoundGridTile*> ReachQueue{StartA};
                Reachable.Add(StartA);
                for (int32 QueueIndex = 0; QueueIndex < ReachQueue.Num(); ++QueueIndex)
                {
                    for (ARoundGridTile* Neighbor : GetNeighborFields(ReachQueue[QueueIndex]))
                    {
                        if (Neighbor && Neighbor->IsMoveable() && !Reachable.Contains(Neighbor))
                        {
                            Reachable.Add(Neighbor);
                            ReachQueue.Add(Neighbor);
                        }
                    }
                }

                int32 StartConnectionCarvedFields = 0;
                if (!Reachable.Contains(StartB))
                {
                    TMap<TObjectPtr<ARoundGridTile>, int32> Cost;
                    TMap<TObjectPtr<ARoundGridTile>, TObjectPtr<ARoundGridTile>> Previous;
                    TSet<TObjectPtr<ARoundGridTile>> Closed;
                    TArray<ARoundGridTile*> Open{StartA};
                    Cost.Add(StartA, 0);
                    while (Open.Num() > 0 && !Closed.Contains(StartB))
                    {
                        int32 BestIndex = 0;
                        for (int32 Index = 1; Index < Open.Num(); ++Index)
                        {
                            if (Cost.FindRef(Open[Index]) < Cost.FindRef(Open[BestIndex])) BestIndex = Index;
                        }
                        ARoundGridTile* Current = Open[BestIndex];
                        Open.RemoveAtSwap(BestIndex);
                        if (!Current || Closed.Contains(Current)) continue;
                        Closed.Add(Current);
                        for (ARoundGridTile* Neighbor : GetNeighborFields(Current))
                        {
                            if (!Neighbor || Closed.Contains(Neighbor)) continue;
                            const int32 NewCost = Cost.FindRef(Current) + (Neighbor->IsMoveable() ? 0 : 1);
                            const int32* OldCost = Cost.Find(Neighbor);
                            if (!OldCost || NewCost < *OldCost)
                            {
                                Cost.Add(Neighbor, NewCost);
                                Previous.Add(Neighbor, Current);
                                Open.Add(Neighbor);
                            }
                        }
                    }
                    if (Closed.Contains(StartB))
                    {
                        for (ARoundGridTile* Tile = StartB; Tile && Tile != StartA; Tile = Previous.FindRef(Tile))
                        {
                            if (!Tile->IsMoveable())
                            {
                                ApplyTerrainTypeToField(Tile, Registry->ConnectivityPassTerrainType);
                                ++StartConnectionCarvedFields;
                            }
                        }
                    }
                }

                TSet<TObjectPtr<ARoundGridTile>> FinalReachable;
                TArray<ARoundGridTile*> FinalQueue{StartA};
                FinalReachable.Add(StartA);
                for (int32 QueueIndex = 0; QueueIndex < FinalQueue.Num(); ++QueueIndex)
                {
                    for (ARoundGridTile* Neighbor : GetNeighborFields(FinalQueue[QueueIndex]))
                    {
                        if (Neighbor && Neighbor->IsMoveable() && !FinalReachable.Contains(Neighbor))
                        {
                            FinalReachable.Add(Neighbor);
                            FinalQueue.Add(Neighbor);
                        }
                    }
                }
                if (FinalReachable.Contains(StartB))
                {
                    UE_LOG(LogTemp, Log, TEXT("Start position connectivity validation - Connected=true CarvedFields=%d StartA=%s StartB=%s"),
                        StartConnectionCarvedFields, *StartA->GridPosition.ToString(), *StartB->GridPosition.ToString());
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Start position connectivity validation - Connected=false StartA=%s StartB=%s"),
                        *StartA->GridPosition.ToString(), *StartB->GridPosition.ToString());
                }
            }
        }
    }
    return true;
}

TArray<ERoundTerrainType> ARoundGameState::OrderTerrainTypes(const TArray<ERoundTerrainType>& InTerrainTypes) const
{
    TArray<ERoundTerrainType> OrderedTerrainTypes = InTerrainTypes;
    OrderedTerrainTypes.StableSort([](const ERoundTerrainType A, const ERoundTerrainType B)
    {
        return GetTerrainPriority(A) < GetTerrainPriority(B);
    });
    return OrderedTerrainTypes;
}

ARoundGridTile* ARoundGameState::SpawnFieldAt(const FVector2D& GridPosition, const ERoundTerrainType TerrainType)
{
    if (!GetWorld())
    {
        return nullptr;
    }

    const TSubclassOf<ARoundGridTile>* SpecificClass = TerrainFieldClasses.Find(TerrainType);
    TSubclassOf<ARoundGridTile> FieldClass = SpecificClass ? *SpecificClass : DefaultFieldClass;
    if (!FieldClass)
    {
        FieldClass = ARoundGridTile::StaticClass();
    }

    const FVector WorldLocation = GetLegacyHexWorldLocation(FieldClass, GridPosition);
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ARoundGridTile* SpawnedField = GetWorld()->SpawnActor<ARoundGridTile>(FieldClass, WorldLocation, FRotator::ZeroRotator, SpawnParameters);
    if (!SpawnedField)
    {
        UE_LOG(LogTemp, Warning, TEXT("SpawnFieldAt failed - Class=%s Pos=%s"), *GetNameSafe(FieldClass), *GridPosition.ToString());
        return nullptr;
    }

    SpawnedField->GridPosition = GridPosition;
    SpawnedField->GridCoordinate = FIntPoint(FMath::RoundToInt(GridPosition.X), FMath::RoundToInt(GridPosition.Y));
    ConfigureFieldForTerrain(SpawnedField, TerrainType);
    RegisterField(SpawnedField);
    return SpawnedField;
}

void ARoundGameState::CreateTerrain(const ERoundTerrainType TerrainType)
{
    if (TerrainType == ERoundTerrainType::Ground || Fields.Num() == 0)
    {
        return;
    }

    int32 ClusterSizeMin = TerrainClusterSizes.FindRef(TerrainType);
    int32 ClusterSizeMax = ClusterSizeMin;
    if (const URoundTerrainData* TerrainData = GetTerrainData(TerrainType))
    {
        ClusterSizeMin = TerrainData->ClusterSizeMin;
        ClusterSizeMax = TerrainData->ClusterSizeMax;
    }

    ClusterSizeMin = FMath::Max(1, ClusterSizeMin);
    ClusterSizeMax = FMath::Max(ClusterSizeMin, ClusterSizeMax);
    const int32 ClusterSize = GetRandomRange(ClusterSizeMin, ClusterSizeMax);
    const TArray<FVector2D> ProtectedSpawnPositions = BuildProtectedTerrainSpawnPositions(*this);
    UE_LOG(LogTemp, Log, TEXT("CreateTerrain - TerrainType=%d ClusterSizeMin=%d ClusterSizeMax=%d ChosenClusterSize=%d Seed=%d"),
        static_cast<int32>(TerrainType),
        ClusterSizeMin,
        ClusterSizeMax,
        ClusterSize,
        MatchRandomSeed);
    if (ProtectedSpawnPositions.Num() > 0)
    {
        FString ProtectedPositionsText;
        for (int32 Index = 0; Index < ProtectedSpawnPositions.Num(); ++Index)
        {
            if (Index > 0)
            {
                ProtectedPositionsText += TEXT(", ");
            }
            ProtectedPositionsText += ProtectedSpawnPositions[Index].ToString();
        }

        UE_LOG(LogTemp, Log, TEXT("CreateTerrain protected spawn positions - %s"), *ProtectedPositionsText);
    }

    int32 AppliedCount = 0;
    switch (TerrainType)
    {
    case ERoundTerrainType::Street:
        AppliedCount = ApplyStreetTerrainPath(*this, TerrainType, ClusterSize, ProtectedSpawnPositions);
        break;
    case ERoundTerrainType::Forest:
    case ERoundTerrainType::Thorns:
    case ERoundTerrainType::Grove:
        AppliedCount = ApplyWildConnectedTerrain(*this, TerrainType, ClusterSize, ProtectedSpawnPositions);
        break;
    case ERoundTerrainType::Rubble:
    case ERoundTerrainType::Rocks:
        AppliedCount = ApplyCompactTerrainBlob(*this, TerrainType, ClusterSize, ProtectedSpawnPositions);
        break;
    default:
        AppliedCount = ApplyWildConnectedTerrain(*this, TerrainType, ClusterSize, ProtectedSpawnPositions);
        break;
    }

    UE_LOG(LogTemp, Log, TEXT("CreateTerrain applied - TerrainType=%d AppliedCount=%d RequestedClusterSize=%d"),
        static_cast<int32>(TerrainType),
        AppliedCount,
        ClusterSize);

    OnBoardStateChanged.Broadcast();
}

void ARoundGameState::ApplyTerrainTypeToField(ARoundGridTile* Field, const ERoundTerrainType TerrainType)
{
    if (!Field)
    {
        return;
    }

    Field->SetTerrainType(TerrainType);
}

TArray<ARoundGridTile*> ARoundGameState::GetNeighborFields(ARoundGridTile* Field) const
{
    TArray<ARoundGridTile*> NeighborFields;
    if (!Field)
    {
        return NeighborFields;
    }

    for (const FVector2D& Offset : GetHexNeighborOffsets())
    {
        if (ARoundGridTile* Neighbor = GetField(Field->GridPosition + Offset))
        {
            NeighborFields.Add(Neighbor);
        }
    }

    return NeighborFields;
}

TArray<ARoundGridTile*> ARoundGameState::GetFieldsBetween(ARoundGridTile* FromField, ARoundGridTile* ToField, const bool bIncludeEndpoints) const
{
    TArray<ARoundGridTile*> LineFields;

    if (!FromField || !ToField)
    {
        return LineFields;
    }

    if (FromField == ToField)
    {
        if (bIncludeEndpoints)
        {
            LineFields.Add(FromField);
        }

        return LineFields;
    }

    const int32 Distance = FromField->GetDistance(ToField);
    if (Distance <= 0)
    {
        return LineFields;
    }

    const FCubeCoord FromCube = AxialToCube(FromField->GridPosition);
    const FCubeCoord ToCube = AxialToCube(ToField->GridPosition);

    for (int32 Step = 0; Step <= Distance; ++Step)
    {
        if (!bIncludeEndpoints && (Step == 0 || Step == Distance))
        {
            continue;
        }

        const double Alpha = static_cast<double>(Step) / static_cast<double>(Distance);
        const FVector2D AxialPosition = CubeToAxial(CubeRound(CubeLerp(FromCube, ToCube, Alpha)));
        if (ARoundGridTile* Field = GetField(AxialPosition))
        {
            LineFields.AddUnique(Field);
        }
    }

    return LineFields;
}

void ARoundGameState::CharacterDie(ARoundCharacter* Character)
{
    if (!Character)
    {
        return;
    }

    RecordCharacterDeath(Character);

    if (Character->CurrentTile && Character->CurrentTile->OccupyingCharacter == Character)
    {
        Character->CurrentTile->OccupyingCharacter = nullptr;
        Character->CurrentTile = nullptr;
    }

    RegisteredCharacters.Remove(Character);
    TurnOrder.RemoveAll([Character](const FRoundTurnEntry& Entry)
    {
        return Entry.Character == Character;
    });

    if (CurrentTurnCharacter == Character)
    {
        CurrentTurnCharacter = nullptr;
    }

    OnBoardStateChanged.Broadcast();
    RebuildTurnOrder();
    TryFinalizeMatchRecordingAfterDeath();
}

ARoundCharacter* ARoundGameState::SetNextTurn()
{
    RebuildTurnOrder();
    EnsureObjectiveCycleInitialized();

    for (int32 Safety = 0; Safety < 10000; ++Safety)
    {
        TArray<ARoundCharacter*> EligibleCharacters;
        int32 HighestInitiative = MIN_int32;

        for (ARoundCharacter* Character : RegisteredCharacters)
        {
            if (!Character || !Character->IsAlive() || !Character->Attributes)
            {
                continue;
            }

            if (Character->CanGetTurn())
            {
                const int32 InitiativeValue = Character->InitiativeToTurn();
                if (InitiativeValue > HighestInitiative)
                {
                    HighestInitiative = InitiativeValue;
                    EligibleCharacters.Reset();
                    EligibleCharacters.Add(Character);
                }
                else if (InitiativeValue == HighestInitiative)
                {
                    EligibleCharacters.Add(Character);
                }
            }
        }

        if (EligibleCharacters.Num() > 0)
        {
            InitializeMatchRandomSeed();
            ShuffleArrayInPlace(EligibleCharacters, MatchRandomStream);
            CurrentTurnCharacter = EligibleCharacters[0];
            ++MatchTurnCounter;
            if (CurrentTurnCharacter)
            {
                CurrentTurnCharacter->BeginTurn();
                RecordTurnStarted(CurrentTurnCharacter);
            }

            RebuildTurnOrder();
            OnTurnChanged.Broadcast(CurrentTurnCharacter);
            OnBoardStateChanged.Broadcast();
            return CurrentTurnCharacter;
        }

        bool bReducedAnyInitiative = false;
        for (ARoundCharacter* Character : RegisteredCharacters)
        {
            if (!Character || !Character->IsAlive() || !Character->Attributes)
            {
                continue;
            }

            Character->Attributes->GainInitiative(Character->ModifyInitiativeGainFromEffects(1));
            Character->AdvanceGlobalInitiative(1);
            for (ARoundAbilityActor* AbilityActor : Character->GrantedAbilities)
            {
                if (AbilityActor)
                {
                    AbilityActor->ReduceCooldown(1);
                }
            }
            bReducedAnyInitiative = true;
        }

        for (ARoundGridTile* Field : Fields)
        {
            if (ARoundTerrainActor* TerrainActor = Field ? Cast<ARoundTerrainActor>(Field->TerrainObject) : nullptr)
            {
                TerrainActor->AdvanceTerrainInitiative(1);
            }
        }

        if (!bReducedAnyInitiative)
        {
            break;
        }

        ++MatchGlobalInitiativeElapsed;
        AdvanceObjectivesByInitiative(1);
    }

    CurrentTurnCharacter = nullptr;
    if (CurrentTurnCharacter)
    {
        CurrentTurnCharacter->BeginTurn();
    }

    OnTurnChanged.Broadcast(CurrentTurnCharacter);
    OnBoardStateChanged.Broadcast();
    return CurrentTurnCharacter;
}

void ARoundGameState::SetVision()
{
    if (!HasAuthority())
    {
        return;
    }

    for (UObject* TeamObject : Teams)
    {
        if (ARoundTeamActor* TeamActor = Cast<ARoundTeamActor>(TeamObject))
        {
            TeamActor->SetVisionFields();
        }
        else
        {
            InvokeNoArgFunction(TeamObject, TEXT("SetVisionFields"));
        }
    }


    NotifyVisionUpdated(Player1);
    NotifyVisionUpdated(Player2);
    OnBoardStateChanged.Broadcast();
}

void ARoundGameState::ScheduleInitialVisionRefresh()
{
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    RemainingInitialVisionRefreshes = 15;
    GetWorld()->GetTimerManager().ClearTimer(InitialVisionRefreshTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(
        InitialVisionRefreshTimerHandle,
        [this]()
        {
            if (!IsValid(this) || !GetWorld())
            {
                return;
            }

            SetVision();

            --RemainingInitialVisionRefreshes;
            if (RemainingInitialVisionRefreshes <= 0)
            {
                GetWorld()->GetTimerManager().ClearTimer(InitialVisionRefreshTimerHandle);
            }
        },
        0.2f,
        true,
        0.0f);
}

void ARoundGameState::RebuildTurnOrder()
{
    TurnOrder.Empty();

    for (ARoundCharacter* Character : RegisteredCharacters)
    {
        if (!Character || !Character->IsAlive() || !Character->Attributes)
        {
            continue;
        }

        FRoundTurnEntry Entry;
        Entry.Character = Character;
        Entry.Initiative = Character->InitiativeToTurn();
        TurnOrder.Add(Entry);
    }

    Algo::Sort(TurnOrder, [](const FRoundTurnEntry& A, const FRoundTurnEntry& B)
    {
        return A.Initiative > B.Initiative;
    });

    if (TurnOrder.Num() == 0)
    {
        ActiveTurnIndex = INDEX_NONE;
        CurrentTurnCharacter = nullptr;
        return;
    }

    ActiveTurnIndex = FMath::Clamp(ActiveTurnIndex, 0, TurnOrder.Num() - 1);
    if (!CurrentTurnCharacter)
    {
        CurrentTurnCharacter = GetActiveCharacter();
    }
    OnTurnChanged.Broadcast(CurrentTurnCharacter);
}

ARoundCharacter* ARoundGameState::GetActiveCharacter() const
{
    if (CurrentTurnCharacter)
    {
        return CurrentTurnCharacter;
    }

    return TurnOrder.IsValidIndex(ActiveTurnIndex) ? TurnOrder[ActiveTurnIndex].Character.Get() : nullptr;
}

bool ARoundGameState::AdvanceTurn()
{
    return SetNextTurn() != nullptr;
}

TArray<FRoundObjectivePreviewEntry> ARoundGameState::GetObjectivePreviewEntries() const
{
    TArray<FRoundObjectivePreviewEntry> Entries;

    auto AddPreviewEntry = [&Entries](const ARoundObjectiveActor* Objective)
    {
        if (!Objective || !Objective->IsPreviewing())
        {
            return;
        }

        FRoundObjectivePreviewEntry Entry;
        Entry.ObjectiveType = Objective->ObjectiveType;
        Entry.RewardName = Objective->RewardDisplayName;
        Entry.SpawnAtGlobalInitiative = Objective->SpawnAtGlobalInitiative;
        Entry.TargetTile = Objective->CenterTile;
        Entry.Description = Objective->RewardDescription;
        Entries.Add(Entry);
    };

    if (GetWorld())
    {
        for (TActorIterator<ARoundObjectiveActor> It(GetWorld()); It; ++It)
        {
            AddPreviewEntry(*It);
        }
    }

    return Entries;
}

bool ARoundGameState::TryStartObjectiveClaim(ARoundCharacter* Character, FString& OutReason)
{
    OutReason.Reset();

    if (!Character)
    {
        OutReason = TEXT("No character selected.");
        return false;
    }

    if (!CurrentTurnCharacter || CurrentTurnCharacter != Character)
    {
        OutReason = TEXT("Character is not currently taking a turn.");
        return false;
    }

    for (TActorIterator<ARoundObjectiveActor> It(GetWorld()); It; ++It)
    {
        ARoundObjectiveActor* Objective = *It;
        if (!Objective
            || Objective->ObjectiveType != ERoundObjectiveType::Channel
            || Objective->ObjectiveState != ERoundObjectiveState::Active
            || Objective->CenterTile != Character->CurrentTile)
        {
            continue;
        }

        if (Objective->ChannelingCharacter && Objective->ChannelingCharacter != Character)
        {
            OutReason = TEXT("Objective is already being claimed.");
            return false;
        }

        Objective->ChannelingCharacter = Character;
        Objective->ChannelRemainingInitiative = FMath::Max(1, Objective->RequiredProgressInitiative);
        OnBoardStateChanged.Broadcast();
        return true;
    }

    OutReason = TEXT("No claimable objective on this tile.");
    return false;
}

bool ARoundGameState::IsCharacterBlockedByObjectiveChannel(const ARoundCharacter* Character) const
{
    if (!Character)
    {
        return false;
    }

    for (TActorIterator<ARoundObjectiveActor> It(GetWorld()); It; ++It)
    {
        const ARoundObjectiveActor* Objective = *It;
        if (Objective
            && Objective->ObjectiveType == ERoundObjectiveType::Channel
            && Objective->ObjectiveState == ERoundObjectiveState::Active
            && Objective->ChannelingCharacter == Character)
        {
            return true;
        }
    }

    return false;
}

bool ARoundGameState::CanCharacterStartObjectiveClaim(const ARoundCharacter* Character, FString& OutReason) const
{
    OutReason.Reset();

    if (bReplayActive)
    {
        return false;
    }

    if (!Character)
    {
        OutReason = TEXT("No character selected.");
        return false;
    }

    if (!CurrentTurnCharacter || CurrentTurnCharacter != Character)
    {
        OutReason = TEXT("Character is not currently taking a turn.");
        return false;
    }

    for (TActorIterator<ARoundObjectiveActor> It(GetWorld()); It; ++It)
    {
        const ARoundObjectiveActor* Objective = *It;
        if (!Objective
            || Objective->ObjectiveType != ERoundObjectiveType::Channel
            || Objective->ObjectiveState != ERoundObjectiveState::Active
            || Objective->CenterTile != Character->CurrentTile)
        {
            continue;
        }

        if (Objective->ChannelingCharacter && Objective->ChannelingCharacter != Character)
        {
            OutReason = TEXT("Objective is already being claimed.");
            return false;
        }

        return true;
    }

    OutReason = TEXT("No claimable objective on this tile.");
    return false;
}

void ARoundGameState::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateActivePresentationEvent(DeltaSeconds);
    UpdateFloatingTexts(DeltaSeconds);
    UpdateTransientMeshes(DeltaSeconds);
}

void ARoundGameState::InitializeMatchRandomSeed() const
{
    if (bMatchRandomStreamInitialized)
    {
        return;
    }

    if (MatchRandomSeed == 0)
    {
        const FString TrimmedSeedOverride = MatchSeedOverride.TrimStartAndEnd();
        int32 ResolvedSeed = 0;

        if (!TrimmedSeedOverride.IsEmpty())
        {
            if (!FDefaultValueHelper::ParseInt(TrimmedSeedOverride, ResolvedSeed))
            {
                UE_LOG(LogTemp, Warning, TEXT("RoundGameState MatchSeedOverride '%s' is invalid. Falling back to a random seed."), *TrimmedSeedOverride);
                ResolvedSeed = 0;
            }
        }

        if (ResolvedSeed == 0)
        {
            const int32 NewSeed = static_cast<int32>(FDateTime::UtcNow().GetTicks() ^ FPlatformTime::Cycles64());
            ResolvedSeed = (NewSeed == 0) ? 1 : NewSeed;
        }

        if (ResolvedSeed == 0)
        {
            ResolvedSeed = 1;
        }

        const_cast<ARoundGameState*>(this)->MatchRandomSeed = ResolvedSeed;
    }

    MatchRandomStream.Initialize(MatchRandomSeed);
    bMatchRandomStreamInitialized = true;
}

int32 ARoundGameState::GetRandomRange(const int32 MinInclusive, const int32 MaxInclusive) const
{
    InitializeMatchRandomSeed();
    return MatchRandomStream.RandRange(MinInclusive, MaxInclusive);
}

float ARoundGameState::GetRandomFraction(const float MinInclusive, const float MaxInclusive) const
{
    InitializeMatchRandomSeed();
    return MatchRandomStream.FRandRange(MinInclusive, MaxInclusive);
}

double ARoundGameState::GetGlobalDamageEscalationMultiplier() const
{
    return GetRuntimeGlobalDamageEscalationMultiplierForInitiative(MatchGlobalInitiativeElapsed);
}

void ARoundGameState::EnsureObjectiveCycleInitialized()
{
    if (bObjectiveCycleInitialized || !HasAuthority() || Fields.Num() == 0)
    {
        return;
    }

    bObjectiveCycleInitialized = true;
}

void ARoundGameState::AdvanceObjectivesByInitiative(const int32 InitiativeAmount)
{
    if (!HasAuthority() || InitiativeAmount <= 0)
    {
        return;
    }

    int32 ObjectiveCount = 0;
    if (GetWorld())
    {
        for (TActorIterator<ARoundObjectiveActor> It(GetWorld()); It; ++It)
        {
            ARoundObjectiveActor* Objective = *It;
            if (!Objective)
            {
                continue;
            }

            ++ObjectiveCount;
            Objective->AdvanceObjectiveTime(this, MatchGlobalInitiativeElapsed, InitiativeAmount);
        }
    }

    if (ObjectiveCount < 3 && bObjectiveCycleInitialized && MatchGlobalInitiativeElapsed >= NextObjectivePreviewAtGlobalInitiative)
    {
        SpawnNextObjectivePreview();
    }
}

void ARoundGameState::SpawnNextObjectivePreview()
{
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    int32 ObjectiveCount = 0;
    for (TActorIterator<ARoundObjectiveActor> It(GetWorld()); It; ++It)
    {
        if (*It)
        {
            ++ObjectiveCount;
        }
    }
    if (ObjectiveCount >= 3)
    {
        return;
    }

    ARoundGridTile* SpawnTile = FindObjectiveSpawnTile();
    if (!SpawnTile)
    {
        return;
    }

    TArray<ERoundObjectiveType> ObjectiveTypes = {
        ERoundObjectiveType::HoldZone,
        ERoundObjectiveType::DestroyNode,
        ERoundObjectiveType::Channel
    };
    InitializeMatchRandomSeed();
    ShuffleArrayInPlace(ObjectiveTypes, MatchRandomStream);

    ERoundObjectiveType ChosenType = ObjectiveTypes[0];
    if (ChosenType == ERoundObjectiveType::HoldZone && BuildObjectiveZone(SpawnTile).Num() != 7)
    {
        ChosenType = ERoundObjectiveType::DestroyNode;
    }

    const TArray<ARoundGridTile*> ObjectiveZone = ChosenType == ERoundObjectiveType::HoldZone
        ? BuildObjectiveZone(SpawnTile)
        : TArray<ARoundGridTile*>{ SpawnTile };

    URoundObjectiveRewardData* RewardData = ChooseObjectiveRewardData();
    if (!RewardData)
    {
        UE_LOG(LogTemp, Warning, TEXT("Objective preview skipped because no objective reward data is configured."));
        return;
    }
    const int32 SpawnAtInitiative = MatchGlobalInitiativeElapsed + 100;
    const int32 ExpireAtInitiative = 0;
    int32 RequiredProgress = 70;
    int32 ObjectiveHealth = 0;

    switch (ChosenType)
    {
    case ERoundObjectiveType::HoldZone:
        RequiredProgress = 70;
        break;
    case ERoundObjectiveType::DestroyNode:
        ObjectiveHealth = 1000;
        break;
    case ERoundObjectiveType::Channel:
        RequiredProgress = 40;
        break;
    default:
        break;
    }

    TSubclassOf<ARoundObjectiveActor> SpawnClass = ObjectiveActorClass;
    if (!SpawnClass || SpawnClass == ARoundObjectiveActor::StaticClass())
    {
        switch (ChosenType)
        {
        case ERoundObjectiveType::HoldZone:
            SpawnClass = ARoundHoldObjectiveActor::StaticClass();
            break;
        case ERoundObjectiveType::DestroyNode:
            SpawnClass = ARoundDestroyObjectiveActor::StaticClass();
            break;
        case ERoundObjectiveType::Channel:
            SpawnClass = ARoundChannelObjectiveActor::StaticClass();
            break;
        default:
            SpawnClass = ARoundObjectiveActor::StaticClass();
            break;
        }
    }
    ARoundObjectiveActor* SpawnedObjective = GetWorld()->SpawnActor<ARoundObjectiveActor>(
        SpawnClass,
        SpawnTile->GetActorLocation(),
        FRotator::ZeroRotator);
    if (!SpawnedObjective)
    {
        return;
    }

    SpawnedObjective->InitializeObjective(
        ChosenType,
        SpawnTile,
        ObjectiveZone,
        RewardData,
        SpawnAtInitiative,
        ExpireAtInitiative,
        RequiredProgress,
        ObjectiveHealth);
    UE_LOG(LogTemp, Log, TEXT("SpawnNextObjectivePreview: Spawned Objective=%s Type=%d State=%d CenterTile=%s CenterPos=%s SpawnAt=%d ExpireAt=%d RequiredProgress=%d Health=%d Reward=%s"),
        *GetNameSafe(SpawnedObjective),
        static_cast<int32>(ChosenType),
        static_cast<int32>(SpawnedObjective->ObjectiveState),
        *GetNameSafe(SpawnTile),
        *SpawnTile->GridPosition.ToString(),
        SpawnAtInitiative,
        ExpireAtInitiative,
        RequiredProgress,
        ObjectiveHealth,
        RewardData ? *RewardData->RewardId.ToString() : TEXT("None"));
    CurrentObjectiveActor = SpawnedObjective;
    NextObjectivePreviewAtGlobalInitiative += GetRandomRange(300, 500);
    OnBoardStateChanged.Broadcast();
}

TArray<ARoundGridTile*> ARoundGameState::BuildObjectiveZone(ARoundGridTile* CenterTile) const
{
    TArray<ARoundGridTile*> Zone;
    if (!CenterTile)
    {
        return Zone;
    }

    Zone.Add(CenterTile);
    const TArray<ARoundGridTile*> Neighbors = GetNeighborFields(CenterTile);
    if (Neighbors.Num() != 6)
    {
        Zone.Reset();
        return Zone;
    }

    for (ARoundGridTile* Neighbor : Neighbors)
    {
        if (!Neighbor || !Neighbor->IsMoveable())
        {
            Zone.Reset();
            return Zone;
        }

        Zone.AddUnique(Neighbor);
    }

    return Zone;
}

ARoundGridTile* ARoundGameState::FindObjectiveSpawnTile() const
{
    if (NextObjectivePreviewAtGlobalInitiative <= 100)
    {
        if (ARoundGridTile* CenterField = GetField(FVector2D::ZeroVector))
        {
            return CenterField;
        }
    }

    TArray<ARoundGridTile*> Candidates;
    for (ARoundGridTile* Field : Fields)
    {
        if (!Field || !Field->IsMoveable() || Field->OccupyingCharacter)
        {
            continue;
        }

        Candidates.Add(Field);
    }

    Candidates = Candidates.FilterByPredicate([this](ARoundGridTile* Field)
    {
        return Field && BuildObjectiveZone(Field).Num() == 7;
    });

    if (Candidates.Num() == 0)
    {
        for (ARoundGridTile* Field : Fields)
        {
            if (Field && Field->IsMoveable() && !Field->OccupyingCharacter)
            {
                Candidates.Add(Field);
            }
        }
    }

    if (Candidates.Num() == 0)
    {
        return nullptr;
    }

    const int32 RandomIndex = GetRandomRange(0, Candidates.Num() - 1);
    return Candidates[RandomIndex];
}

URoundObjectiveRewardData* ARoundGameState::ChooseObjectiveRewardData() const
{
    TArray<URoundObjectiveRewardData*> LoadedRewards;
    for (const TSoftObjectPtr<URoundObjectiveRewardData>& RewardPtr : ObjectiveRewardPool)
    {
        if (URoundObjectiveRewardData* RewardData = RewardPtr.LoadSynchronous())
        {
            LoadedRewards.Add(RewardData);
        }
    }

    if (LoadedRewards.Num() > 0)
    {
        return LoadedRewards[GetRandomRange(0, LoadedRewards.Num() - 1)];
    }

    URoundObjectiveRewardData* Fallback = NewObject<URoundObjectiveRewardData>(GetTransientPackage());
    if (!Fallback)
    {
        return nullptr;
    }

    switch (GetRandomRange(0, 4))
    {
    case 0:
        Fallback->RewardId = TEXT("ObjectiveHeal");
        Fallback->DisplayName = FText::FromString(TEXT("Heal"));
        Fallback->Description = FText::FromString(TEXT("Restores health when collected."));
        Fallback->RewardType = ERoundObjectiveRewardType::Heal;
        Fallback->Value = 250.0;
        break;
    case 1:
        Fallback->RewardId = TEXT("ObjectiveAttack");
        Fallback->DisplayName = FText::FromString(TEXT("Attack Up"));
        Fallback->Description = FText::FromString(TEXT("Temporary attack boost."));
        Fallback->RewardType = ERoundObjectiveRewardType::AttackAdd;
        Fallback->Value = 20.0;
        Fallback->DurationInitiative = 120;
        break;
    case 2:
        Fallback->RewardId = TEXT("ObjectiveDefense");
        Fallback->DisplayName = FText::FromString(TEXT("Defense Up"));
        Fallback->Description = FText::FromString(TEXT("Temporary defense boost."));
        Fallback->RewardType = ERoundObjectiveRewardType::DefenseAdd;
        Fallback->Value = 15.0;
        Fallback->DurationInitiative = 120;
        break;
    case 3:
        Fallback->RewardId = TEXT("ObjectiveMaxHealth");
        Fallback->DisplayName = FText::FromString(TEXT("Vitality"));
        Fallback->Description = FText::FromString(TEXT("Permanent max health increase."));
        Fallback->RewardType = ERoundObjectiveRewardType::MaxHealthAdd;
        Fallback->Value = 120.0;
        Fallback->bPermanent = true;
        break;
    default:
        Fallback->RewardId = TEXT("ObjectiveMaxInitiative");
        Fallback->DisplayName = FText::FromString(TEXT("Tempo Core"));
        Fallback->Description = FText::FromString(TEXT("Permanent max initiative increase."));
        Fallback->RewardType = ERoundObjectiveRewardType::MaxInitiativeAdd;
        Fallback->Value = 20.0;
        Fallback->bPermanent = true;
        break;
    }

    return Fallback;
}

bool ARoundGameState::IsCoreMoveLegal(const ARoundCharacter* Character, const ARoundGridTile* TargetTile) const
{
    if (!Character || !TargetTile)
    {
        return false;
    }

    if (!CoreMatchAdapter)
    {
        CoreMatchAdapter = MakeUnique<FRoundCoreMatchAdapter>();
    }

    FString SnapshotReason;
    if (!CoreMatchAdapter->Rebuild(*this, SnapshotReason))
    {
        UE_LOG(LogTemp, Warning, TEXT("RoundCore movement snapshot failed: %s"), *SnapshotReason);
        return false;
    }

    FString CoreReason;
    const bool bCoreLegal = CoreMatchAdapter->IsMoveLegal(Character, TargetTile, CoreReason);
    const bool bActorLegal = Character->CanMoveTo(const_cast<ARoundGridTile*>(TargetTile));
    if (bCoreLegal != bActorLegal)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("RoundCore movement mismatch Character=%s Target=%s Actor=%s Core=%s Reason=%s"),
            *GetNameSafe(Character),
            *GetNameSafe(TargetTile),
            bActorLegal ? TEXT("legal") : TEXT("illegal"),
            bCoreLegal ? TEXT("legal") : TEXT("illegal"),
            *CoreReason);
    }
    return bCoreLegal;
}

bool ARoundGameState::PreviewCoreBasicAttack(
    const ARoundCharacter* Character,
    const ARoundCharacter* TargetCharacter,
    FRoundCoreBasicAttackPreview& OutPreview,
    FString& OutReason) const
{
    if (!CoreMatchAdapter)
    {
        CoreMatchAdapter = MakeUnique<FRoundCoreMatchAdapter>();
    }
    if (!CoreMatchAdapter->Rebuild(*this, OutReason))
    {
        return false;
    }
    return CoreMatchAdapter->PreviewBasicAttack(Character, TargetCharacter, OutPreview, OutReason);
}

bool ARoundGameState::ResolveCoreBasicAttackCritical(
    const ARoundCharacter* Character,
    bool& bOutCritical,
    FString& OutReason)
{
    if (!CoreMatchAdapter)
    {
        OutReason = TEXT("Core attack snapshot has not been prepared.");
        return false;
    }
    return CoreMatchAdapter->ResolveBasicAttackCritical(Character, bOutCritical, OutReason);
}

FRoundActionResult ARoundGameState::ExecuteCoreBasicAttack(
    ARoundCharacter* Character, ARoundCharacter* TargetCharacter)
{
    FRoundActionResult Result;
    if (!HasAuthority() || !Character || !TargetCharacter)
    {
        Result.Reason = TEXT("Invalid Core basic-attack context.");
        return Result;
    }
    if (!CoreMatchAdapter)
    {
        CoreMatchAdapter = MakeUnique<FRoundCoreMatchAdapter>();
    }

    FString CoreReason;
    if (!CoreMatchAdapter->Rebuild(*this, CoreReason))
    {
        Result.Reason = FString::Printf(TEXT("Core snapshot failed: %s"), *CoreReason);
        return Result;
    }

    FRoundCoreBasicAttackExecution Execution;
    if (!CoreMatchAdapter->ExecuteBasicAttack(Character, TargetCharacter, Execution, CoreReason))
    {
        Result.Reason = CoreReason;
        return Result;
    }

    for (const FRoundCoreCharacterRuntimeUpdate& Update : Execution.CharacterUpdates)
    {
        if (Update.Character && Update.Tile && Update.Character->CurrentTile != Update.Tile)
        {
            bApplyingCoreMovementCallbacks = true;
            Update.Character->SetField(Update.Tile, false);
            bApplyingCoreMovementCallbacks = false;
        }
        if (Update.Character && Update.Character->Attributes)
        {
            Update.Character->Attributes->SetCurrentHealthValue(Update.Health);
            Update.Character->Attributes->SetCurrentInitiativeValue(Update.Initiative);
        }
        if (ARoundCharacter1* Character1 = Cast<ARoundCharacter1>(Update.Character))
        {
            Character1->NativeChainCount = Update.ResourceCurrent;
            Character1->NativeChainRechargeProgress = Update.ResourceRechargeProgress;
            Character1->NativeDamageRechargeCooldownRemaining = Update.SpecialCooldownRemaining;
            Character1->bGrantedChainSpendRefundThisTurn = !Update.bFirstResourceSpendRefundAvailable;
            Character1->bMovedThisTurn = Update.bMovedSinceLastTurn;
            Character1->bStationaryGuardActive = Update.bStationaryGuardActive;
        }
        ApplyCharacter2CoreState(Update);
        ApplyCharacter3CoreState(Update);
        ApplyCharacter4CoreState(Update);
        ApplyChronomancerCoreState(Update);
        if (ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(Update.Character))
            Character5->ApplyCoreBloodState(Update.ResourceCurrent, Update.SpecialCooldownRemaining,
                Update.bFirstBloodDamageReductionAvailable, Update.bBasicAttackBloodAvailable, Update.bBloodCapTriggerAvailable);
    }
    for (const FRoundCoreAbilityRuntimeUpdate& Update : Execution.AbilityUpdates)
    {
        if (Update.Ability)
        {
            Update.Ability->SetCooldown(Update.Cooldown);
            Update.Ability->CurrentCharges = Update.Charges;
            if (ARoundSnipeAbility* Snipe = Cast<ARoundSnipeAbility>(Update.Ability)) Snipe->ReadyInitiativeElapsed = Update.ReadyInitiativeElapsed;
        }
    }
    for (const FRoundCoreEffectRemoval& Removal : Execution.RemovedEffects)
    {
        if (!Removal.Character)
        {
            continue;
        }
        if (Removal.EffectId.Contains(TEXT(":BearForm")))
            if (ARoundCharacterDruid* Druid = Cast<ARoundCharacterDruid>(Removal.Character)) Druid->ExitBearForm();
        RemoveCoreAimPresentation(Removal.Character, Removal.EffectId);
        const TArray<TObjectPtr<ARoundEffectActor>> EffectsSnapshot = Removal.Character->ActiveEffects;
        for (ARoundEffectActor* Effect : EffectsSnapshot)
        {
            if (Effect && (Effect->EffectId.ToString() == Removal.EffectId
                || (Effect->EffectId.IsNone() && Effect->GetClass()->GetName() == Removal.EffectId)))
            {
                Effect->RemoveEffect();
                break;
            }
        }
    }
    for (const FRoundCoreModifierEffectAddition& Addition : Execution.AddedModifierEffects)
    {
        PresentCoreGenericModifier(GetWorld(), Addition, TEXT("Vorbereiteter Effekt"),
            TEXT("Dieser Effekt wird von RoundCore verwaltet."));
    }

    for (const FRoundCoreDamageEvent& Damage : Execution.DamageEvents)
    {
        if (Damage.Target)
        {
            Damage.Target->ApplyCoreDamageOutcome(
                Damage.Source ? Damage.Source : Character,
                Damage.Amount,
                ERoundDamageType::Physical,
                Damage.bTargetDied);
        }
    }
    const TArray<TObjectPtr<ARoundEffectActor>> AttackEffectsSnapshot = Character->ActiveEffects;
    for (ARoundEffectActor* Effect : AttackEffectsSnapshot)
    {
        if (Effect)
        {
            Effect->OnBasicAttackExecuted(Character, TargetCharacter);
        }
    }

    RecordBasicAttack(Character, TargetCharacter, Execution.AppliedDamage);
    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Attack executed by RoundCore for %d damage."), Execution.AppliedDamage);

    FString ResyncReason;
    if (!CoreMatchAdapter->Rebuild(*this, ResyncReason))
    {
        UE_LOG(LogTemp, Error, TEXT("RoundCore post-attack resync failed: %s"), *ResyncReason);
    }
    SetVision();
    OnBoardStateChanged.Broadcast();
    return Result;
}

FRoundActionResult ARoundGameState::ExecuteCoreAbility(
    ARoundCharacter* Character, ARoundAbilityActor* Ability, ARoundCharacter* TargetCharacter,
    ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!HasAuthority() || !Character || !Ability || (!TargetCharacter && !TargetTile))
    {
        Result.Reason = TEXT("Invalid Core ability context.");
        return Result;
    }
    if (!CoreMatchAdapter)
    {
        CoreMatchAdapter = MakeUnique<FRoundCoreMatchAdapter>();
    }

    FString CoreReason;
    if (!CoreMatchAdapter->Rebuild(*this, CoreReason))
    {
        Result.Reason = FString::Printf(TEXT("Core snapshot failed: %s"), *CoreReason);
        return Result;
    }

    FRoundCoreAbilityExecution Execution;
    if (!CoreMatchAdapter->ExecuteAbility(Character, Ability, TargetCharacter, TargetTile, Execution, CoreReason))
    {
        Result.Reason = CoreReason;
        return Result;
    }

    for (const FRoundCoreModifierEffectAddition& Addition : Execution.AddedModifierEffects)
    {
        if (Addition.CoreEffectKind == static_cast<int32>(roundcore::ModifierEffectKind::BearForm))
            if (ARoundCharacterDruid* Druid = Cast<ARoundCharacterDruid>(Addition.Character)) Druid->EnterBearForm(Ability);
    }

    for (const FRoundCoreCharacterRuntimeUpdate& Update : Execution.CharacterUpdates)
    {
        if (Update.Character && Update.Tile && Update.Character->CurrentTile != Update.Tile)
        {
            bApplyingCoreMovementCallbacks = true;
            Update.Character->SetField(Update.Tile, false);
            bApplyingCoreMovementCallbacks = false;
        }
        if (Update.Character && Update.Character->Attributes)
        {
            Update.Character->Attributes->SetCurrentHealthValue(Update.Health);
            Update.Character->Attributes->SetCurrentInitiativeValue(Update.Initiative);
        }
        if (ARoundCharacter1* Character1 = Cast<ARoundCharacter1>(Update.Character))
        {
            Character1->NativeChainCount = Update.ResourceCurrent;
            Character1->NativeChainRechargeProgress = Update.ResourceRechargeProgress;
            Character1->NativeDamageRechargeCooldownRemaining = Update.SpecialCooldownRemaining;
            Character1->bGrantedChainSpendRefundThisTurn = !Update.bFirstResourceSpendRefundAvailable;
            Character1->bMovedThisTurn = Update.bMovedSinceLastTurn;
            Character1->bStationaryGuardActive = Update.bStationaryGuardActive;
        }
        ApplyCharacter2CoreState(Update);
        ApplyCharacter3CoreState(Update);
        ApplyCharacter4CoreState(Update);
        ApplyChronomancerCoreState(Update);
        if (ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(Update.Character))
            Character5->ApplyCoreBloodState(Update.ResourceCurrent, Update.SpecialCooldownRemaining,
                Update.bFirstBloodDamageReductionAvailable, Update.bBasicAttackBloodAvailable, Update.bBloodCapTriggerAvailable);
    }
    for (const FRoundCoreAbilityRuntimeUpdate& Update : Execution.AbilityUpdates)
    {
        if (Update.Ability)
        {
            Update.Ability->SetCooldown(Update.Cooldown);
            Update.Ability->CurrentCharges = Update.Charges;
            if (ARoundSnipeAbility* Snipe = Cast<ARoundSnipeAbility>(Update.Ability)) Snipe->ReadyInitiativeElapsed = Update.ReadyInitiativeElapsed;
        }
    }
    for (const TPair<ARoundGridTile*, int32>& Update : Execution.TerrainUpdates)
    {
        if (Update.Key) Update.Key->SetTerrainType(static_cast<ERoundTerrainType>(Update.Value));
    }
    for (const FRoundCoreEffectRemoval& Removal : Execution.RemovedEffects)
    {
        if (!Removal.Character)
        {
            continue;
        }
        if (Removal.EffectId.Contains(TEXT(":BearForm")))
            if (ARoundCharacterDruid* Druid = Cast<ARoundCharacterDruid>(Removal.Character)) Druid->ExitBearForm();
        RemoveCoreAimPresentation(Removal.Character, Removal.EffectId);
        const TArray<TObjectPtr<ARoundEffectActor>> EffectsSnapshot = Removal.Character->ActiveEffects;
        for (ARoundEffectActor* Effect : EffectsSnapshot)
        {
            if (Effect && (Effect->EffectId.ToString() == Removal.EffectId
                || (Effect->EffectId.IsNone() && Effect->GetClass()->GetName() == Removal.EffectId)))
            {
                Effect->RemoveEffect();
                break;
            }
        }
    }
    for (const FRoundCoreModifierEffectAddition& Addition : Execution.AddedModifierEffects)
    {
        if (Addition.CoreEffectKind == static_cast<int32>(roundcore::ModifierEffectKind::BearForm))
        {
            PresentCoreGenericModifier(GetWorld(), Addition, TEXT("Bear Form"),
                TEXT("Der Druide befindet sich in Baerenform."));
            continue;
        }
        if (!Addition.Character || !GetWorld())
        {
            continue;
        }
        const TArray<TObjectPtr<ARoundEffectActor>> ExistingEffects = Addition.Character->ActiveEffects;
        for (ARoundEffectActor* ExistingEffect : ExistingEffects)
        {
            if (ExistingEffect && ExistingEffect->EffectId.ToString() == Addition.EffectId)
            {
                ExistingEffect->RemoveEffect();
            }
        }
        if (Addition.CoreEffectKind == static_cast<int32>(roundcore::ModifierEffectKind::AimField)
            && Addition.AreaCenterTile)
        {
            TArray<ARoundGridTile*> TriggerTiles;
            TSet<ARoundGridTile*> Visited;
            TArray<TPair<ARoundGridTile*, int32>> Frontier;
            Frontier.Add({Addition.AreaCenterTile, 0});
            Visited.Add(Addition.AreaCenterTile);
            while (!Frontier.IsEmpty())
            {
                const TPair<ARoundGridTile*, int32> Current = Frontier[0];
                Frontier.RemoveAt(0);
                TriggerTiles.Add(Current.Key);
                if (Current.Value >= Addition.AreaRadius) continue;
                for (ARoundGridTile* Neighbor : GetNeighborFields(Current.Key))
                {
                    if (Neighbor && !Visited.Contains(Neighbor))
                    {
                        Visited.Add(Neighbor);
                        Frontier.Add({Neighbor, Current.Value + 1});
                    }
                }
            }
            if (ARoundAimFieldTriggerActor* Presentation = GetWorld()->SpawnActor<ARoundAimFieldTriggerActor>())
            {
                Presentation->bPresentationOnly = true;
                Presentation->CoreEffectId = Addition.EffectId;
                Presentation->InitializeTrigger(Addition.Character, TriggerTiles);
            }
        }
        if (Addition.CoreEffectKind != static_cast<int32>(roundcore::ModifierEffectKind::AimField)
            && (Addition.AreaCenterTile || Addition.AreaDamage > 0 || Addition.AreaInitiativeLossPerPulse > 0))
        {
            if (ARoundSanctifiedGroundAuraEffect* Effect = GetWorld()->SpawnActor<ARoundSanctifiedGroundAuraEffect>())
            {
                Effect->EffectId = FName(*Addition.EffectId);
                Effect->DisplayName = Ability->AbilityData ? Ability->AbilityData->DisplayName.ToString() : TEXT("Sanctify Ground");
                Effect->Description = Ability->AbilityData ? Ability->AbilityData->Description.ToString() : TEXT("Heilige Bodenzone.");
                Effect->Radius = Addition.AreaRadius;
                Effect->bAffectEnemies = true;
                Effect->bAffectAllies = false;
                Effect->bAffectSelf = false;
                Effect->bAuraUsesFixedPosition = !Addition.bAreaFollowsCauser;
                Effect->AuraFixedPosition = Addition.AreaCenterTile
                    ? Addition.AreaCenterTile->GridPosition
                    : Addition.Character->CurrentTile->GridPosition;
                Effect->AuraTriggerDamage = Addition.AreaDamage;
                Effect->AuraTriggerDamageType = Addition.AreaDamageType;
                Effect->InitiativeLossPerPulse = Addition.AreaInitiativeLossPerPulse;
                Effect->CauserCharacter = Addition.Causer;
                Effect->SetDurationInitiative(Addition.RemainingInitiative, false);
                Addition.Character->AddEffect(Effect);
            }
        }
        else if (Addition.RedirectPreMitigationBasisPoints > 0)
        {
            if (ARoundShieldOathEffect* Effect = GetWorld()->SpawnActor<ARoundShieldOathEffect>())
            {
                Effect->EffectId = FName(*Addition.EffectId);
                Effect->DisplayName = Ability->AbilityData ? Ability->AbilityData->DisplayName.ToString() : TEXT("Shield Oath");
                Effect->Description = TEXT("Leitet einen Teil des Schadens auf den Beschuetzer um.");
                Effect->RedirectPreMitigationFraction = Addition.RedirectPreMitigationBasisPoints / 10000.0;
                Effect->RedirectedDamageMultiplier = Addition.RedirectedDamageMultiplierBasisPoints / 10000.0;
                Effect->PaladinHealFraction = Addition.ProtectorHealFractionBasisPoints / 10000.0;
                Effect->ProtectedAllyInitiativePerHit = Addition.ProtectedInitiativePerHit;
                Effect->CauserCharacter = Addition.Causer;
                Effect->SetDurationInitiative(Addition.RemainingInitiative, false);
                Addition.Character->AddEffect(Effect);
            }
        }
        else if (Addition.bIsBind)
        {
            if (ARoundBindEffect* Effect = GetWorld()->SpawnActor<ARoundBindEffect>())
            {
                Effect->EffectId = FName(*Addition.EffectId);
                Effect->DisplayName = TEXT("Bind");
                Effect->Description = TEXT("Wird beim Bewegen ausgeloest.");
                Effect->TriggerDamage = Addition.TriggerDamage;
                Effect->DelayAmount = Addition.TriggerDelay;
                Effect->TriggerOutgoingDamageMultiplier = Addition.TriggerOutgoingDamageMultiplierBasisPoints / 10000.0;
                Effect->TriggerSlowAmount = FMath::Max(0.0, Addition.TriggerSlowMultiplierBasisPoints / 10000.0 - 1.0);
                Effect->TriggerSlowDuration = Addition.TriggerSlowDurationInitiative / 40;
                Effect->TriggerAoEInitiativeLossRadius = Addition.TriggerAreaRadius;
                Effect->TriggerAoEInitiativeLossAmount = Addition.TriggerAreaInitiativeLoss;
                Effect->TriggerInitiativeGainForCauser = Addition.TriggerInitiativeGainForCauser;
                Effect->bBlockMovementBonusesOnTarget = Addition.bBlockMovementBonuses;
                Effect->CauserCharacter = Addition.Causer;
                Effect->SetDurationInitiative(Addition.RemainingInitiative, false);
                Addition.Character->AddEffect(Effect);
            }
        }
        else if (Addition.bIsTaunt)
        {
            if (ARoundTauntEffect* Effect = GetWorld()->SpawnActor<ARoundTauntEffect>())
            {
                Effect->EffectId = FName(*Addition.EffectId);
                Effect->DisplayName = TEXT("Taunt");
                Effect->Description = TEXT("Angriffe gegen andere Ziele verursachen weniger Schaden.");
                Effect->Amount = Addition.OutgoingDamageMultiplierAgainstOtherTargetsBasisPoints / 10000.0;
                Effect->InitiativeLossWhenIgnoringCauser = Addition.InitiativeLossWhenIgnoringCauser;
                Effect->InitiativeGainForCauserWhenAttacked = Addition.InitiativeGainForCauserWhenAttacked;
                Effect->IncomingDamageMultiplier = Addition.IncomingDamageMultiplierBasisPoints / 10000.0;
                Effect->IncomingDamageMultiplierFromCauser = Addition.IncomingDamageMultiplierFromCauserBasisPoints / 10000.0;
                Effect->CauserCharacter = Addition.Causer;
                Effect->SetDurationInitiative(Addition.RemainingInitiative, false);
                Addition.Character->AddEffect(Effect);
            }
        }
        else if (Addition.MoveCostMultiplierBasisPoints != 10000 || Addition.InitiativeLossOnMove > 0)
        {
            if (ARoundSlowEffect* Effect = GetWorld()->SpawnActor<ARoundSlowEffect>())
            {
                Effect->EffectId = FName(*Addition.EffectId);
                Effect->DisplayName = TEXT("Shackle");
                Effect->Description = TEXT("Erhoeht Bewegungskosten.");
                Effect->Amount = FMath::Max(0.0, Addition.MoveCostMultiplierBasisPoints / 10000.0 - 1.0);
                Effect->InitiativeLossOnMove = Addition.InitiativeLossOnMove;
                Effect->InitiativeLossOnExpire = Addition.InitiativeLossOnExpire;
                Effect->bBlockMovementBonuses = Addition.bBlockMovementBonuses;
                Effect->IncomingDamageMultiplierFromCauser = Addition.IncomingDamageMultiplierFromCauserBasisPoints / 10000.0;
                Effect->CauserCharacter = Addition.Causer;
                Effect->SetDurationInitiative(Addition.RemainingInitiative, false);
                Addition.Character->AddEffect(Effect);
            }
        }
        else if (Addition.bPreventsAbilityUse)
        {
            if (ARoundSilenceEffect* Effect = GetWorld()->SpawnActor<ARoundSilenceEffect>())
            {
                Effect->EffectId = FName(*Addition.EffectId);
                Effect->DisplayName = Ability->AbilityData ? Ability->AbilityData->DisplayName.ToString() : TEXT("Silence");
                Effect->Description = TEXT("Kann keine Skills einsetzen.");
                Effect->CauserCharacter = Addition.Causer;
                Effect->SetDurationInitiative(Addition.RemainingInitiative, false);
                Addition.Character->AddEffect(Effect);
            }
        }
        else if (Addition.CoreEffectKind == static_cast<int32>(roundcore::ModifierEffectKind::Invisibility))
        {
            if (ARoundInvisibilityEffect* Effect = GetWorld()->SpawnActor<ARoundInvisibilityEffect>())
            {
                Effect->EffectId = FName(*Addition.EffectId);
                Effect->DisplayName = TEXT("Unsichtbar");
                Effect->Description = TEXT("Kann ohne Sichtaufdeckung nicht gesehen werden.");
                Effect->CauserCharacter = Addition.Causer;
                Effect->SetDurationInitiative(Addition.RemainingInitiative, false);
                Addition.Character->AddEffect(Effect);
            }
        }
        else if (ARoundModifierEffect* Effect = GetWorld()->SpawnActor<ARoundModifierEffect>())
        {
            Effect->EffectId = FName(*Addition.EffectId);
            Effect->DisplayName = Ability->AbilityData ? Ability->AbilityData->DisplayName.ToString() : TEXT("Modifier");
            Effect->Description = Ability->AbilityData ? Ability->AbilityData->Description.ToString() : TEXT("Core modifier effect.");
            Effect->IncomingDamageMultiplier = Addition.IncomingDamageMultiplierBasisPoints / 10000.0;
            Effect->OutgoingDamageMultiplier = Addition.OutgoingDamageMultiplierBasisPoints / 10000.0;
            Effect->DefenseIgnoreAdd = Addition.DefenseIgnoreAdd;
            Effect->HealFractionFromDamageDealt = Addition.HealFractionFromDamageDealtBasisPoints / 10000.0;
            Effect->MoveCostAdd = Addition.MoveCostAdd;
            Effect->IncomingEffectDurationAdd = Addition.IncomingEffectDurationAdd;
            Effect->bOnlyAffectCauserDamage = Addition.bOnlyAffectCauserDamage;
            Effect->CauserCharacter = Addition.Causer;
            Effect->bTreatAsBuff = !Addition.bIsDebuff;
            Effect->bTreatAsDebuff = Addition.bIsDebuff;
            Effect->SetDurationInitiative(Addition.RemainingInitiative, false);
            Addition.Character->AddEffect(Effect);
        }
        for (ARoundEffectActor* PresentedEffect : Addition.Character->ActiveEffects)
        {
            if (!PresentedEffect || PresentedEffect->EffectId.ToString() != Addition.EffectId) continue;
            PresentedEffect->StringState.FindOrAdd(TEXT("CoreSourceAbilityId")) = Addition.SourceAbilityId;
            PresentedEffect->StringState.FindOrAdd(TEXT("CorePreferredTarget")) = Addition.PreferredTarget
                ? Addition.PreferredTarget->GetPathName() : FString();
            PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreEffectKind")) = Addition.CoreEffectKind;
            PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreCounterA")) = Addition.CounterA;
            PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreCounterB")) = Addition.CounterB;
            PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreCounterC")) = Addition.CounterC;
            PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreValueA")) = Addition.ValueA;
            PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreValueB")) = Addition.ValueB;
            PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreValueC")) = Addition.ValueC;
            PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreFlags")) = Addition.Flags;
            break;
        }
    }

    for (const FRoundCoreBoardDamageEvent& Damage : Execution.BoardDamageEvents)
    {
        if (!Damage.Tile || Damage.Amount <= 0)
        {
            continue;
        }
        AActor* BoardActor = Damage.bTerrainObject
            ? Damage.Tile->TerrainObject.Get()
            : Damage.Tile->OccupyingFieldObject.Get();
        if (BoardActor && BoardActor->GetClass()->ImplementsInterface(URoundBoardDamageableInterface::StaticClass()))
        {
            IRoundBoardDamageableInterface::Execute_ApplyBoardDamage(
                BoardActor, Damage.Amount, Ability->GetAbilityDamageType(), 0, Character);
        }
    }
    for (const FRoundCoreDamageEvent& Damage : Execution.DamageEvents)
    {
        if (Damage.Target)
        {
            Damage.Target->ApplyCoreDamageOutcome(
                Damage.Source ? Damage.Source : Character,
                Damage.Amount,
                Ability->GetAbilityDamageType(),
                Damage.bTargetDied);
        }
    }
    for (const FRoundCoreHealingEvent& Healing : Execution.HealingEvents)
    {
        if (Healing.Character && Healing.Amount > 0)
        {
            RecordHeal(Healing.Character, Healing.Amount);
        }
    }
    if (Execution.TargetInitiativeLoss > 0)
    {
        RecordDelayApplied(TargetCharacter, Execution.TargetInitiativeLoss);
    }

    Result.bSuccess = true;
    Result.Reason = !Execution.AddedModifierEffects.IsEmpty()
        ? TEXT("Modifier ability executed by RoundCore.")
        : (Execution.AppliedDamage > 0
        ? FString::Printf(TEXT("Ability executed by RoundCore for %d damage."), Execution.AppliedDamage)
        : FString::Printf(TEXT("Ability executed by RoundCore for %d healing."), Execution.AppliedHealing));

    FString ResyncReason;
    if (!CoreMatchAdapter->Rebuild(*this, ResyncReason))
    {
        UE_LOG(LogTemp, Error, TEXT("RoundCore post-ability resync failed: %s"), *ResyncReason);
    }
    return Result;
}

bool ARoundGameState::IsCoreAbilityLegal(
    const ARoundCharacter* Character, const ARoundAbilityActor* Ability, const ARoundCharacter* TargetCharacter,
    const ARoundGridTile* TargetTile) const
{
    if (!Character || !Ability || (!TargetCharacter && !TargetTile))
    {
        return false;
    }
    if (!CoreMatchAdapter)
    {
        CoreMatchAdapter = MakeUnique<FRoundCoreMatchAdapter>();
    }
    FString CoreReason;
    return CoreMatchAdapter->Rebuild(*this, CoreReason)
        && CoreMatchAdapter->IsAbilityLegal(Character, Ability, TargetCharacter, TargetTile, CoreReason);
}

bool ARoundGameState::GetCoreVisionFields(
    const ARoundCharacter* Character,
    TArray<ARoundGridTile*>& OutVisibleFields) const
{
    OutVisibleFields.Reset();
    if (!Character)
    {
        return false;
    }
    // Character initialization can request visibility before SpawnCharacter
    // has registered the new Actor. It cannot be represented in a complete
    // Core snapshot yet; the caller temporarily shows only its current tile
    // and requests Core visibility again after registration.
    if (!RegisteredCharacters.Contains(Character))
    {
        return false;
    }
    if (!CoreMatchAdapter)
    {
        CoreMatchAdapter = MakeUnique<FRoundCoreMatchAdapter>();
    }
    FString CoreReason;
    if (!CoreMatchAdapter->Rebuild(*this, CoreReason)
        || !CoreMatchAdapter->GetVisibleTiles(Character, OutVisibleFields, CoreReason))
    {
        UE_LOG(LogTemp, Error, TEXT("RoundCore visibility failed for %s: %s"), *GetNameSafe(Character), *CoreReason);
        return false;
    }
    return true;
}

bool ARoundGameState::ExportCoreTrainingPackage(const FString& FilePath)
{
    if (!CoreMatchAdapter)
    {
        CoreMatchAdapter = MakeUnique<FRoundCoreMatchAdapter>();
    }
    FString CoreReason;
    if (!CoreMatchAdapter->Rebuild(*this, CoreReason))
    {
        UE_LOG(LogTemp, Error, TEXT("RoundCore training package export failed: %s"), *CoreReason);
        return false;
    }

    const FString ResolvedPath = FilePath.IsEmpty()
        ? FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Training"), TEXT("roundcore_match.rndcore"))
        : FPaths::ConvertRelativePathToFull(FilePath);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(ResolvedPath), true);
    try
    {
        roundcore::MatchSetup Setup;
        Setup.definitions = CoreMatchAdapter->GetGame().Definitions();
        Setup.initialState = CoreMatchAdapter->GetGame().State();
        const std::vector<std::uint8_t> Serialized = roundcore::SerializeMatchSetup(Setup);
        TArray<uint8> Bytes;
        Bytes.Append(Serialized.data(), static_cast<int32>(Serialized.size()));
        if (!FFileHelper::SaveArrayToFile(Bytes, *ResolvedPath))
        {
            UE_LOG(LogTemp, Error, TEXT("Could not write RoundCore training package to %s."), *ResolvedPath);
            return false;
        }
    }
    catch (const std::exception& Error)
    {
        UE_LOG(LogTemp, Error, TEXT("RoundCore training package serialization failed: %s"), UTF8_TO_TCHAR(Error.what()));
        return false;
    }
    UE_LOG(LogTemp, Display, TEXT("Exported RoundCore training package to %s."), *ResolvedPath);
    return true;
}

bool ARoundGameState::IsSimMoveLegal(const ARoundCharacter* Character, const ARoundGridTile* TargetTile) const
{
    return IsCoreMoveLegal(Character, TargetTile);
}

FRoundActionResult ARoundGameState::ExecuteCoreMove(ARoundCharacter* Character, ARoundGridTile* TargetTile)
{
    return ExecuteCoreMovePath(Character, {TargetTile});
}

FRoundActionResult ARoundGameState::ExecuteCoreMovePath(ARoundCharacter* Character, const TArray<ARoundGridTile*>& Path)
{
    FRoundActionResult Result;
    if (!HasAuthority() || !Character || Path.IsEmpty() || !Character->Attributes || !Character->CurrentTile)
    {
        Result.Reason = TEXT("Invalid Core movement context.");
        return Result;
    }

    if (!CoreMatchAdapter)
    {
        CoreMatchAdapter = MakeUnique<FRoundCoreMatchAdapter>();
    }

    FString CoreReason;
    if (!CoreMatchAdapter->Rebuild(*this, CoreReason))
    {
        Result.Reason = FString::Printf(TEXT("Core snapshot failed: %s"), *CoreReason);
        return Result;
    }

    const FVector2D StartPosition = Character->CurrentTile->GridPosition;
    TArray<FVector2D> TraversedPositions;
    int32 TotalMovementCost = 0;

    for (int32 Index = 0; Index < Path.Num(); ++Index)
    {
        ARoundGridTile* TargetTile = Path[Index];
        if (!TargetTile)
        {
            CoreReason = TEXT("Movement path contains an invalid tile.");
            break;
        }

        FRoundCoreMoveExecution MoveExecution;
        if (!CoreMatchAdapter->ExecuteMove(Character, TargetTile, MoveExecution, CoreReason))
        {
            break;
        }

        // Apply the migrated result first. SetField remains the presentation
        // bridge for effects/terrain that have not moved to Core yet.
        for (const FRoundCoreCharacterRuntimeUpdate& Update : MoveExecution.CharacterUpdates)
        {
            if (Update.Character && Update.Character->Attributes)
            {
                Update.Character->Attributes->SetCurrentHealthValue(Update.Health);
                Update.Character->Attributes->SetCurrentInitiativeValue(Update.Initiative);
            }
            ApplyCharacter2CoreState(Update);
            ApplyCharacter3CoreState(Update);
            ApplyCharacter4CoreState(Update);
            ApplyChronomancerCoreState(Update);
            if (ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(Update.Character))
                Character5->ApplyCoreBloodState(Update.ResourceCurrent, Update.SpecialCooldownRemaining,
                    Update.bFirstBloodDamageReductionAvailable, Update.bBasicAttackBloodAvailable, Update.bBloodCapTriggerAvailable);
        }
        bApplyingCoreMovementCallbacks = true;
        Character->SetField(TargetTile, Index == Path.Num() - 1);
        bApplyingCoreMovementCallbacks = false;
        for (const FRoundCoreEffectRemoval& Removal : MoveExecution.RemovedEffects)
        {
            if (!Removal.Character) continue;
            RemoveCoreAimPresentation(Removal.Character, Removal.EffectId);
            const TArray<TObjectPtr<ARoundEffectActor>> Effects = Removal.Character->ActiveEffects;
            for (ARoundEffectActor* Effect : Effects)
            {
                if (Effect && Effect->EffectId.ToString() == Removal.EffectId) { Effect->RemoveEffect(); break; }
            }
        }
        for (const FRoundCoreModifierEffectAddition& Addition : MoveExecution.AddedModifierEffects)
        {
            if (!Addition.Character || !GetWorld()) continue;
            if (Addition.MoveCostMultiplierBasisPoints != 10000)
            {
                if (ARoundSlowEffect* Effect = GetWorld()->SpawnActor<ARoundSlowEffect>())
                {
                    Effect->EffectId = FName(*Addition.EffectId); Effect->DisplayName = TEXT("Bind");
                    Effect->Amount = FMath::Max(0.0, Addition.MoveCostMultiplierBasisPoints / 10000.0 - 1.0);
                    Effect->CauserCharacter = Addition.Causer; Effect->SetDurationInitiative(Addition.RemainingInitiative, false);
                    Addition.Character->AddEffect(Effect);
                }
            }
            else if (ARoundModifierEffect* Effect = GetWorld()->SpawnActor<ARoundModifierEffect>())
            {
                Effect->EffectId = FName(*Addition.EffectId); Effect->DisplayName = TEXT("Bind");
                Effect->OutgoingDamageMultiplier = Addition.OutgoingDamageMultiplierBasisPoints / 10000.0;
                Effect->bTreatAsDebuff = true; Effect->CauserCharacter = Addition.Causer;
                Effect->SetDurationInitiative(Addition.RemainingInitiative, false); Addition.Character->AddEffect(Effect);
            }
            for (ARoundEffectActor* PresentedEffect : Addition.Character->ActiveEffects)
            {
                if (!PresentedEffect || PresentedEffect->EffectId.ToString() != Addition.EffectId) continue;
                PresentedEffect->StringState.FindOrAdd(TEXT("CoreSourceAbilityId")) = Addition.SourceAbilityId;
                PresentedEffect->StringState.FindOrAdd(TEXT("CorePreferredTarget")) = Addition.PreferredTarget
                    ? Addition.PreferredTarget->GetPathName() : FString();
                PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreEffectKind")) = Addition.CoreEffectKind;
                PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreCounterA")) = Addition.CounterA;
                PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreCounterB")) = Addition.CounterB;
                PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreCounterC")) = Addition.CounterC;
                PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreValueA")) = Addition.ValueA;
                PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreValueB")) = Addition.ValueB;
                PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreValueC")) = Addition.ValueC;
                PresentedEffect->IntegerState.FindOrAdd(TEXT("CoreFlags")) = Addition.Flags;
                break;
            }
        }
        for (const FRoundCoreDamageEvent& Damage : MoveExecution.DamageEvents)
        {
            if (Damage.Target) Damage.Target->ApplyCoreDamageOutcome(Damage.Source, Damage.Amount, ERoundDamageType::Physical, Damage.bTargetDied);
        }
        TraversedPositions.Add(TargetTile->GridPosition);
        TotalMovementCost += MoveExecution.MovementCost;

        FString ResyncReason;
        if (!CoreMatchAdapter->Rebuild(*this, ResyncReason))
        {
            UE_LOG(LogTemp, Error, TEXT("RoundCore post-move resync failed: %s"), *ResyncReason);
            CoreReason = ResyncReason;
            break;
        }
        if (TargetTile->TerrainStopsMovement())
        {
            break;
        }
    }

    if (TraversedPositions.IsEmpty())
    {
        Result.Reason = CoreReason.IsEmpty() ? TEXT("Tile is not reachable.") : CoreReason;
        return Result;
    }

    RecordMovement(Character, StartPosition, TraversedPositions, TotalMovementCost);
    Result.bSuccess = true;
    Result.Reason = TraversedPositions.Num() == Path.Num()
        ? TEXT("Movement executed by RoundCore.")
        : FString::Printf(TEXT("Movement interrupted by RoundCore: %s"), *CoreReason);
    return Result;
}

ARoundCharacter* ARoundGameState::AdvanceCoreTurn(ARoundCharacter* EndingCharacter)
{
    if (!HasAuthority() || !EndingCharacter || CurrentTurnCharacter != EndingCharacter)
    {
        UE_LOG(LogTemp, Warning, TEXT("RoundCore turn advance rejected invalid ending character %s."), *GetNameSafe(EndingCharacter));
        return nullptr;
    }

    if (!CoreMatchAdapter)
    {
        CoreMatchAdapter = MakeUnique<FRoundCoreMatchAdapter>();
    }

    FString CoreReason;
    if (!CoreMatchAdapter->Rebuild(*this, CoreReason))
    {
        UE_LOG(LogTemp, Error, TEXT("RoundCore turn snapshot failed: %s"), *CoreReason);
        return nullptr;
    }

    const ARoundCharacter* CoreEndingCharacter = EndingCharacter;
    for (int32 Safety = 0; Safety < 10000; ++Safety)
    {
        FRoundCoreTurnExecution Execution;
        if (!CoreMatchAdapter->ExecuteTurnStep(
                CoreEndingCharacter,
                Execution,
                CoreReason))
        {
            UE_LOG(LogTemp, Error, TEXT("RoundCore turn step failed: %s"), *CoreReason);
            CurrentTurnCharacter = nullptr;
            OnTurnChanged.Broadcast(nullptr);
            OnBoardStateChanged.Broadcast();
            return nullptr;
        }

        CoreEndingCharacter = nullptr;
        CurrentTurnCharacter = nullptr;
        for (const FRoundCoreCharacterRuntimeUpdate& Update : Execution.CharacterUpdates)
        {
            if (Update.Character && Update.Tile && Update.Character->CurrentTile != Update.Tile)
            {
                bApplyingCoreMovementCallbacks = true;
                Update.Character->SetField(Update.Tile, false);
                bApplyingCoreMovementCallbacks = false;
            }
            if (Update.Character && Update.Character->Attributes)
            {
                Update.Character->Attributes->SetCurrentHealthValue(Update.Health);
                Update.Character->Attributes->SetCurrentInitiativeValue(Update.Initiative);
            }
            if (ARoundCharacter1* Character1 = Cast<ARoundCharacter1>(Update.Character))
            {
                Character1->NativeChainCount = Update.ResourceCurrent;
                Character1->NativeChainRechargeProgress = Update.ResourceRechargeProgress;
                Character1->NativeDamageRechargeCooldownRemaining = Update.SpecialCooldownRemaining;
                Character1->bGrantedChainSpendRefundThisTurn = !Update.bFirstResourceSpendRefundAvailable;
                Character1->bMovedThisTurn = Update.bMovedSinceLastTurn;
                Character1->bStationaryGuardActive = Update.bStationaryGuardActive;
            }
            ApplyCharacter2CoreState(Update);
            ApplyCharacter3CoreState(Update);
            ApplyCharacter4CoreState(Update);
            ApplyChronomancerCoreState(Update);
            if (ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(Update.Character))
                Character5->ApplyCoreBloodState(Update.ResourceCurrent, Update.SpecialCooldownRemaining,
                    Update.bFirstBloodDamageReductionAvailable, Update.bBasicAttackBloodAvailable, Update.bBloodCapTriggerAvailable);
        }
        for (const FRoundCoreAbilityRuntimeUpdate& Update : Execution.AbilityUpdates)
        {
            if (Update.Ability)
            {
                Update.Ability->SetCooldown(Update.Cooldown);
                Update.Ability->CurrentCharges = Update.Charges;
                if (ARoundSnipeAbility* Snipe = Cast<ARoundSnipeAbility>(Update.Ability)) Snipe->ReadyInitiativeElapsed = Update.ReadyInitiativeElapsed;
            }
        }

        for (const FRoundCoreDamageEvent& Damage : Execution.DamageEvents)
        {
            if (Damage.Target)
            {
                Damage.Target->ApplyCoreDamageOutcome(
                    Damage.Source,
                    Damage.Amount,
                    Damage.DamageType,
                    Damage.bTargetDied);
            }
        }
        for (const FRoundCoreHealingEvent& Healing : Execution.HealingEvents)
        {
            if (Healing.Character && Healing.Amount > 0)
            {
                RecordHeal(Healing.Character, Healing.Amount);
            }
        }
        for (const TPair<ARoundCharacter*, int32>& Delay : Execution.AppliedDelays)
        {
            if (Delay.Key && Delay.Value > 0) RecordDelayApplied(Delay.Key, Delay.Value);
        }
        for (const FRoundCoreEffectRemoval& Removal : Execution.RemovedEffects)
        {
            if (!Removal.Character) continue;
            if (Removal.EffectId.Contains(TEXT(":BearForm")))
                if (ARoundCharacterDruid* Druid = Cast<ARoundCharacterDruid>(Removal.Character)) Druid->ExitBearForm();
            RemoveCoreAimPresentation(Removal.Character, Removal.EffectId);
            const TArray<TObjectPtr<ARoundEffectActor>> EffectsSnapshot = Removal.Character->ActiveEffects;
            for (ARoundEffectActor* Effect : EffectsSnapshot)
            {
                if (Effect && Effect->EffectId.ToString() == Removal.EffectId)
                {
                    Effect->RemoveEffect();
                    break;
                }
            }
        }
        for (const FRoundCoreModifierEffectAddition& Addition : Execution.AddedModifierEffects)
        {
            PresentCoreGenericModifier(GetWorld(), Addition, TEXT("Geraeteeffekt"),
                TEXT("Dieser Effekt wird von RoundCore verwaltet."));
        }

        if (Execution.GlobalInitiativeAdvance > 0)
        {
            bApplyingCoreInitiativeAdvance = true;
            for (ARoundCharacter* Character : RegisteredCharacters)
            {
                if (!Character || !Character->IsAlive() || !Character->Attributes)
                {
                    continue;
                }

                Character->AdvanceGlobalInitiative(Execution.GlobalInitiativeAdvance);
            }
            bApplyingCoreInitiativeAdvance = false;

            for (ARoundGridTile* Field : Fields)
            {
                if (ARoundTerrainActor* TerrainActor = Field ? Cast<ARoundTerrainActor>(Field->TerrainObject) : nullptr)
                {
                    TerrainActor->AdvanceTerrainInitiative(Execution.GlobalInitiativeAdvance);
                }
            }

            MatchGlobalInitiativeElapsed += Execution.GlobalInitiativeAdvance;
            AdvanceObjectivesByInitiative(Execution.GlobalInitiativeAdvance);

            if (!CoreMatchAdapter->Rebuild(*this, CoreReason))
            {
                UE_LOG(LogTemp, Error, TEXT("RoundCore initiative resync failed: %s"), *CoreReason);
                return nullptr;
            }
            continue;
        }

        ARoundCharacter* NextCharacter = Execution.ActiveCharacter;
        if (!NextCharacter)
        {
            UE_LOG(LogTemp, Error, TEXT("RoundCore turn step produced neither initiative progress nor an active character."));
            return nullptr;
        }

        CurrentTurnCharacter = NextCharacter;
        ++MatchTurnCounter;
        NextCharacter->BeginTurnFromCore();
        RecordTurnStarted(NextCharacter);

        if (!CoreMatchAdapter->Rebuild(*this, CoreReason))
        {
            UE_LOG(LogTemp, Error, TEXT("RoundCore turn-start resync failed: %s"), *CoreReason);
        }

        RebuildTurnOrder();
        OnTurnChanged.Broadcast(CurrentTurnCharacter);
        OnBoardStateChanged.Broadcast();
        return CurrentTurnCharacter;
    }

    UE_LOG(LogTemp, Error, TEXT("RoundCore turn advance exceeded the safety limit."));
    CurrentTurnCharacter = nullptr;
    OnTurnChanged.Broadcast(nullptr);
    OnBoardStateChanged.Broadcast();
    return nullptr;
}

bool ARoundGameState::IsSimAttackLegal(const ARoundCharacter* Character, const ARoundGridTile* TargetTile) const
{
    return Character
        && TargetTile
        && Character->CurrentTile
        && Character->CurrentTile->GetDistance(TargetTile) <= Character->GetAttackRange()
        && Character->EnemyField(const_cast<ARoundGridTile*>(TargetTile));
}

bool ARoundGameState::IsSimAbilityLegal(const ARoundCharacter* Character, const int32 AbilityIndex, const ARoundGridTile* TargetTile) const
{
    return Character
        && Character->GrantedAbilities.IsValidIndex(AbilityIndex)
        && Character->GrantedAbilities[AbilityIndex]
        && Character->GrantedAbilities[AbilityIndex]->CanTargetTile(TargetTile);
}

bool ARoundGameState::ShouldUseSimMoveValidation() const
{
    return false;
}

bool ARoundGameState::ShouldUseSimAttackValidation() const
{
    return false;
}

bool ARoundGameState::ShouldUseSimAbilityValidation() const
{
    return false;
}

bool ARoundGameState::ShouldRunSimShadowValidation() const
{
    return false;
}

bool ARoundGameState::ShouldLogSimProfiling() const
{
    return false;
}

bool ARoundGameState::ShouldUseSimSimpleReconciliation() const
{
    return false;
}

bool ARoundGameState::ShouldUseSimAuthoritativeSimple() const
{
    return false;
}

bool ARoundGameState::ShouldUseSimAuthoritativeMove() const
{
    return false;
}

bool ARoundGameState::ShouldUseSimAuthoritativeAttack() const
{
    return false;
}

bool ARoundGameState::ShouldUseSimAuthoritativeAbility() const
{
    return false;
}

bool ARoundGameState::ShouldUseSimAuthoritativePuppetAbility() const
{
    return false;
}

bool ARoundGameState::ShouldExecuteSimAttackResult() const
{
    return false;
}

bool ARoundGameState::ShouldExecuteSimAbilityResult() const
{
    return false;
}

bool ARoundGameState::ExecutePythonLiveStep(const FString& PolicyName, FString& OutReason, float* OutNextAutoplayDelaySeconds)
{
    bPythonLiveAutoplayEnabled = false;
    PythonLiveAutoplayPolicyName.Reset();
    if (OutNextAutoplayDelaySeconds)
    {
        *OutNextAutoplayDelaySeconds = 0.0f;
    }

    OutReason = FString::Printf(TEXT("Live Python step is no longer available after removing the sim backend. Policy=%s"), *PolicyName);
    return false;
}

void ARoundGameState::StartPythonLiveAutoplay(const FString& PolicyName, const float InitialDelaySeconds)
{
    bPythonLiveAutoplayEnabled = false;
    PythonLiveAutoplayPolicyName.Reset();
    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(PythonLiveAutoplayTimerHandle);
    }

    UE_LOG(LogTemp, Warning, TEXT("StartPythonLiveAutoplay ignored because sim-backed autoplay was removed. Policy=%s Delay=%.2f"), *PolicyName, InitialDelaySeconds);
}

void ARoundGameState::StopPythonLiveAutoplay()
{
    bPythonLiveAutoplayEnabled = false;
    PythonLiveAutoplayPolicyName.Reset();
    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(PythonLiveAutoplayTimerHandle);
    }
}

void ARoundGameState::InitializeMatchRecording()
{
    if (!HasAuthority())
    {
        return;
    }

    CurrentMatchRecord = FRoundMatchRecord();
    CharacterRecordIds.Reset();
    MatchEventSequence = 0;
    MatchTurnCounter = 0;
    MatchGlobalInitiativeElapsed = 0;
    ResetAiKnowledge();
    InitializeMatchRandomSeed();
    bMatchRecordingFinalized = false;

    CurrentMatchRecord.MatchId = BuildMatchRecordId();
    CurrentMatchRecord.StartedAtUtc = BuildIsoUtcTimestamp();
    CurrentMatchRecord.FieldSize = Fields.Num();
    CurrentMatchRecord.TerrainAmount = InitialTerrainAmount;
    CurrentMatchRecord.SkillAmount = InitialSkillAmount;

    if (const URoundGameInstance* RoundGameInstance = GetGameInstance<URoundGameInstance>())
    {
        const FRoundTeamDefinition* TeamOneDefinition = nullptr;
        const FRoundTeamDefinition* TeamTwoDefinition = nullptr;

        if (RoundGameInstance->LoadedTeams.IsValidIndex(0))
        {
            TeamOneDefinition = &RoundGameInstance->LoadedTeams[0];
        }
        else
        {
            TeamOneDefinition = &RoundGameInstance->StrTeam;
        }

        if (RoundGameInstance->LoadedTeams.IsValidIndex(1))
        {
            TeamTwoDefinition = &RoundGameInstance->LoadedTeams[1];
        }

        if (TeamOneDefinition)
        {
            CurrentMatchRecord.Teams.Add(*TeamOneDefinition);
            CurrentMatchRecord.Format = TeamOneDefinition->Format;

            FRoundFormatDefinition FormatDefinition;
            if (RoundGameInstance->TryGetFormatDefinition(TeamOneDefinition->Format, FormatDefinition))
            {
                CurrentMatchRecord.FormatName = FormatDefinition.Name;
                CurrentMatchRecord.FieldSize = FormatDefinition.FieldSize;
                CurrentMatchRecord.TerrainAmount = FormatDefinition.Terrain;
                CurrentMatchRecord.SkillAmount = FormatDefinition.SkillAmount;
            }
        }

        if (TeamTwoDefinition)
        {
            CurrentMatchRecord.Teams.Add(*TeamTwoDefinition);
        }
    }

    for (ARoundGridTile* Field : Fields)
    {
        if (!Field)
        {
            continue;
        }

        FRoundMatchFieldSnapshot Snapshot;
        Snapshot.Position = Field->GridPosition;
        Snapshot.TerrainType = Field->TerrainType;
        CurrentMatchRecord.Fields.Add(MoveTemp(Snapshot));
    }
}

void ARoundGameState::ResetAiKnowledge()
{
    // AI knowledge was sim-derived; with actor-authoritative gameplay there is nothing to cache here.
}

FString ARoundGameState::ResolveCharacterRecordId(const ARoundCharacter* Character) const
{
    if (!Character)
    {
        return FString();
    }

    if (const FString* ExistingId = CharacterRecordIds.Find(Character))
    {
        return *ExistingId;
    }

    return Character->GetName();
}

void ARoundGameState::AppendMatchEvent(const FRoundMatchEventRecord& EventRecord)
{
    if (!HasAuthority() || bMatchRecordingFinalized)
    {
        return;
    }

    CurrentMatchRecord.Events.Add(EventRecord);
    MulticastEnqueuePresentationEvent(BuildPresentationEvent(EventRecord));
}

FRoundPresentationEventRecord ARoundGameState::BuildPresentationEvent(const FRoundMatchEventRecord& EventRecord) const
{
    FRoundPresentationEventRecord PresentationEvent;
    PresentationEvent.EventType = EventRecord.EventType;
    PresentationEvent.SourceCharacterId = EventRecord.SourceCharacterId;
    PresentationEvent.TargetCharacterId = EventRecord.TargetCharacterId;
    PresentationEvent.AbilityId = EventRecord.AbilityId;
    PresentationEvent.TilePosition = EventRecord.TilePosition;
    PresentationEvent.SecondaryTilePosition = EventRecord.SecondaryTilePosition;
    PresentationEvent.Amount = static_cast<float>(EventRecord.Amount);
    PresentationEvent.bSuccess = EventRecord.bSuccess;
    if (const ARoundCharacter* SourceCharacter = FindLiveCharacterByRecordId(EventRecord.SourceCharacterId))
    {
        PresentationEvent.SourceTeam = SourceCharacter->Team;
    }
    if (const ARoundCharacter* TargetCharacter = FindLiveCharacterByRecordId(EventRecord.TargetCharacterId))
    {
        PresentationEvent.TargetTeam = TargetCharacter->Team;
    }
    return PresentationEvent;
}

bool ARoundGameState::ShouldQueuePresentationEvent(const FRoundPresentationEventRecord& EventRecord) const
{
    return EventRecord.EventType == TEXT("turn_started")
        || EventRecord.EventType == TEXT("turn_ended")
        || EventRecord.EventType == TEXT("move")
        || EventRecord.EventType == TEXT("basic_attack")
        || EventRecord.EventType == TEXT("ability_use")
        || EventRecord.EventType == TEXT("damage")
        || EventRecord.EventType == TEXT("heal")
        || EventRecord.EventType == TEXT("initiative_loss")
        || EventRecord.EventType == TEXT("death");
}

void ARoundGameState::MulticastEnqueuePresentationEvent_Implementation(const FRoundPresentationEventRecord& EventRecord)
{
    if (bReplayActive)
    {
        return;
    }

    EnqueuePresentationEvent(EventRecord);
}

void ARoundGameState::EnqueuePresentationEvent(const FRoundPresentationEventRecord& EventRecord)
{
    if (!ShouldQueuePresentationEvent(EventRecord))
    {
        return;
    }

    PendingPresentationEvents.Add(EventRecord);
    TryStartNextPresentationEvent();
}

ARoundCharacter* ARoundGameState::FindLiveCharacterByRecordId(const FString& CharacterId) const
{
    if (CharacterId.IsEmpty())
    {
        return nullptr;
    }

    for (ARoundCharacter* Character : RegisteredCharacters)
    {
        if (!Character)
        {
            continue;
        }

        if (ResolveCharacterRecordId(Character) == CharacterId || Character->GetName() == CharacterId)
        {
            return Character;
        }
    }

    return nullptr;
}

FVector ARoundGameState::ResolveTileWorldLocation(const FVector2D& TilePosition) const
{
    if (const ARoundGridTile* Tile = GetField(TilePosition))
    {
        return Tile->GetActorLocation();
    }

    return FVector::ZeroVector;
}

void ARoundGameState::InitializePresentationProfiles()
{
    PresentationProfilesByEvent.Reset();
    PresentationProfilesByAbility.Reset();

    FRoundPresentationProfile MoveProfile;
    MoveProfile.TextColor = FLinearColor(0.72f, 0.88f, 1.0f, 1.0f);
    MoveProfile.BeamThicknessScale = 0.10f;
    MoveProfile.SoundVolumeMultiplier = 0.26f;
    MoveProfile.SoundPitchMultiplier = 1.05f;
    MoveProfile.SoundMaxDurationSeconds = 0.07f;
    PresentationProfilesByEvent.Add(TEXT("move"), MoveProfile);

    FRoundPresentationProfile AttackProfile;
    AttackProfile.TextColor = FLinearColor(1.0f, 0.8f, 0.35f, 1.0f);
    AttackProfile.TextOverride = TEXT("ATK");
    AttackProfile.BeamThicknessScale = 0.14f;
    AttackProfile.SoundVolumeMultiplier = 0.48f;
    AttackProfile.SoundPitchMultiplier = 1.00f;
    AttackProfile.SoundMaxDurationSeconds = 0.14f;
    PresentationProfilesByEvent.Add(TEXT("basic_attack"), AttackProfile);

    FRoundPresentationProfile AbilityProfile;
    AbilityProfile.TextColor = FLinearColor(0.55f, 0.85f, 1.0f, 1.0f);
    AbilityProfile.BeamThicknessScale = 0.20f;
    AbilityProfile.SoundVolumeMultiplier = 0.55f;
    AbilityProfile.SoundPitchMultiplier = 0.95f;
    AbilityProfile.SoundMaxDurationSeconds = 0.18f;
    PresentationProfilesByEvent.Add(TEXT("ability_use"), AbilityProfile);

    FRoundPresentationProfile DamageProfile;
    DamageProfile.TextColor = FLinearColor(1.0f, 0.25f, 0.25f, 1.0f);
    DamageProfile.TextHeightOffset = 110.0f;
    DamageProfile.PulseStartScale = FVector(0.18f, 0.18f, 0.18f);
    DamageProfile.PulseEndScale = FVector(0.44f, 0.44f, 0.44f);
    DamageProfile.SoundVolumeMultiplier = 0.52f;
    DamageProfile.SoundPitchMultiplier = 0.92f;
    DamageProfile.SoundMaxDurationSeconds = 0.11f;
    DamageProfile.ScalePunch = RoundPresentationDamageScalePunch;
    PresentationProfilesByEvent.Add(TEXT("damage"), DamageProfile);

    FRoundPresentationProfile HealProfile;
    HealProfile.TextColor = FLinearColor(0.35f, 1.0f, 0.45f, 1.0f);
    HealProfile.TextHeightOffset = 110.0f;
    HealProfile.PulseStartScale = FVector(0.18f, 0.18f, 0.18f);
    HealProfile.PulseEndScale = FVector(0.40f, 0.40f, 0.40f);
    HealProfile.SoundVolumeMultiplier = 0.42f;
    HealProfile.SoundPitchMultiplier = 1.15f;
    HealProfile.SoundMaxDurationSeconds = 0.14f;
    HealProfile.ScalePunch = RoundPresentationHealScalePunch;
    PresentationProfilesByEvent.Add(TEXT("heal"), HealProfile);

    FRoundPresentationProfile InitiativeProfile;
    InitiativeProfile.TextColor = FLinearColor(1.0f, 0.85f, 0.25f, 1.0f);
    InitiativeProfile.TextHeightOffset = 95.0f;
    InitiativeProfile.SoundVolumeMultiplier = 0.45f;
    InitiativeProfile.SoundPitchMultiplier = 0.80f;
    InitiativeProfile.SoundMaxDurationSeconds = 0.10f;
    InitiativeProfile.ScalePunch = 0.04f;
    PresentationProfilesByEvent.Add(TEXT("initiative_loss"), InitiativeProfile);

    FRoundPresentationProfile TurnProfile;
    TurnProfile.TextColor = FLinearColor(0.95f, 0.95f, 0.55f, 1.0f);
    TurnProfile.TextHeightOffset = 135.0f;
    TurnProfile.TextOverride = TEXT("TURN");
    TurnProfile.PulseStartScale = FVector(0.28f, 0.28f, 0.05f);
    TurnProfile.PulseEndScale = FVector(0.80f, 0.80f, 0.08f);
    TurnProfile.SoundVolumeMultiplier = 0.40f;
    TurnProfile.SoundPitchMultiplier = 1.20f;
    TurnProfile.SoundMaxDurationSeconds = 0.10f;
    PresentationProfilesByEvent.Add(TEXT("turn_started"), TurnProfile);

    FRoundPresentationProfile DeathProfile;
    DeathProfile.SoundVolumeMultiplier = 0.68f;
    DeathProfile.SoundPitchMultiplier = 0.78f;
    DeathProfile.SoundMaxDurationSeconds = 0.18f;
    PresentationProfilesByEvent.Add(TEXT("death"), DeathProfile);

    FRoundPresentationProfile IceProfile = AbilityProfile;
    IceProfile.TextColor = FLinearColor(0.62f, 0.92f, 1.0f, 1.0f);
    IceProfile.BeamThicknessScale = 0.24f;
    IceProfile.SoundPitchMultiplier = 1.05f;
    PresentationProfilesByAbility.Add(TEXT("Icebeam"), IceProfile);

    FRoundPresentationProfile PuppetProfile = AbilityProfile;
    PuppetProfile.TextColor = FLinearColor(1.0f, 0.62f, 0.82f, 1.0f);
    PuppetProfile.BeamThicknessScale = 0.22f;
    PuppetProfile.SoundPitchMultiplier = 0.90f;
    PresentationProfilesByAbility.Add(TEXT("PuppetHit"), PuppetProfile);

    FRoundPresentationProfile SmokeProfile = AbilityProfile;
    SmokeProfile.TextColor = FLinearColor(0.82f, 0.82f, 0.82f, 1.0f);
    SmokeProfile.BeamThicknessScale = 0.16f;
    SmokeProfile.SoundVolumeMultiplier = 0.34f;
    SmokeProfile.SoundPitchMultiplier = 0.85f;
    PresentationProfilesByAbility.Add(TEXT("SmokeStep"), SmokeProfile);
}

FRoundPresentationProfile ARoundGameState::ResolvePresentationProfile(const FRoundPresentationEventRecord& EventRecord) const
{
    FRoundPresentationProfile ResolvedProfile;
    if (const FRoundPresentationProfile* EventProfile = PresentationProfilesByEvent.Find(EventRecord.EventType))
    {
        ResolvedProfile = *EventProfile;
    }

    const ARoundCharacter* SourceCharacter = FindLiveCharacterByRecordId(EventRecord.SourceCharacterId);

    if (EventRecord.EventType == TEXT("ability_use"))
    {
        if (SourceCharacter)
        {
            for (const ARoundAbilityActor* AbilityActor : SourceCharacter->GrantedAbilities)
            {
                if (!AbilityActor || !AbilityActor->AbilityData)
                {
                    continue;
                }

                const FName LiveAbilityId = AbilityActor->AbilityData->AbilityId.IsNone()
                    ? AbilityActor->RowName
                    : AbilityActor->AbilityData->AbilityId;
                if (LiveAbilityId.ToString().Equals(EventRecord.AbilityId, ESearchCase::IgnoreCase))
                {
                    ApplyPresentationProfileDataAsset(AbilityActor->AbilityData->PresentationProfile, ResolvedProfile);
                    break;
                }
            }
        }

        if (const FRoundPresentationProfile* AbilityProfile = PresentationProfilesByAbility.Find(EventRecord.AbilityId))
        {
            ResolvedProfile = *AbilityProfile;
            if (ResolvedProfile.TextOverride.IsEmpty())
            {
                ResolvedProfile.TextOverride = EventRecord.AbilityId;
            }
        }
        else if (ResolvedProfile.TextOverride.IsEmpty())
        {
            ResolvedProfile.TextOverride = EventRecord.AbilityId;
        }
    }
    else if (SourceCharacter && SourceCharacter->CharacterData)
    {
        if (EventRecord.EventType == TEXT("move"))
        {
            ApplyPresentationProfileDataAsset(SourceCharacter->CharacterData->MovePresentationProfile, ResolvedProfile);
        }
        else if (EventRecord.EventType == TEXT("basic_attack"))
        {
            ApplyPresentationProfileDataAsset(SourceCharacter->CharacterData->BasicAttackPresentationProfile, ResolvedProfile);
        }
    }

    return ResolvedProfile;
}

void ARoundGameState::TryStartNextPresentationEvent()
{
    if (bPresentationEventActive || PendingPresentationEvents.Num() <= 0)
    {
        return;
    }

    ActivePresentationEvent = PendingPresentationEvents[0];
    PendingPresentationEvents.RemoveAt(0);
    ActivePresentationElapsedSeconds = 0.0f;
    ActivePresentationDurationSeconds = 0.0f;
    ActivePresentationSourceCharacter = nullptr;
    ActivePresentationTargetCharacter = nullptr;
    ActivePresentationStartLocation = FVector::ZeroVector;
    ActivePresentationEndLocation = FVector::ZeroVector;
    ActivePresentationSourceBaseScale = FVector::OneVector;
    ActivePresentationTargetBaseScale = FVector::OneVector;
    ActivePresentationProfile = ResolvePresentationProfile(ActivePresentationEvent);
    bPresentationEventActive = true;

    if (ActivePresentationEvent.EventType == TEXT("turn_started"))
    {
        if (HasAuthority())
        {
            DispatchPresentationSoundToPlayers(
                ActivePresentationEvent,
                ActivePresentationProfile.SoundVolumeMultiplier,
                ActivePresentationProfile.SoundPitchMultiplier,
                ActivePresentationProfile.SoundMaxDurationSeconds);
        }
        ActivePresentationSourceCharacter = FindLiveCharacterByRecordId(ActivePresentationEvent.SourceCharacterId);
        if (ARoundCharacter* SourceCharacter = ActivePresentationSourceCharacter.Get())
        {
            SpawnTransientSpherePulse(
                SourceCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 8.0f),
                RoundPresentationTurnPulseDurationSeconds,
                ActivePresentationProfile.PulseStartScale,
                ActivePresentationProfile.PulseEndScale);
        }
        ActivePresentationDurationSeconds = RoundPresentationTurnStartDurationSeconds;
        return;
    }

    if (ActivePresentationEvent.EventType == TEXT("turn_ended"))
    {
        ActivePresentationDurationSeconds = RoundPresentationTurnEndDurationSeconds;
        return;
    }

    if (ActivePresentationEvent.EventType == TEXT("initiative_loss"))
    {
        if (HasAuthority())
        {
            DispatchPresentationSoundToPlayers(
                ActivePresentationEvent,
                ActivePresentationProfile.SoundVolumeMultiplier,
                ActivePresentationProfile.SoundPitchMultiplier,
                ActivePresentationProfile.SoundMaxDurationSeconds);
        }
        ActivePresentationTargetCharacter = FindLiveCharacterByRecordId(ActivePresentationEvent.TargetCharacterId);
        if (ARoundCharacter* TargetCharacter = ActivePresentationTargetCharacter.Get())
        {
            ActivePresentationTargetBaseScale = TargetCharacter->GetActorScale3D();
        }
        ActivePresentationDurationSeconds = RoundPresentationDelayDurationSeconds;
        return;
    }

    if (ActivePresentationEvent.EventType == TEXT("move"))
    {
        if (HasAuthority())
        {
            DispatchPresentationSoundToPlayers(
                ActivePresentationEvent,
                ActivePresentationProfile.SoundVolumeMultiplier,
                ActivePresentationProfile.SoundPitchMultiplier,
                ActivePresentationProfile.SoundMaxDurationSeconds);
        }
        ActivePresentationSourceCharacter = FindLiveCharacterByRecordId(ActivePresentationEvent.SourceCharacterId);
        if (ARoundCharacter* SourceCharacter = ActivePresentationSourceCharacter.Get())
        {
            ActivePresentationStartLocation = ResolveTileWorldLocation(ActivePresentationEvent.TilePosition);
            ActivePresentationEndLocation = ResolveTileWorldLocation(ActivePresentationEvent.SecondaryTilePosition);
            if (!ActivePresentationStartLocation.IsNearlyZero() && !ActivePresentationEndLocation.IsNearlyZero())
            {
                SourceCharacter->SetActorLocation(ActivePresentationStartLocation);
            }
        }
        ActivePresentationDurationSeconds = RoundPresentationMoveStepDurationSeconds;
        return;
    }

    if (ActivePresentationEvent.EventType == TEXT("basic_attack") || ActivePresentationEvent.EventType == TEXT("ability_use"))
    {
        if (HasAuthority())
        {
            DispatchPresentationSoundToPlayers(
                ActivePresentationEvent,
                ActivePresentationProfile.SoundVolumeMultiplier,
                ActivePresentationProfile.SoundPitchMultiplier,
                ActivePresentationProfile.SoundMaxDurationSeconds);
        }
        ActivePresentationSourceCharacter = FindLiveCharacterByRecordId(ActivePresentationEvent.SourceCharacterId);
        ActivePresentationTargetCharacter = FindLiveCharacterByRecordId(ActivePresentationEvent.TargetCharacterId);
        if (ARoundCharacter* SourceCharacter = ActivePresentationSourceCharacter.Get())
        {
            ActivePresentationStartLocation = SourceCharacter->GetActorLocation();
        }
        if (ARoundCharacter* TargetCharacter = ActivePresentationTargetCharacter.Get())
        {
            ActivePresentationEndLocation = TargetCharacter->GetActorLocation();
        }
        else
        {
            ActivePresentationEndLocation = ResolveTileWorldLocation(ActivePresentationEvent.TilePosition);
        }
        ActivePresentationDurationSeconds = ActivePresentationEvent.EventType == TEXT("ability_use")
            ? RoundPresentationAbilityDurationSeconds
            : RoundPresentationAttackDurationSeconds;
        if (!ActivePresentationStartLocation.IsNearlyZero() && !ActivePresentationEndLocation.IsNearlyZero())
        {
            SpawnTransientBeam(
                ActivePresentationStartLocation + FVector(0.0f, 0.0f, 55.0f),
                ActivePresentationEndLocation + FVector(0.0f, 0.0f, 55.0f),
                RoundPresentationBeamDurationSeconds,
                ActivePresentationProfile.BeamThicknessScale);
        }
        return;
    }

    if (ActivePresentationEvent.EventType == TEXT("damage") || ActivePresentationEvent.EventType == TEXT("heal"))
    {
        if (HasAuthority())
        {
            DispatchPresentationSoundToPlayers(
                ActivePresentationEvent,
                ActivePresentationProfile.SoundVolumeMultiplier,
                ActivePresentationProfile.SoundPitchMultiplier,
                ActivePresentationProfile.SoundMaxDurationSeconds);
        }
        ActivePresentationTargetCharacter = FindLiveCharacterByRecordId(ActivePresentationEvent.TargetCharacterId);
        if (ARoundCharacter* TargetCharacter = ActivePresentationTargetCharacter.Get())
        {
            ActivePresentationTargetBaseScale = TargetCharacter->GetActorScale3D();
        }
        ActivePresentationDurationSeconds = ActivePresentationEvent.EventType == TEXT("heal")
            ? RoundPresentationHealDurationSeconds
            : RoundPresentationDamageDurationSeconds;
        return;
    }

    if (ActivePresentationEvent.EventType == TEXT("death"))
    {
        if (HasAuthority())
        {
            DispatchPresentationSoundToPlayers(
                ActivePresentationEvent,
                ActivePresentationProfile.SoundVolumeMultiplier,
                ActivePresentationProfile.SoundPitchMultiplier,
                ActivePresentationProfile.SoundMaxDurationSeconds);
        }
        ActivePresentationTargetCharacter = FindLiveCharacterByRecordId(ActivePresentationEvent.TargetCharacterId);
        if (ARoundCharacter* TargetCharacter = ActivePresentationTargetCharacter.Get())
        {
            ActivePresentationTargetBaseScale = TargetCharacter->GetActorScale3D();
        }
        ActivePresentationDurationSeconds = RoundPresentationDeathDurationSeconds;
        return;
    }

    bPresentationEventActive = false;
    TryStartNextPresentationEvent();
}

void ARoundGameState::UpdateActivePresentationEvent(const float DeltaSeconds)
{
    if (!bPresentationEventActive)
    {
        TryStartNextPresentationEvent();
        return;
    }

    const float SafeDuration = FMath::Max(0.01f, ActivePresentationDurationSeconds);
    ActivePresentationElapsedSeconds += DeltaSeconds;
    const float Alpha = FMath::Clamp(ActivePresentationElapsedSeconds / SafeDuration, 0.0f, 1.0f);
    const float EaseAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
    const float PulseAlpha = FMath::Sin(Alpha * PI);

    if (ActivePresentationEvent.EventType == TEXT("move"))
    {
        if (ARoundCharacter* SourceCharacter = ActivePresentationSourceCharacter.Get())
        {
            FVector NewLocation = FMath::Lerp(ActivePresentationStartLocation, ActivePresentationEndLocation, EaseAlpha);
            NewLocation.Z += RoundPresentationMoveHopHeight * PulseAlpha;
            SourceCharacter->SetActorLocation(NewLocation);
        }
    }
    else if (ActivePresentationEvent.EventType == TEXT("basic_attack") || ActivePresentationEvent.EventType == TEXT("ability_use"))
    {
        if (ARoundCharacter* SourceCharacter = ActivePresentationSourceCharacter.Get())
        {
            FVector Direction = ActivePresentationEndLocation - ActivePresentationStartLocation;
            if (!Direction.Normalize())
            {
                Direction = FVector::ForwardVector;
            }

            const FVector LungeLocation = ActivePresentationStartLocation + (Direction * RoundPresentationAttackLungeDistance * PulseAlpha);
            SourceCharacter->SetActorLocation(LungeLocation);
        }
    }
    else if (ActivePresentationEvent.EventType == TEXT("damage") || ActivePresentationEvent.EventType == TEXT("heal"))
    {
        if (ARoundCharacter* TargetCharacter = ActivePresentationTargetCharacter.Get())
        {
            const float ScalePunch = ActivePresentationProfile.ScalePunch;
            TargetCharacter->SetActorScale3D(ActivePresentationTargetBaseScale * (1.0f + (ScalePunch * PulseAlpha)));
        }
    }
    else if (ActivePresentationEvent.EventType == TEXT("initiative_loss"))
    {
        if (ARoundCharacter* TargetCharacter = ActivePresentationTargetCharacter.Get())
        {
            TargetCharacter->SetActorScale3D(ActivePresentationTargetBaseScale * (1.0f + (ActivePresentationProfile.ScalePunch * PulseAlpha)));
        }
    }
    else if (ActivePresentationEvent.EventType == TEXT("death"))
    {
        if (ARoundCharacter* TargetCharacter = ActivePresentationTargetCharacter.Get())
        {
            TargetCharacter->SetActorScale3D(FMath::Lerp(ActivePresentationTargetBaseScale, FVector::ZeroVector, EaseAlpha));
        }
    }

    if (Alpha >= 1.0f)
    {
        FinishActivePresentationEvent();
    }
}

void ARoundGameState::FinishActivePresentationEvent()
{
    if (ARoundCharacter* SourceCharacter = ActivePresentationSourceCharacter.Get())
    {
        if (ActivePresentationEvent.EventType == TEXT("move"))
        {
            if (!ActivePresentationEndLocation.IsNearlyZero())
            {
                SourceCharacter->SetActorLocation(ActivePresentationEndLocation);
            }
        }
        else if (ActivePresentationEvent.EventType == TEXT("basic_attack") || ActivePresentationEvent.EventType == TEXT("ability_use"))
        {
            if (SourceCharacter->CurrentTile)
            {
                SourceCharacter->SetActorLocation(SourceCharacter->CurrentTile->GetActorLocation());
            }
            else
            {
                SourceCharacter->SetActorLocation(ActivePresentationStartLocation);
            }
        }
    }

    if (ARoundCharacter* TargetCharacter = ActivePresentationTargetCharacter.Get())
    {
        if (ActivePresentationEvent.EventType == TEXT("damage")
            || ActivePresentationEvent.EventType == TEXT("heal")
            || ActivePresentationEvent.EventType == TEXT("initiative_loss"))
        {
            TargetCharacter->SetActorScale3D(ActivePresentationTargetBaseScale);
        }
    }

    if (ActivePresentationEvent.EventType == TEXT("damage"))
    {
        if (ARoundCharacter* TargetCharacter = ActivePresentationTargetCharacter.Get())
        {
            SpawnTransientSpherePulse(
                TargetCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f),
                RoundPresentationImpactPulseDurationSeconds,
                ActivePresentationProfile.PulseStartScale,
                ActivePresentationProfile.PulseEndScale);
            SpawnFloatingText(
                TargetCharacter->GetActorLocation() + FVector(0.0f, 0.0f, ActivePresentationProfile.TextHeightOffset),
                FString::Printf(TEXT("-%d"), FMath::RoundToInt(ActivePresentationEvent.Amount)),
                ActivePresentationProfile.TextColor);
        }
    }
    else if (ActivePresentationEvent.EventType == TEXT("heal"))
    {
        if (ARoundCharacter* TargetCharacter = ActivePresentationTargetCharacter.Get())
        {
            SpawnTransientSpherePulse(
                TargetCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f),
                RoundPresentationImpactPulseDurationSeconds,
                ActivePresentationProfile.PulseStartScale,
                ActivePresentationProfile.PulseEndScale);
            SpawnFloatingText(
                TargetCharacter->GetActorLocation() + FVector(0.0f, 0.0f, ActivePresentationProfile.TextHeightOffset),
                FString::Printf(TEXT("+%d"), FMath::RoundToInt(ActivePresentationEvent.Amount)),
                ActivePresentationProfile.TextColor);
        }
    }
    else if (ActivePresentationEvent.EventType == TEXT("initiative_loss"))
    {
        if (ARoundCharacter* TargetCharacter = ActivePresentationTargetCharacter.Get())
        {
            SpawnFloatingText(
                TargetCharacter->GetActorLocation() + FVector(0.0f, 0.0f, ActivePresentationProfile.TextHeightOffset),
                FString::Printf(TEXT("-%d Init"), FMath::RoundToInt(ActivePresentationEvent.Amount)),
                ActivePresentationProfile.TextColor);
        }
    }
    else if (ActivePresentationEvent.EventType == TEXT("turn_started"))
    {
        if (ARoundCharacter* SourceCharacter = ActivePresentationSourceCharacter.Get())
        {
            SpawnFloatingText(
                SourceCharacter->GetActorLocation() + FVector(0.0f, 0.0f, ActivePresentationProfile.TextHeightOffset),
                ActivePresentationProfile.TextOverride,
                ActivePresentationProfile.TextColor);
        }
    }
    else if (ActivePresentationEvent.EventType == TEXT("basic_attack"))
    {
        if (ARoundCharacter* SourceCharacter = ActivePresentationSourceCharacter.Get())
        {
            SpawnFloatingText(
                SourceCharacter->GetActorLocation() + FVector(0.0f, 0.0f, ActivePresentationProfile.TextHeightOffset),
                ActivePresentationProfile.TextOverride,
                ActivePresentationProfile.TextColor);
        }
    }
    else if (ActivePresentationEvent.EventType == TEXT("ability_use"))
    {
        if (ARoundCharacter* SourceCharacter = ActivePresentationSourceCharacter.Get())
        {
            const FString AbilityLabel = ActivePresentationProfile.TextOverride.IsEmpty()
                ? TEXT("CAST")
                : ActivePresentationProfile.TextOverride;
            SpawnFloatingText(
                SourceCharacter->GetActorLocation() + FVector(0.0f, 0.0f, ActivePresentationProfile.TextHeightOffset),
                AbilityLabel,
                ActivePresentationProfile.TextColor);
        }
    }

    bPresentationEventActive = false;
    ActivePresentationEvent = FRoundPresentationEventRecord();
    ActivePresentationSourceCharacter = nullptr;
    ActivePresentationTargetCharacter = nullptr;
    ActivePresentationProfile = FRoundPresentationProfile();
    ActivePresentationElapsedSeconds = 0.0f;
    ActivePresentationDurationSeconds = 0.0f;
    TryStartNextPresentationEvent();
}

bool ARoundGameState::CanAnyLocalPlayerPerceivePresentationEvent(const FRoundPresentationEventRecord& EventRecord) const
{
    if (!GetWorld() || IsReplayActive())
    {
        return true;
    }

    const ARoundCharacter* SourceCharacter = FindLiveCharacterByRecordId(EventRecord.SourceCharacterId);
    const ARoundCharacter* TargetCharacter = FindLiveCharacterByRecordId(EventRecord.TargetCharacterId);
    const ARoundGridTile* PrimaryTile = GetField(EventRecord.TilePosition);
    const ARoundGridTile* SecondaryTile = GetField(EventRecord.SecondaryTilePosition);

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PlayerController = It->Get();
        if (!PlayerController || !PlayerController->IsLocalController())
        {
            continue;
        }

        const ARoundPlayerPawn* PlayerPawn = Cast<ARoundPlayerPawn>(PlayerController->GetPawn());
        if (!PlayerPawn)
        {
            return true;
        }

        const ERoundTeam EffectiveTeam = PlayerPawn->GetEffectiveTeam();
        if ((EventRecord.SourceTeam != ERoundTeam::Neutral && EventRecord.SourceTeam == EffectiveTeam)
            || (EventRecord.TargetTeam != ERoundTeam::Neutral && EventRecord.TargetTeam == EffectiveTeam))
        {
            return true;
        }

        if (SourceCharacter && PlayerPawn->CanSeeCharacter(SourceCharacter))
        {
            return true;
        }

        if (TargetCharacter && PlayerPawn->CanSeeCharacter(TargetCharacter))
        {
            return true;
        }

        if (PrimaryTile && PlayerPawn->CanSeeTile(PrimaryTile))
        {
            return true;
        }

        if (SecondaryTile && PlayerPawn->CanSeeTile(SecondaryTile))
        {
            return true;
        }
    }

    return false;
}

USoundBase* ARoundGameState::GetPresentationSoundForEvent(const FString& EventType) const
{
    if (ActivePresentationProfile.SoundOverride)
    {
        return ActivePresentationProfile.SoundOverride;
    }

    if (EventType == TEXT("move"))
    {
        return PresentationMoveSound;
    }
    if (EventType == TEXT("basic_attack"))
    {
        return PresentationAttackSound;
    }
    if (EventType == TEXT("ability_use"))
    {
        return PresentationAbilitySound;
    }
    if (EventType == TEXT("heal"))
    {
        return PresentationHealSound;
    }
    if (EventType == TEXT("death"))
    {
        return PresentationDeathSound;
    }
    if (EventType == TEXT("turn_started"))
    {
        return PresentationTurnSound;
    }
    if (EventType == TEXT("damage") || EventType == TEXT("initiative_loss"))
    {
        return PresentationImpactSound;
    }

    return nullptr;
}

bool ARoundGameState::CanPlayerPerceivePresentationEvent(const ARoundPlayerPawn* PlayerPawn, const FRoundPresentationEventRecord& EventRecord) const
{
    if (!PlayerPawn)
    {
        return false;
    }

    if (IsReplayActive())
    {
        return true;
    }

    const ERoundTeam EffectiveTeam = PlayerPawn->GetEffectiveTeam();
    if ((EventRecord.SourceTeam != ERoundTeam::Neutral && EventRecord.SourceTeam == EffectiveTeam)
        || (EventRecord.TargetTeam != ERoundTeam::Neutral && EventRecord.TargetTeam == EffectiveTeam))
    {
        return true;
    }

    TSet<const ARoundGridTile*> VisibleTiles;
    if (PlayerPawn->TeamActor && PlayerPawn->TeamActor->TeamId == EffectiveTeam)
    {
        for (ARoundGridTile* VisionTile : PlayerPawn->TeamActor->VisionFields)
        {
            if (VisionTile)
            {
                VisibleTiles.Add(VisionTile);
            }
        }
    }

    const ARoundCharacter* SourceCharacter = FindLiveCharacterByRecordId(EventRecord.SourceCharacterId);
    const ARoundCharacter* TargetCharacter = FindLiveCharacterByRecordId(EventRecord.TargetCharacterId);
    if (SourceCharacter && SourceCharacter->CurrentTile && VisibleTiles.Contains(SourceCharacter->CurrentTile))
    {
        return true;
    }
    if (TargetCharacter && TargetCharacter->CurrentTile && VisibleTiles.Contains(TargetCharacter->CurrentTile))
    {
        return true;
    }

    const ARoundGridTile* PrimaryTile = GetField(EventRecord.TilePosition);
    const ARoundGridTile* SecondaryTile = GetField(EventRecord.SecondaryTilePosition);
    return (PrimaryTile && VisibleTiles.Contains(PrimaryTile))
        || (SecondaryTile && VisibleTiles.Contains(SecondaryTile));
}

void ARoundGameState::DispatchPresentationSoundToPlayers(
    const FRoundPresentationEventRecord& EventRecord,
    const float VolumeMultiplier,
    const float PitchMultiplier,
    const float MaxDurationSeconds) const
{
    if (!HasAuthority())
    {
        return;
    }

    USoundBase* Sound = ActivePresentationProfile.SoundOverride
        ? ActivePresentationProfile.SoundOverride.Get()
        : GetPresentationSoundForEvent(EventRecord.EventType);
    if (!Sound || !GetWorld())
    {
        return;
    }

    for (TActorIterator<ARoundPlayerPawn> It(GetWorld()); It; ++It)
    {
        ARoundPlayerPawn* PlayerPawn = *It;
        if (!PlayerPawn || !CanPlayerPerceivePresentationEvent(PlayerPawn, EventRecord))
        {
            continue;
        }

        PlayerPawn->ClientPlayPresentationEventSound(
            Sound,
            VolumeMultiplier,
            PitchMultiplier,
            MaxDurationSeconds);
    }
}

void ARoundGameState::PlayPresentationSound(USoundBase* Sound, const FVector& WorldLocation, const float VolumeMultiplier, const float PitchMultiplier, const float MaxDurationSeconds) const
{
    if (!GetWorld() || !Sound || !CanAnyLocalPlayerPerceivePresentationEvent(ActivePresentationEvent))
    {
        return;
    }

    UAudioComponent* AudioComponent = UGameplayStatics::SpawnSound2D(
        this,
        Sound,
        VolumeMultiplier,
        PitchMultiplier);
    if (!AudioComponent || MaxDurationSeconds <= 0.0f)
    {
        return;
    }

    const float FadeOutSeconds = FMath::Min(0.08f, MaxDurationSeconds);
    const float HoldSeconds = FMath::Max(0.0f, MaxDurationSeconds - FadeOutSeconds);
    TWeakObjectPtr<UAudioComponent> WeakAudioComponent = AudioComponent;
    GetWorld()->GetTimerManager().SetTimerForNextTick([this, WeakAudioComponent, HoldSeconds, FadeOutSeconds]()
    {
        if (!GetWorld() || !WeakAudioComponent.IsValid())
        {
            return;
        }

        if (HoldSeconds <= 0.0f)
        {
            WeakAudioComponent->FadeOut(FadeOutSeconds, 0.0f);
            return;
        }

        FTimerHandle LocalFadeHandle;
        GetWorld()->GetTimerManager().SetTimer(
            LocalFadeHandle,
            [WeakAudioComponent, FadeOutSeconds]()
            {
                if (WeakAudioComponent.IsValid())
                {
                    WeakAudioComponent->FadeOut(FadeOutSeconds, 0.0f);
                }
            },
            HoldSeconds,
            false);
    });
}

void ARoundGameState::SpawnFloatingText(const FVector& WorldLocation, const FString& Text, const FLinearColor& Color)
{
    if (!GetWorld())
    {
        return;
    }

    UTextRenderComponent* TextComponent = NewObject<UTextRenderComponent>(this);
    if (!TextComponent)
    {
        return;
    }

    TextComponent->RegisterComponentWithWorld(GetWorld());
    TextComponent->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    TextComponent->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
    TextComponent->SetWorldSize(34.0f);
    TextComponent->SetText(FText::FromString(Text));
    TextComponent->SetTextRenderColor(Color.ToFColor(true));
    TextComponent->SetWorldLocation(WorldLocation);
    TextComponent->SetWorldRotation(FRotator(0.0f, 90.0f, 0.0f));
    TextComponent->SetHiddenInGame(false);

    FRoundFloatingTextEntry Entry;
    Entry.Component = TextComponent;
    Entry.StartWorldLocation = WorldLocation;
    Entry.EndWorldLocation = WorldLocation + FVector(0.0f, 0.0f, RoundFloatingTextRiseHeight);
    Entry.Color = Color;
    Entry.DurationSeconds = RoundFloatingTextDurationSeconds;
    ActiveFloatingTexts.Add(MoveTemp(Entry));
}

void ARoundGameState::UpdateFloatingTexts(const float DeltaSeconds)
{
    for (int32 Index = ActiveFloatingTexts.Num() - 1; Index >= 0; --Index)
    {
        FRoundFloatingTextEntry& Entry = ActiveFloatingTexts[Index];
        if (!Entry.Component)
        {
            ActiveFloatingTexts.RemoveAt(Index);
            continue;
        }

        Entry.ElapsedSeconds += DeltaSeconds;
        const float Alpha = FMath::Clamp(Entry.ElapsedSeconds / FMath::Max(0.01f, Entry.DurationSeconds), 0.0f, 1.0f);
        const float EaseAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
        Entry.Component->SetWorldLocation(FMath::Lerp(Entry.StartWorldLocation, Entry.EndWorldLocation, EaseAlpha));

        FLinearColor CurrentColor = Entry.Color;
        CurrentColor.A = 1.0f - Alpha;
        Entry.Component->SetTextRenderColor(CurrentColor.ToFColor(true));

        if (Alpha >= 1.0f)
        {
            Entry.Component->DestroyComponent();
            ActiveFloatingTexts.RemoveAt(Index);
        }
    }
}

void ARoundGameState::SpawnTransientSpherePulse(const FVector& WorldLocation, const float DurationSeconds, const FVector& StartScale, const FVector& EndScale)
{
    if (!GetWorld() || !PresentationSphereMesh)
    {
        return;
    }

    UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(this);
    if (!MeshComponent)
    {
        return;
    }

    MeshComponent->SetStaticMesh(PresentationSphereMesh);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetGenerateOverlapEvents(false);
    MeshComponent->SetCastShadow(false);
    MeshComponent->SetMobility(EComponentMobility::Movable);
    MeshComponent->SetHiddenInGame(false);
    MeshComponent->RegisterComponentWithWorld(GetWorld());
    MeshComponent->SetWorldLocation(WorldLocation);
    MeshComponent->SetWorldScale3D(StartScale);

    FRoundTransientMeshEntry Entry;
    Entry.Component = MeshComponent;
    Entry.StartWorldLocation = WorldLocation;
    Entry.EndWorldLocation = WorldLocation;
    Entry.StartScale = StartScale;
    Entry.EndScale = EndScale;
    Entry.DurationSeconds = DurationSeconds;
    ActiveTransientMeshes.Add(MoveTemp(Entry));
}

void ARoundGameState::SpawnTransientBeam(const FVector& StartWorldLocation, const FVector& EndWorldLocation, const float DurationSeconds, const float ThicknessScale)
{
    if (!GetWorld() || !PresentationCylinderMesh)
    {
        return;
    }

    const FVector Direction = EndWorldLocation - StartWorldLocation;
    const float Length = Direction.Size();
    if (Length <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    UStaticMeshComponent* MeshComponent = NewObject<UStaticMeshComponent>(this);
    if (!MeshComponent)
    {
        return;
    }

    MeshComponent->SetStaticMesh(PresentationCylinderMesh);
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetGenerateOverlapEvents(false);
    MeshComponent->SetCastShadow(false);
    MeshComponent->SetMobility(EComponentMobility::Movable);
    MeshComponent->SetHiddenInGame(false);
    MeshComponent->RegisterComponentWithWorld(GetWorld());

    const FVector Midpoint = (StartWorldLocation + EndWorldLocation) * 0.5f;
    MeshComponent->SetWorldLocation(Midpoint);
    MeshComponent->SetWorldRotation(FRotationMatrix::MakeFromZ(Direction.GetSafeNormal()).Rotator());

    FRoundTransientMeshEntry Entry;
    Entry.Component = MeshComponent;
    Entry.StartWorldLocation = Midpoint;
    Entry.EndWorldLocation = Midpoint;
    Entry.StartScale = FVector(ThicknessScale, ThicknessScale, Length / RoundPresentationCylinderHeight);
    Entry.EndScale = FVector(0.01f, 0.01f, Length / RoundPresentationCylinderHeight);
    Entry.DurationSeconds = DurationSeconds;
    ActiveTransientMeshes.Add(MoveTemp(Entry));
}

void ARoundGameState::UpdateTransientMeshes(const float DeltaSeconds)
{
    for (int32 Index = ActiveTransientMeshes.Num() - 1; Index >= 0; --Index)
    {
        FRoundTransientMeshEntry& Entry = ActiveTransientMeshes[Index];
        if (!Entry.Component)
        {
            ActiveTransientMeshes.RemoveAt(Index);
            continue;
        }

        Entry.ElapsedSeconds += DeltaSeconds;
        const float Alpha = FMath::Clamp(Entry.ElapsedSeconds / FMath::Max(0.01f, Entry.DurationSeconds), 0.0f, 1.0f);
        const float EaseAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
        Entry.Component->SetWorldLocation(FMath::Lerp(Entry.StartWorldLocation, Entry.EndWorldLocation, EaseAlpha));
        Entry.Component->SetWorldScale3D(FMath::Lerp(Entry.StartScale, Entry.EndScale, EaseAlpha));

        if (Alpha >= 1.0f)
        {
            Entry.Component->DestroyComponent();
            ActiveTransientMeshes.RemoveAt(Index);
        }
    }
}

void ARoundGameState::RecordCharacterSpawned(ARoundCharacter* Character)
{
    if (!HasAuthority() || !Character)
    {
        return;
    }

    const FString CharacterId = FString::Printf(TEXT("%s_%d"), *Character->GetName(), CurrentMatchRecord.SpawnedCharacters.Num());
    CharacterRecordIds.Add(Character, CharacterId);

    FRoundMatchCharacterSnapshot Snapshot;
    Snapshot.CharacterInstanceId = CharacterId;
    Snapshot.CharacterRowName = Character->CharacterRowName.ToString();
    Snapshot.Team = Character->Team;
    Snapshot.SpawnPosition = Character->CurrentTile ? Character->CurrentTile->GridPosition : FVector2D::ZeroVector;
    Snapshot.CharacterPassiveSelections = Character->CharacterPassiveSelections;
    Snapshot.AbilitySkillSelections = Character->AbilitySkillSelections;
    CurrentMatchRecord.SpawnedCharacters.Add(MoveTemp(Snapshot));

    FRoundMatchEventRecord Event;
    Event.Sequence = ++MatchEventSequence;
    Event.TurnNumber = MatchTurnCounter;
    Event.TimestampUtc = BuildIsoUtcTimestamp();
    Event.EventType = TEXT("character_spawned");
    Event.SourceCharacterId = CharacterId;
    Event.TilePosition = Character->CurrentTile ? Character->CurrentTile->GridPosition : FVector2D::ZeroVector;
    Event.StringData.Add(TEXT("character_row"), Character->CharacterRowName.ToString());
    Event.StringData.Add(TEXT("team"), StaticEnum<ERoundTeam>()->GetNameStringByValue(static_cast<int64>(Character->Team)));
    Event.bSuccess = true;
    AppendMatchEvent(Event);
}

void ARoundGameState::RecordTurnStarted(ARoundCharacter* Character)
{
    if (!HasAuthority() || !Character)
    {
        return;
    }

    FRoundMatchEventRecord Event;
    Event.Sequence = ++MatchEventSequence;
    Event.TurnNumber = MatchTurnCounter;
    Event.TimestampUtc = BuildIsoUtcTimestamp();
    Event.EventType = TEXT("turn_started");
    Event.SourceCharacterId = ResolveCharacterRecordId(Character);
    Event.TilePosition = Character->CurrentTile ? Character->CurrentTile->GridPosition : FVector2D::ZeroVector;
    Event.NumericData.Add(TEXT("initiative"), Character->Attributes ? Character->Attributes->CurrentInitiative : 0.0);
    Event.bSuccess = true;
    AppendMatchEvent(Event);
}

void ARoundGameState::RecordTurnEnded(ARoundCharacter* Character)
{
    if (!HasAuthority() || !Character)
    {
        return;
    }

    FRoundMatchEventRecord Event;
    Event.Sequence = ++MatchEventSequence;
    Event.TurnNumber = MatchTurnCounter;
    Event.TimestampUtc = BuildIsoUtcTimestamp();
    Event.EventType = TEXT("turn_ended");
    Event.SourceCharacterId = ResolveCharacterRecordId(Character);
    Event.TilePosition = Character->CurrentTile ? Character->CurrentTile->GridPosition : FVector2D::ZeroVector;
    Event.bSuccess = true;
    AppendMatchEvent(Event);
}

void ARoundGameState::RecordMovement(ARoundCharacter* Character, const FVector2D& FromPosition, const TArray<FVector2D>& TraversedPositions, const int32 TotalCost)
{
    if (!HasAuthority() || !Character || TraversedPositions.Num() <= 0)
    {
        return;
    }

    FVector2D PreviousPosition = FromPosition;
    for (int32 StepIndex = 0; StepIndex < TraversedPositions.Num(); ++StepIndex)
    {
        FRoundMatchEventRecord Event;
        Event.Sequence = ++MatchEventSequence;
        Event.TurnNumber = MatchTurnCounter;
        Event.TimestampUtc = BuildIsoUtcTimestamp();
        Event.EventType = TEXT("move");
        Event.SourceCharacterId = ResolveCharacterRecordId(Character);
        Event.TilePosition = PreviousPosition;
        Event.SecondaryTilePosition = TraversedPositions[StepIndex];
        Event.Amount = StepIndex == TraversedPositions.Num() - 1 ? TotalCost : 0;
        Event.SecondaryAmount = StepIndex + 1;
        Event.bSuccess = true;
        AppendMatchEvent(Event);
        PreviousPosition = TraversedPositions[StepIndex];
    }
}

void ARoundGameState::RecordBasicAttack(ARoundCharacter* SourceCharacter, ARoundCharacter* TargetCharacter, int32 DamageDone)
{
    if (!HasAuthority() || !SourceCharacter)
    {
        return;
    }

    FRoundMatchEventRecord Event;
    Event.Sequence = ++MatchEventSequence;
    Event.TurnNumber = MatchTurnCounter;
    Event.TimestampUtc = BuildIsoUtcTimestamp();
    Event.EventType = TEXT("basic_attack");
    Event.SourceCharacterId = ResolveCharacterRecordId(SourceCharacter);
    Event.TargetCharacterId = ResolveCharacterRecordId(TargetCharacter);
    Event.TilePosition = TargetCharacter && TargetCharacter->CurrentTile ? TargetCharacter->CurrentTile->GridPosition : FVector2D::ZeroVector;
    Event.Amount = DamageDone;
    Event.bSuccess = true;
    AppendMatchEvent(Event);
}

void ARoundGameState::RecordAbilityUse(ARoundCharacter* SourceCharacter, FName AbilityId, ARoundGridTile* TargetTile, ARoundCharacter* TargetCharacter, const FRoundActionResult& Result)
{
    if (!HasAuthority() || !SourceCharacter)
    {
        return;
    }

    FRoundMatchEventRecord Event;
    Event.Sequence = ++MatchEventSequence;
    Event.TurnNumber = MatchTurnCounter;
    Event.TimestampUtc = BuildIsoUtcTimestamp();
    Event.EventType = TEXT("ability_use");
    Event.SourceCharacterId = ResolveCharacterRecordId(SourceCharacter);
    Event.TargetCharacterId = ResolveCharacterRecordId(TargetCharacter);
    Event.AbilityId = AbilityId.ToString();
    Event.TilePosition = TargetTile ? TargetTile->GridPosition : FVector2D::ZeroVector;
    Event.bSuccess = Result.bSuccess;
    Event.Reason = Result.Reason;
    AppendMatchEvent(Event);
}

void ARoundGameState::RecordDamage(ARoundCharacter* SourceCharacter, ARoundCharacter* TargetCharacter, int32 RequestedDamage, int32 AppliedDamage, ERoundDamageType DamageType, int32 DefenseIgnore, bool bCrit)
{
    if (!HasAuthority() || !TargetCharacter)
    {
        return;
    }

    FRoundMatchEventRecord Event;
    Event.Sequence = ++MatchEventSequence;
    Event.TurnNumber = MatchTurnCounter;
    Event.TimestampUtc = BuildIsoUtcTimestamp();
    Event.EventType = TEXT("damage");
    Event.SourceCharacterId = ResolveCharacterRecordId(SourceCharacter);
    Event.TargetCharacterId = ResolveCharacterRecordId(TargetCharacter);
    Event.TilePosition = TargetCharacter->CurrentTile ? TargetCharacter->CurrentTile->GridPosition : FVector2D::ZeroVector;
    Event.Amount = AppliedDamage;
    Event.SecondaryAmount = RequestedDamage;
    Event.bSuccess = AppliedDamage > 0;
    Event.StringData.Add(TEXT("damage_type"), StaticEnum<ERoundDamageType>()->GetNameStringByValue(static_cast<int64>(DamageType)));
    Event.NumericData.Add(TEXT("defense_ignore"), DefenseIgnore);
    Event.NumericData.Add(TEXT("crit"), bCrit ? 1.0 : 0.0);
    AppendMatchEvent(Event);
}

void ARoundGameState::RecordHeal(ARoundCharacter* TargetCharacter, int32 HealAmount)
{
    if (!HasAuthority() || !TargetCharacter || HealAmount <= 0)
    {
        return;
    }

    FRoundMatchEventRecord Event;
    Event.Sequence = ++MatchEventSequence;
    Event.TurnNumber = MatchTurnCounter;
    Event.TimestampUtc = BuildIsoUtcTimestamp();
    Event.EventType = TEXT("heal");
    Event.TargetCharacterId = ResolveCharacterRecordId(TargetCharacter);
    Event.TilePosition = TargetCharacter->CurrentTile ? TargetCharacter->CurrentTile->GridPosition : FVector2D::ZeroVector;
    Event.Amount = HealAmount;
    Event.bSuccess = true;
    AppendMatchEvent(Event);
}

void ARoundGameState::RecordDelayApplied(ARoundCharacter* TargetCharacter, int32 Amount)
{
    if (!HasAuthority() || !TargetCharacter || Amount <= 0)
    {
        return;
    }

    FRoundMatchEventRecord Event;
    Event.Sequence = ++MatchEventSequence;
    Event.TurnNumber = MatchTurnCounter;
    Event.TimestampUtc = BuildIsoUtcTimestamp();
    Event.EventType = TEXT("initiative_loss");
    Event.TargetCharacterId = ResolveCharacterRecordId(TargetCharacter);
    Event.TilePosition = TargetCharacter->CurrentTile ? TargetCharacter->CurrentTile->GridPosition : FVector2D::ZeroVector;
    Event.Amount = Amount;
    Event.bSuccess = true;
    AppendMatchEvent(Event);
}

void ARoundGameState::RecordCharacterDeath(ARoundCharacter* Character)
{
    if (!HasAuthority() || !Character)
    {
        return;
    }

    FRoundMatchEventRecord Event;
    Event.Sequence = ++MatchEventSequence;
    Event.TurnNumber = MatchTurnCounter;
    Event.TimestampUtc = BuildIsoUtcTimestamp();
    Event.EventType = TEXT("death");
    Event.TargetCharacterId = ResolveCharacterRecordId(Character);
    Event.TilePosition = Character->CurrentTile ? Character->CurrentTile->GridPosition : FVector2D::ZeroVector;
    Event.StringData.Add(TEXT("team"), StaticEnum<ERoundTeam>()->GetNameStringByValue(static_cast<int64>(Character->Team)));
    Event.bSuccess = true;
    AppendMatchEvent(Event);
}

void ARoundGameState::TryFinalizeMatchRecordingAfterDeath()
{
    if (!HasAuthority() || bMatchRecordingFinalized)
    {
        return;
    }

    TSet<ERoundTeam> AliveTeams;
    for (ARoundCharacter* Character : RegisteredCharacters)
    {
        if (!Character || !Character->IsAlive() || Character->Team == ERoundTeam::Neutral)
        {
            continue;
        }

        AliveTeams.Add(Character->Team);
    }

    if (AliveTeams.Num() == 1)
    {
        FinalizeAndSaveMatchRecording(AliveTeams.Array()[0]);
    }
}

void ARoundGameState::FinalizeAndSaveMatchRecording(ERoundTeam WinnerTeam)
{
    if (!HasAuthority() || bMatchRecordingFinalized)
    {
        return;
    }

    StopPythonLiveAutoplay();
    bMatchRecordingFinalized = true;
    CurrentMatchRecord.WinnerTeam = WinnerTeam;
    CurrentMatchRecord.FinishedAtUtc = BuildIsoUtcTimestamp();

    FRoundMatchEventRecord Event;
    Event.Sequence = ++MatchEventSequence;
    Event.TurnNumber = MatchTurnCounter;
    Event.TimestampUtc = CurrentMatchRecord.FinishedAtUtc;
    Event.EventType = TEXT("match_finished");
    Event.StringData.Add(TEXT("winner_team"), StaticEnum<ERoundTeam>()->GetNameStringByValue(static_cast<int64>(WinnerTeam)));
    Event.bSuccess = true;
    CurrentMatchRecord.Events.Add(Event);

    TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    if (!FJsonObjectConverter::UStructToJsonObject(FRoundMatchRecord::StaticStruct(), &CurrentMatchRecord, JsonObject, 0, 0))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to convert match record to json - MatchId=%s"), *CurrentMatchRecord.MatchId);
        return;
    }

    FString JsonOutput;
    const TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&JsonOutput);
    if (!FJsonSerializer::Serialize(JsonObject, Writer))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to serialize match record json - MatchId=%s"), *CurrentMatchRecord.MatchId);
        return;
    }

    const FString DirectoryPath = TEXT("F:/RoundGamesHist");
    IFileManager::Get().MakeDirectory(*DirectoryPath, true);
    const FString FilePath = FPaths::Combine(DirectoryPath, FString::Printf(TEXT("%s.json"), *CurrentMatchRecord.MatchId));

    if (!FFileHelper::SaveStringToFile(JsonOutput, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save match record - %s"), *FilePath);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Saved match record to %s"), *FilePath);
}

void ARoundGameState::EnsureInitialFields()
{
    if (Fields.Num() > 0)
    {
        return;
    }

    if (InitialFieldPositions.Num() == 0)
    {
        const URoundGameInstance* RoundGameInstance = GetGameInstance<URoundGameInstance>();

        ERoundFormat ActiveFormat = ERoundFormat::Format3x16;
        if (RoundGameInstance)
        {
            if (RoundGameInstance->LoadedTeams.IsValidIndex(0))
            {
                ActiveFormat = RoundGameInstance->LoadedTeams[0].Format;
            }
            else
            {
                ActiveFormat = RoundGameInstance->StrTeam.Format;
            }
        }

        FRoundFormatDefinition FormatDefinition;
        bool bHasFormatDefinition = false;
        if (RoundGameInstance)
        {
            bHasFormatDefinition = RoundGameInstance->TryGetFormatDefinition(ActiveFormat, FormatDefinition);
        }
        if (!bHasFormatDefinition)
        {
            bHasFormatDefinition = URoundGameInstance::TryGetBuiltInFormatDefinition(ActiveFormat, FormatDefinition);
        }

        if (bHasFormatDefinition)
        {
            TMap<ERoundTerrainType, float> TerrainWeights;
            if (FormatDefinition.TerrainWeights.Num() > 0)
            {
                for (const TPair<ERoundTerrainType, double>& Pair : FormatDefinition.TerrainWeights)
                {
                    TerrainWeights.Add(Pair.Key, static_cast<float>(Pair.Value));
                }
            }
            else
            {
                TerrainWeights = BuildDefaultTerrainWeights();
            }
            CreateFieldFromParameters(
                FormatDefinition.FieldSize,
                FormatDefinition.StartPosition,
                FormatDefinition.SkillAmount,
                FormatDefinition.Terrain,
                TerrainWeights);
            return;
        }

        if (InitialFieldSize > 0)
        {
            CreateFieldFromParameters(
                InitialFieldSize,
                InitialFieldStartPosition,
                InitialSkillAmount,
                InitialTerrainAmount,
                InitialTerrainWeights.Num() > 0 ? InitialTerrainWeights : BuildDefaultTerrainWeights());
            return;
        }
    }

    for (const FVector2D& GridPosition : InitialFieldPositions)
    {
        SpawnFieldAt(GridPosition, ERoundTerrainType::Ground);
    }
}

void ARoundGameState::GenerateInitialTerrain()
{
    if (Fields.Num() == 0 || bFinishedGenerating)
    {
        return;
    }

    if (GenerateTerrainRegions())
    {
        OnBoardStateChanged.Broadcast();
        return;
    }

    if (InitialTerrainSequence.Num() == 0)
    {
        return;
    }

    const TArray<ERoundTerrainType> OrderedTerrainTypes = OrderTerrainTypes(InitialTerrainSequence);
    for (const ERoundTerrainType TerrainType : OrderedTerrainTypes)
    {
        CreateTerrain(TerrainType);
    }
}

void ARoundGameState::ResetBoardFields()
{
    for (ARoundGridTile* Field : Fields)
    {
        if (Field)
        {
            Field->Destroy();
        }
    }

    Fields.Reset();
    TerrainTypes.Reset();
    bFinishedGenerating = false;
}

void ARoundGameState::ResetReplayBoard()
{
    StopReplay();
    StopPythonLiveAutoplay();

    if (CurrentObjectiveActor)
    {
        CurrentObjectiveActor->Destroy();
        CurrentObjectiveActor = nullptr;
    }

    for (TActorIterator<ARoundObjectiveRewardActor> It(GetWorld()); It; ++It)
    {
        if (ARoundObjectiveRewardActor* RewardActor = *It)
        {
            RewardActor->Destroy();
        }
    }

    for (ARoundCharacter* Character : RegisteredCharacters)
    {
        if (Character)
        {
            Character->Destroy();
        }
    }
    RegisteredCharacters.Reset();
    CharacterRecordIds.Reset();
    ReplayCharactersByRecordId.Reset();

    for (UObject* TeamObject : Teams)
    {
        if (AActor* TeamActor = Cast<AActor>(TeamObject))
        {
            TeamActor->Destroy();
        }
    }
    Teams.Reset();
    Player1 = nullptr;
    Player2 = nullptr;

    TurnOrder.Reset();
    ActiveTurnIndex = INDEX_NONE;
    CurrentTurnCharacter = nullptr;
    MatchTurnCounter = 0;
    MatchGlobalInitiativeElapsed = 0;
    bMatchRecordingFinalized = false;
    ActiveReplayRecord = FRoundMatchRecord();
    ReplayNextEventIndex = 0;
    bReplayActive = false;

    ResetBoardFields();
    OnBoardStateChanged.Broadcast();
}

ARoundTeamActor* ARoundGameState::SpawnReplayTeamActor(const ERoundTeam TeamId, const FRoundTeamDefinition& TeamDefinition)
{
    if (!GetWorld() || !TeamActorClass)
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ARoundTeamActor* TeamActor = GetWorld()->SpawnActor<ARoundTeamActor>(TeamActorClass, FTransform::Identity, SpawnParameters);
    if (!TeamActor)
    {
        return nullptr;
    }

    TeamActor->TeamId = TeamId;
    TeamActor->TeamDefinition = TeamDefinition;
    Teams.Add(TeamActor);
    return TeamActor;
}

ARoundCharacter* ARoundGameState::FindReplayCharacterByRecordId(const FString& CharacterId) const
{
    if (const TWeakObjectPtr<ARoundCharacter>* Found = ReplayCharactersByRecordId.Find(CharacterId))
    {
        return Found->Get();
    }

    return nullptr;
}

ARoundCharacter* ARoundGameState::FindReplayCharacterAtTile(const FVector2D& TilePosition) const
{
    for (const TPair<FString, TWeakObjectPtr<ARoundCharacter>>& Pair : ReplayCharactersByRecordId)
    {
        if (ARoundCharacter* Character = Pair.Value.Get())
        {
            if (Character->CurrentTile && Character->CurrentTile->GridPosition == TilePosition)
            {
                return Character;
            }
        }
    }

    return nullptr;
}

bool ARoundGameState::LoadReplayFromFile(const FString& FilePath, FString& OutError)
{
    FString JsonText;
    const FString ResolvedPath = ResolveReplayImportPath(FilePath);
    if (ResolvedPath.IsEmpty())
    {
        OutError = TEXT("Empty replay path.");
        return false;
    }

    if (!FFileHelper::LoadFileToString(JsonText, *ResolvedPath))
    {
        OutError = FString::Printf(TEXT("Could not read replay file: %s"), *ResolvedPath);
        return false;
    }

    FRoundMatchRecord MatchRecord;
    if (!FJsonObjectConverter::JsonObjectStringToUStruct(JsonText, &MatchRecord, 0, 0))
    {
        OutError = FString::Printf(TEXT("Could not parse replay json: %s"), *ResolvedPath);
        return false;
    }

    StartReplay(MatchRecord);
    return true;
}

void ARoundGameState::StartReplay(const FRoundMatchRecord& MatchRecord)
{
    ResetReplayBoard();
    ActiveReplayRecord = MatchRecord;
    bMatchRecordingFinalized = true;
    UE_LOG(
        LogTemp,
        Log,
        TEXT("Replay StartReplay: match_id=%s fields=%d teams=%d characters=%d events=%d"),
        *MatchRecord.MatchId,
        MatchRecord.Fields.Num(),
        MatchRecord.Teams.Num(),
        MatchRecord.SpawnedCharacters.Num(),
        MatchRecord.Events.Num());

    for (const FRoundMatchFieldSnapshot& FieldSnapshot : MatchRecord.Fields)
    {
        SpawnFieldAt(FieldSnapshot.Position, FieldSnapshot.TerrainType);
    }
    bFinishedGenerating = Fields.Num() > 0;

    ARoundTeamActor* TeamAActor = nullptr;
    ARoundTeamActor* TeamBActor = nullptr;
    if (MatchRecord.Teams.IsValidIndex(0))
    {
        TeamAActor = SpawnReplayTeamActor(ERoundTeam::TeamA, MatchRecord.Teams[0]);
    }
    if (MatchRecord.Teams.IsValidIndex(1))
    {
        TeamBActor = SpawnReplayTeamActor(ERoundTeam::TeamB, MatchRecord.Teams[1]);
    }

    for (const FRoundMatchCharacterSnapshot& CharacterSnapshot : MatchRecord.SpawnedCharacters)
    {
        const int32 ReplayCharacterIndex = ReplayCharactersByRecordId.Num();
        UObject* TeamObject = CharacterSnapshot.Team == ERoundTeam::TeamB
            ? static_cast<UObject*>(TeamBActor)
            : static_cast<UObject*>(TeamAActor);
        ARoundCharacter* SpawnedCharacter = SpawnCharacterByRowName(
            *CharacterSnapshot.CharacterRowName,
            CharacterSnapshot.SpawnPosition,
            TeamObject,
            0,
            CharacterSnapshot.CharacterPassiveSelections,
            CharacterSnapshot.AbilitySkillSelections);
        if (!SpawnedCharacter)
        {
            continue;
        }

        CharacterRecordIds.Add(SpawnedCharacter, CharacterSnapshot.CharacterInstanceId);
        ReplayCharactersByRecordId.Add(CharacterSnapshot.CharacterInstanceId, SpawnedCharacter);
        ReplayCharactersByRecordId.Add(FString::FromInt(ReplayCharacterIndex), SpawnedCharacter);
    }

    if (TeamAActor)
    {
        TeamAActor->SetVisionFields();
    }
    if (TeamBActor)
    {
        TeamBActor->SetVisionFields();
    }

    ReplayNextEventIndex = 0;
    bReplayActive = true;

    if (GetWorld())
    {
        GetWorldTimerManager().SetTimer(
            ReplayTimerHandle,
            this,
            &ARoundGameState::AdvanceReplay,
            1.0f,
            true,
            1.0f);
    }

    RefreshReplayVisionForPlayers();
}

void ARoundGameState::StopReplay()
{
    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(ReplayTimerHandle);
    }
    UE_LOG(
        LogTemp,
        Log,
        TEXT("Replay StopReplay: next_event_index=%d total_events=%d"),
        ReplayNextEventIndex,
        ActiveReplayRecord.Events.Num());
    bReplayActive = false;
}

void ARoundGameState::ApplyReplayEvent(const FRoundMatchEventRecord& EventRecord)
{
    MatchTurnCounter = EventRecord.TurnNumber;

    if (EventRecord.NumericData.Contains(TEXT("global_initiative_elapsed")))
    {
        MatchGlobalInitiativeElapsed = FMath::RoundToInt(EventRecord.NumericData.FindRef(TEXT("global_initiative_elapsed")));
    }

    if (EventRecord.EventType == TEXT("character_spawned"))
    {
        UE_LOG(
            LogTemp,
            Log,
            TEXT("Replay ApplyEvent: turn=%d type=%s ignored_setup_spawn source=%s target=%s tile=(%.0f,%.0f) secondary=(%.0f,%.0f)"),
            EventRecord.TurnNumber,
            *EventRecord.EventType,
            *EventRecord.SourceCharacterId,
            *EventRecord.TargetCharacterId,
            EventRecord.TilePosition.X,
            EventRecord.TilePosition.Y,
            EventRecord.SecondaryTilePosition.X,
            EventRecord.SecondaryTilePosition.Y);
        return;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("Replay ApplyEvent: turn=%d type=%s source=%s target=%s tile=(%.0f,%.0f) secondary=(%.0f,%.0f) amount=%.2f"),
        EventRecord.TurnNumber,
        *EventRecord.EventType,
        *EventRecord.SourceCharacterId,
        *EventRecord.TargetCharacterId,
        EventRecord.TilePosition.X,
        EventRecord.TilePosition.Y,
        EventRecord.SecondaryTilePosition.X,
        EventRecord.SecondaryTilePosition.Y,
        EventRecord.Amount);

    if (EventRecord.EventType == TEXT("turn_started"))
    {
        CurrentTurnCharacter = FindReplayCharacterByRecordId(EventRecord.SourceCharacterId);
        if (!CurrentTurnCharacter)
        {
            CurrentTurnCharacter = FindReplayCharacterAtTile(EventRecord.TilePosition);
        }
        OnTurnChanged.Broadcast(CurrentTurnCharacter);
        return;
    }

    if (EventRecord.EventType == TEXT("turn_ended"))
    {
        CurrentTurnCharacter = nullptr;
        OnTurnChanged.Broadcast(nullptr);
        return;
    }

    if (EventRecord.EventType == TEXT("move"))
    {
        ARoundCharacter* SourceCharacter = FindReplayCharacterByRecordId(EventRecord.SourceCharacterId);
        if (!SourceCharacter)
        {
            SourceCharacter = FindReplayCharacterAtTile(EventRecord.TilePosition);
        }
        if (SourceCharacter)
        {
            if (ARoundGridTile* TargetTile = GetField(EventRecord.SecondaryTilePosition))
            {
                UE_LOG(
                    LogTemp,
                    Log,
                    TEXT("Replay Move: character=%s from_tile=%s to_tile=%s"),
                    *GetNameSafe(SourceCharacter),
                    *GetNameSafe(SourceCharacter->CurrentTile),
                    *GetNameSafe(TargetTile));
                SourceCharacter->SetField(TargetTile, true);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Replay Move: target tile not found for secondary position (%.0f,%.0f)"), EventRecord.SecondaryTilePosition.X, EventRecord.SecondaryTilePosition.Y);
            }
        }
        else
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Replay Move: source character not resolved for source_id=%s tile=(%.0f,%.0f)"),
                *EventRecord.SourceCharacterId,
                EventRecord.TilePosition.X,
                EventRecord.TilePosition.Y);
        }
        return;
    }

    if (EventRecord.EventType == TEXT("basic_attack") || EventRecord.EventType == TEXT("ability_use"))
    {
        return;
    }

    if (EventRecord.EventType == TEXT("damage"))
    {
        ARoundCharacter* TargetCharacter = FindReplayCharacterByRecordId(EventRecord.TargetCharacterId);
        if (!TargetCharacter)
        {
            TargetCharacter = FindReplayCharacterAtTile(EventRecord.TilePosition);
        }
        ARoundCharacter* SourceCharacter = FindReplayCharacterByRecordId(EventRecord.SourceCharacterId);
        if (!SourceCharacter)
        {
            SourceCharacter = FindReplayCharacterAtTile(EventRecord.TilePosition);
        }
        if (TargetCharacter)
        {
            if (URoundAttributeSetComponent* Attributes = TargetCharacter->Attributes)
            {
                const int32 NewHealth = FMath::Max(0, Attributes->CurrentHealth - FMath::Max(0, EventRecord.Amount));
                Attributes->SetCurrentHealthValue(NewHealth);
            }
        }
        return;
    }

    if (EventRecord.EventType == TEXT("heal"))
    {
        ARoundCharacter* TargetCharacter = FindReplayCharacterByRecordId(EventRecord.TargetCharacterId);
        if (!TargetCharacter)
        {
            TargetCharacter = FindReplayCharacterAtTile(EventRecord.TilePosition);
        }
        if (TargetCharacter)
        {
            TargetCharacter->Heal(EventRecord.Amount);
        }
        return;
    }

    if (EventRecord.EventType == TEXT("initiative_loss"))
    {
        ARoundCharacter* TargetCharacter = FindReplayCharacterByRecordId(EventRecord.TargetCharacterId);
        if (!TargetCharacter)
        {
            TargetCharacter = FindReplayCharacterAtTile(EventRecord.TilePosition);
        }
        if (TargetCharacter)
        {
            TargetCharacter->ApplyDelay(EventRecord.Amount);
        }
        return;
    }

    if (EventRecord.EventType == TEXT("death"))
    {
        ARoundCharacter* TargetCharacter = FindReplayCharacterByRecordId(EventRecord.TargetCharacterId);
        if (!TargetCharacter)
        {
            TargetCharacter = FindReplayCharacterAtTile(EventRecord.TilePosition);
        }
        if (TargetCharacter)
        {
            TargetCharacter->Die();
        }
        return;
    }

    if (EventRecord.EventType == TEXT("match_finished"))
    {
        UE_LOG(LogTemp, Log, TEXT("Replay ApplyEvent: match_finished"));
        StopReplay();
        return;
    }
}

void ARoundGameState::AdvanceReplay()
{
    if (!bReplayActive || ReplayNextEventIndex >= ActiveReplayRecord.Events.Num())
    {
        UE_LOG(
            LogTemp,
            Log,
            TEXT("Replay AdvanceReplay: stopping because active=%d next_event_index=%d total_events=%d"),
            bReplayActive ? 1 : 0,
            ReplayNextEventIndex,
            ActiveReplayRecord.Events.Num());
        StopReplay();
        return;
    }

    const int32 CurrentTurnNumber = ActiveReplayRecord.Events[ReplayNextEventIndex].TurnNumber;
    UE_LOG(
        LogTemp,
        Log,
        TEXT("Replay AdvanceReplay: turn=%d start_index=%d total_events=%d"),
        CurrentTurnNumber,
        ReplayNextEventIndex,
        ActiveReplayRecord.Events.Num());
    while (ReplayNextEventIndex < ActiveReplayRecord.Events.Num()
        && ActiveReplayRecord.Events[ReplayNextEventIndex].TurnNumber == CurrentTurnNumber)
    {
        ApplyReplayEvent(ActiveReplayRecord.Events[ReplayNextEventIndex]);
        ++ReplayNextEventIndex;
    }

    SetVision();
    RefreshReplayVisionForPlayers();
    OnBoardStateChanged.Broadcast();

    if (ReplayNextEventIndex >= ActiveReplayRecord.Events.Num())
    {
        UE_LOG(LogTemp, Log, TEXT("Replay AdvanceReplay: reached end of event stream after turn=%d"), CurrentTurnNumber);
        StopReplay();
    }
}

void ARoundGameState::RefreshReplayVisionForPlayers() const
{
    if (!bReplayActive || !GetWorld())
    {
        return;
    }

    for (TActorIterator<ARoundPlayerPawn> It(GetWorld()); It; ++It)
    {
        if (ARoundPlayerPawn* PlayerPawn = *It)
        {
            PlayerPawn->UpdateVision();
        }
    }
}

TArray<FVector2D> ARoundGameState::BuildHexSpiralPositions(const FVector2D StartPosition, const int32 Radius) const
{
    TArray<FVector2D> Positions;
    if (Radius < 0)
    {
        return Positions;
    }

    const int32 EstimatedCount = 1 + (3 * Radius * (Radius + 1));
    Positions.Reserve(EstimatedCount);
    Positions.Add(StartPosition);

    if (Radius == 0)
    {
        return Positions;
    }

    const TArray<FVector2D> Directions = GetHexNeighborOffsets();
    int32 Ring = 1;

    while (Ring <= Radius)
    {
        FVector2D Current = StartPosition + FVector2D(-Ring, Ring);

        for (int32 DirectionIndex = 0; DirectionIndex < Directions.Num(); ++DirectionIndex)
        {
            for (int32 Step = 0; Step < Ring; ++Step)
            {
                Positions.Add(Current);
                Current += Directions[DirectionIndex];
            }
        }

        ++Ring;
    }

    return Positions;
}

ERoundTerrainType ARoundGameState::GetWeightedTerrainType(const TMap<ERoundTerrainType, float>& TerrainWeights) const
{
    if (TerrainWeights.Num() == 0)
    {
        return ERoundTerrainType::Ground;
    }

    bool bHasNonGroundTerrain = false;
    for (const TPair<ERoundTerrainType, float>& Pair : TerrainWeights)
    {
        if (Pair.Key != ERoundTerrainType::Ground && Pair.Value > 0.f)
        {
            bHasNonGroundTerrain = true;
            break;
        }
    }

    float TotalWeight = 0.f;
    for (const TPair<ERoundTerrainType, float>& Pair : TerrainWeights)
    {
        if (bHasNonGroundTerrain && Pair.Key == ERoundTerrainType::Ground)
        {
            continue;
        }

        if (Pair.Value > 0.f)
        {
            TotalWeight += Pair.Value;
        }
    }

    if (TotalWeight <= 0.f)
    {
        return ERoundTerrainType::Ground;
    }

    const_cast<ARoundGameState*>(this)->InitializeMatchRandomSeed();
    float RandomValue = const_cast<ARoundGameState*>(this)->GetRandomFraction(0.f, TotalWeight);
    for (const TPair<ERoundTerrainType, float>& Pair : TerrainWeights)
    {
        if (bHasNonGroundTerrain && Pair.Key == ERoundTerrainType::Ground)
        {
            continue;
        }

        if (Pair.Value <= 0.f)
        {
            continue;
        }

        RandomValue -= Pair.Value;
        if (RandomValue <= 0.f)
        {
            return Pair.Key;
        }
    }

    if (bHasNonGroundTerrain)
    {
        for (const TPair<ERoundTerrainType, float>& Pair : TerrainWeights)
        {
            if (Pair.Key != ERoundTerrainType::Ground && Pair.Value > 0.f)
            {
                return Pair.Key;
            }
        }
    }

    return TerrainWeights.CreateConstIterator()->Key;
}

void ARoundGameState::InvokeNoArgFunction(UObject* TargetObject, const FName FunctionName) const
{
    if (!TargetObject)
    {
        return;
    }

    if (UFunction* Function = TargetObject->FindFunction(FunctionName))
    {
        TargetObject->ProcessEvent(Function, nullptr);
    }
}

void ARoundGameState::NotifyVisionUpdated(UObject* PlayerObject) const
{
    if (!PlayerObject)
    {
        return;
    }

    if (ARoundPlayerPawn* PlayerPawn = Cast<ARoundPlayerPawn>(PlayerObject))
    {
        if (APlayerController* PlayerController = Cast<APlayerController>(PlayerPawn->GetController());
            PlayerController && PlayerController->IsLocalController())
        {
            PlayerPawn->UpdateVision();
        }
        else
        {
            PlayerPawn->ClientRefreshVision();
        }
        return;
    }

    InvokeNoArgFunction(PlayerObject, TEXT("UpdateVision"));
}

void ARoundGameState::TrySetObjectProperty(UObject* TargetObject, const FName PropertyName, UObject* Value) const
{
    if (!TargetObject)
    {
        return;
    }

    if (FObjectProperty* ObjectProperty = FindFProperty<FObjectProperty>(TargetObject->GetClass(), PropertyName))
    {
        ObjectProperty->SetObjectPropertyValue_InContainer(TargetObject, Value);
    }
}

void ARoundGameState::TrySetIntProperty(UObject* TargetObject, const FName PropertyName, int32 Value) const
{
    if (!TargetObject)
    {
        return;
    }

    if (FIntProperty* IntProperty = FindFProperty<FIntProperty>(TargetObject->GetClass(), PropertyName))
    {
        IntProperty->SetPropertyValue_InContainer(TargetObject, Value);
    }
}

void ARoundGameState::TrySetInt64Property(UObject* TargetObject, const FName PropertyName, int64 Value) const
{
    if (!TargetObject)
    {
        return;
    }

    if (FInt64Property* Int64Property = FindFProperty<FInt64Property>(TargetObject->GetClass(), PropertyName))
    {
        Int64Property->SetPropertyValue_InContainer(TargetObject, Value);
    }
}

void ARoundGameState::TrySetNameProperty(UObject* TargetObject, const FName PropertyName, FName Value) const
{
    if (!TargetObject)
    {
        return;
    }

    if (FNameProperty* NameProperty = FindFProperty<FNameProperty>(TargetObject->GetClass(), PropertyName))
    {
        NameProperty->SetPropertyValue_InContainer(TargetObject, Value);
    }
}

void ARoundGameState::ConfigureFieldForTerrain(ARoundGridTile* Field, const ERoundTerrainType TerrainType) const
{
    if (!Field)
    {
        return;
    }

    Field->SetTerrainType(TerrainType);
}

namespace
{
UWorld* ResolveRoundExportWorld()
{
    if (!GEngine)
    {
        return nullptr;
    }

    for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
    {
        UWorld* World = WorldContext.World();
        if (!World || !World->IsGameWorld())
        {
            continue;
        }

        if (World->GetGameState<ARoundGameState>())
        {
            return World;
        }
    }

    return nullptr;
}

int32 GetInitiativeMigrationScale()
{
    return 40;
}

bool ShouldExportInitiativeMigrationKey(const FString& Key)
{
    return Key.Contains(TEXT("Duration"))
        || Key.Contains(TEXT("CooldownTurns"));
}

FString BuildInitiativeMigrationKey(const FString& Key)
{
    FString Result = Key;
    Result.ReplaceInline(TEXT("CooldownTurns"), TEXT("CooldownInitiative"));
    Result.ReplaceInline(TEXT("DurationTurns"), TEXT("DurationInitiative"));

    if (!Result.Contains(TEXT("Initiative")) && Result.Contains(TEXT("Duration")))
    {
        Result += TEXT("Initiative");
    }

    return Result;
}

FString ResolveInitiativeMigrationExportPath(const FString& RequestedPath)
{
    if (!RequestedPath.IsEmpty())
    {
        return FPaths::IsRelative(RequestedPath)
            ? FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), RequestedPath)
            : RequestedPath;
    }

    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Migrations"), TEXT("initiative_value_migration.csv"));
}

bool ExportInitiativeMigrationSuggestions(const FString& RequestedPath, FString& OutResolvedPath)
{
    static const TArray<FString> SourceFiles = {
        FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/Round/Private/Data/RoundAbilityRegistryData.cpp")),
        FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/Round/Private/Data/RoundCharacterRegistryData.cpp"))
    };

    const FRegexPattern PairPattern(TEXT("TEXT\\(\"([^\"]+)\"\\)\\s*,\\s*(-?[0-9]+(?:\\.[0-9]+)?)"));
    FString Csv = TEXT("SourceFile,Line,OldKey,NewKey,OldValue,SuggestedInitiativeValue,Scale\r\n");

    for (const FString& SourceFile : SourceFiles)
    {
        FString Content;
        if (!FFileHelper::LoadFileToString(Content, *SourceFile))
        {
            continue;
        }

        TArray<FString> Lines;
        Content.ParseIntoArrayLines(Lines, true);
        for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
        {
            FRegexMatcher Matcher(PairPattern, Lines[LineIndex]);
            while (Matcher.FindNext())
            {
                const FString Key = Matcher.GetCaptureGroup(1);
                if (!ShouldExportInitiativeMigrationKey(Key))
                {
                    continue;
                }

                const FString ValueString = Matcher.GetCaptureGroup(2);
                const double OldValue = FCString::Atod(*ValueString);
                const double SuggestedValue = OldValue * static_cast<double>(GetInitiativeMigrationScale());
                Csv += FString::Printf(
                    TEXT("\"%s\",%d,\"%s\",\"%s\",%s,%.3f,%d\r\n"),
                    *SourceFile,
                    LineIndex + 1,
                    *Key,
                    *BuildInitiativeMigrationKey(Key),
                    *ValueString,
                    SuggestedValue,
                    GetInitiativeMigrationScale());
            }
        }
    }

    OutResolvedPath = ResolveInitiativeMigrationExportPath(RequestedPath);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutResolvedPath), true);
    return FFileHelper::SaveStringToFile(Csv, *OutResolvedPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
}

static FAutoConsoleCommand GLoadReplayCommand(
    TEXT("round.replay.load"),
    TEXT("Loads a recorded match JSON and replays one recorded turn per second. Arg: file path."),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
    {
        if (Args.Num() <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("round.replay.load requires a file path."));
            return;
        }

        UWorld* World = ResolveRoundExportWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("round.replay.load failed: no world available."));
            return;
        }

        ARoundGameState* RoundGameState = World->GetGameState<ARoundGameState>();
        if (!RoundGameState)
        {
            UE_LOG(LogTemp, Warning, TEXT("round.replay.load failed: RoundGameState not found."));
            return;
        }

        FString Error;
        if (!RoundGameState->LoadReplayFromFile(Args[0], Error))
        {
            UE_LOG(LogTemp, Warning, TEXT("round.replay.load failed: %s"), *Error);
            return;
        }

        UE_LOG(LogTemp, Log, TEXT("round.replay.load started: %s"), *Args[0]);
    }));

static FAutoConsoleCommand GExportRoundInitiativeMigrationCommand(
    TEXT("round.data.export_initiative_migration"),
    TEXT("Exports one-time CSV suggestions for migrating duration/cooldown turn values to initiative values. Optional arg: target file path."),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
    {
        FString SavedPath;
        const FString RequestedPath = Args.Num() > 0 ? Args[0] : FString();
        if (!ExportInitiativeMigrationSuggestions(RequestedPath, SavedPath))
        {
            UE_LOG(LogTemp, Warning, TEXT("round.data.export_initiative_migration failed."));
            return;
        }

        UE_LOG(LogTemp, Log, TEXT("Saved initiative migration suggestions to %s"), *SavedPath);
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                5.0f,
                FColor::Green,
                FString::Printf(TEXT("Initiative migration CSV saved: %s"), *SavedPath));
        }
    }));
