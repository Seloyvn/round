#include "Actors/RoundCharacter.h"

#include "Actors/RoundAbilityActor.h"
#include "Actors/RoundGridTile.h"
#include "Actors/RoundFieldObjectActor.h"
#include "Actors/RoundEffectActor.h"
#include "Actors/RoundCharacter5.h"
#include "Actors/RoundCharacter3.h"
#include "Actors/RoundCharacterPaladin.h"
#include "Actors/RoundCharacter6.h"
#include "Actors/RoundTeamActor.h"
#include "Actors/RoundTerrainActor.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundAbilityRegistryLookup.h"
#include "Data/RoundCharacterData.h"
#include "Data/RoundCharacterRegistryLookup.h"
#include "Data/RoundGameDataRegistryLookup.h"
#include "Data/RoundAbilityData.h"
#include "Data/RoundCharacterProgressionData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Game/RoundGameState.h"
#include "Game/RoundPlayerPawn.h"
#include "Interfaces/RoundBoardDamageableInterface.h"
#include "Engine/AssetManager.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Containers/Queue.h"
#include "Modules/ModuleManager.h"
#include "EngineUtils.h"
#include "Engine/DataTable.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "UI/RoundHealthbarWidget.h"
#include "Data/RoundSkillProgressionData.h"
#include "Effects/RoundModifierEffect.h"
#include "Effects/RoundAuraEffect.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundAbilityFlow, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogRoundCharacterVision, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogRoundCharacterData, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogRoundEffectRuntime, Log, All);

namespace
{
FName NormalizeCharacterIdForRuntime(const FName InCharacterId)
{
    FString Normalized = InCharacterId.ToString();
    Normalized.ReplaceInline(TEXT("BP_"), TEXT(""));
    Normalized.ReplaceInline(TEXT("_"), TEXT(""));
    Normalized.ReplaceInline(TEXT(" "), TEXT(""));
    Normalized.ReplaceInline(TEXT("Charcter"), TEXT("Character"));

    if (Normalized.IsNumeric())
    {
        return FName(*FString::Printf(TEXT("Character%s"), *Normalized));
    }

    return FName(*Normalized);
}

FString NormalizeSkillLookupKey(FString Value)
{
    Value = Value.TrimStartAndEnd();
    Value.ReplaceInline(TEXT("DA_Ability_"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("DA_SkillProgression_"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("BP_"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("_C"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("Round"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("Ability"), TEXT(""), ESearchCase::IgnoreCase);
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

bool IsExpectedNoAbilityCharacter(const URoundCharacterData* CharacterData)
{
    if (!CharacterData)
    {
        return false;
    }

    const FString CharacterId = CharacterData->CharacterId.ToString();
    return CharacterId.Equals(TEXT("Character6Puppet"), ESearchCase::IgnoreCase)
        || CharacterId.Contains(TEXT("Puppet"), ESearchCase::IgnoreCase)
        || CharacterData->GetName().Contains(TEXT("Puppet"), ESearchCase::IgnoreCase);
}

double FindNumericValueCaseInsensitive(const TMap<FString, double>& Values, const FString& Key, bool& bOutFound)
{
    if (const double* ExactValue = Values.Find(Key))
    {
        bOutFound = true;
        return *ExactValue;
    }

    for (const TPair<FString, double>& Pair : Values)
    {
        if (Pair.Key.Equals(Key, ESearchCase::IgnoreCase))
        {
            bOutFound = true;
            return Pair.Value;
        }
    }

    bOutFound = false;
    return 0.0;
}

TArray<FString> BuildInitiativeCompatibleLookupKeys(const FString& Key)
{
    TArray<FString> Keys;
    Keys.Add(Key);

    auto AddUniqueKey = [&Keys](FString Candidate)
    {
        if (!Candidate.IsEmpty())
        {
            Keys.AddUnique(MoveTemp(Candidate));
        }
    };

    FString Migrated = Key;
    Migrated.ReplaceInline(TEXT("CooldownTurns"), TEXT("CooldownInitiative"));
    Migrated.ReplaceInline(TEXT("DurationTurns"), TEXT("DurationInitiative"));
    if (!Migrated.Contains(TEXT("Initiative")) && Migrated.Contains(TEXT("Duration")))
    {
        Migrated += TEXT("Initiative");
    }
    AddUniqueKey(Migrated);

    FString Legacy = Key;
    Legacy.ReplaceInline(TEXT("CooldownInitiative"), TEXT("CooldownTurns"));
    Legacy.ReplaceInline(TEXT("DurationInitiative"), TEXT("DurationTurns"));
    if (Legacy.EndsWith(TEXT("Initiative")) && !Legacy.Contains(TEXT("CooldownTurns")) && !Legacy.Contains(TEXT("DurationTurns")))
    {
        Legacy.LeftChopInline(FCString::Strlen(TEXT("Initiative")));
    }
    AddUniqueKey(Legacy);

    return Keys;
}

int32 GetPackedTierSelectionFromMask(const int64 PackedValue, const int32 TierIndex)
{
    return static_cast<int32>((PackedValue >> (TierIndex * 2)) & 0x3);
}

TSubclassOf<ARoundAbilityActor> ResolveAbilityClassById(const FName AbilityId)
{
    static TMap<FString, TSubclassOf<ARoundAbilityActor>> CachedNativeAbilityClasses;
    static TMap<FString, TSubclassOf<ARoundAbilityActor>> CachedAnyAbilityClasses;
    if (CachedAnyAbilityClasses.Num() == 0)
    {
        for (TObjectIterator<UClass> It; It; ++It)
        {
            UClass* Class = *It;
            if (!IsValid(Class)
                || !Class->IsChildOf(ARoundAbilityActor::StaticClass())
                || Class == ARoundAbilityActor::StaticClass()
                  || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
            {
                continue;
            }

            const FString NormalizedKey = NormalizeSkillLookupKey(Class->GetName());
            CachedAnyAbilityClasses.Add(NormalizedKey, Class);

            if (!Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
            {
                CachedNativeAbilityClasses.Add(NormalizedKey, Class);
            }
        }
    }

    const FString AbilityKey = NormalizeSkillLookupKey(AbilityId.ToString());
    if (const TSubclassOf<ARoundAbilityActor>* FoundNativeClass = CachedNativeAbilityClasses.Find(AbilityKey))
    {
        return *FoundNativeClass;
    }

    if (TSubclassOf<ARoundAbilityActor> RegistryClass = RoundAbilityRegistry::FindAbilityClassById(AbilityId))
    {
        return RegistryClass;
    }

    if (const TSubclassOf<ARoundAbilityActor>* FoundClass = CachedAnyAbilityClasses.Find(AbilityKey))
    {
        return *FoundClass;
    }

    return nullptr;
}

}

ARoundCharacter::ARoundCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(true);
    Attributes = CreateDefaultSubobject<URoundAttributeSetComponent>(TEXT("Attributes"));
    HealthbarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthbarWidgetComponent"));
    HealthbarWidgetComponent->SetupAttachment(GetRootComponent());
    HealthbarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    HealthbarWidgetComponent->SetDrawAtDesiredSize(false);
    HealthbarWidgetComponent->SetDrawSize(FVector2D(120.0f, 28.0f));
    HealthbarWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
    HealthbarWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));
    HealthbarWidgetComponent->SetRelativeRotation(FRotator::ZeroRotator);
    HealthbarWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HealthbarWidgetComponent->SetGenerateOverlapEvents(false);
    HealthbarWidgetComponent->SetHiddenInGame(false);
    HealthbarWidgetComponent->SetVisibility(true);

    if (UClass* HealthbarWidgetClass = RoundGameDataRegistryLookup::ResolveHealthbarWidgetClass().Get())
    {
        HealthbarWidgetComponent->SetWidgetClass(HealthbarWidgetClass);
    }
}

void ARoundCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundCharacter, Team);
    DOREPLIFETIME(ARoundCharacter, GrantedAbilities);
    DOREPLIFETIME(ARoundCharacter, ActiveEffects);
    DOREPLIFETIME(ARoundCharacter, CurrentTile);
    DOREPLIFETIME(ARoundCharacter, OwningTeamActor);
    DOREPLIFETIME(ARoundCharacter, CharacterRowName);
    DOREPLIFETIME(ARoundCharacter, CharacterDataAsset);
    DOREPLIFETIME(ARoundCharacter, CharacterPassiveSelections);
    DOREPLIFETIME(ARoundCharacter, AbilitySkillSelections);
    DOREPLIFETIME(ARoundCharacter, NativeDelayedActionInitiative);
}

void ARoundCharacter::OnRep_CurrentTile()
{
    if (CurrentTile)
    {
        SetActorLocation(CurrentTile->GetActorLocation());
        UpdateFacingAfterTileChange(nullptr, CurrentTile);
    }

    if (ARoundGameState* RoundGameState = GetRoundGameState())
    {
        RoundGameState->OnBoardStateChanged.Broadcast();
    }

    if (!GetWorld())
    {
        return;
    }

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PlayerController = It->Get();
        if (!PlayerController || !PlayerController->IsLocalController())
        {
            continue;
        }

        if (ARoundPlayerPawn* PlayerPawn = Cast<ARoundPlayerPawn>(PlayerController->GetPawn()))
        {
            PlayerPawn->UpdateVision();
        }
    }
}

void ARoundCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (HealthbarWidgetComponent)
    {
        HealthbarWidgetComponent->InitWidget();
        HealthbarWidgetComponent->SetHiddenInGame(false);
        HealthbarWidgetComponent->SetVisibility(true);
    }
    InitializeCharacter();
    RefreshHealthbarWidget();

    if (ARoundGameState* RoundGameState = GetRoundGameState())
    {
        RoundGameState->RegisterCharacter(this);
    }
}

void ARoundCharacter::NotifyRoundStateChanged()
{
    if (ARoundGameState* RoundGameState = GetRoundGameState())
    {
        RoundGameState->OnBoardStateChanged.Broadcast();
    }
}

void ARoundCharacter::InitializeCharacter()
{
    if (!Attributes)
    {
        return;
    }

    LoadConfiguredCharacterData();

    if (CharacterData)
    {
        RebuildCharacterPassiveValues();
        if (HasAuthority() && AppliedCharacterData != CharacterData)
        {
            FRoundStatBlock EffectiveStats = CharacterData->BaseStats;
            ApplyCharacterPassiveStats(EffectiveStats);
            Attributes->InitializeFromStats(EffectiveStats);
            AppliedCharacterData = CharacterData;
        }
        else if (!HasAuthority() && AppliedCharacterData != CharacterData)
        {
            AppliedCharacterData = CharacterData;
        }

        if (GrantedAbilities.Num() == 0)
        {
            if (HasAuthority())
            {
                GrantDefaultAbilities();
            }
        }
        else
        {
            ApplyAbilitySkillSelectionsToGrantedAbilities();
        }

        RefreshHealthbarWidget();
    }
}

void ARoundCharacter::GrantDefaultAbilities()
{
    if (!CharacterData || !GetWorld())
    {
        UE_LOG(LogRoundCharacterData, Error, TEXT("GrantDefaultAbilities failed - Character=%s CharacterData=%s"),
            *GetName(),
            *GetNameSafe(CharacterData));
        return;
    }

    if (CharacterData->Abilities.Num() == 0)
    {
        if (!IsExpectedNoAbilityCharacter(CharacterData))
        {
            UE_LOG(LogRoundCharacterData, Error, TEXT("CharacterData has no abilities - Character=%s CharacterData=%s CharacterId=%s"),
                *GetName(),
                *GetNameSafe(CharacterData),
                *CharacterData->CharacterId.ToString());
        }
        return;
    }

    for (ARoundAbilityActor* ExistingAbility : GrantedAbilities)
    {
        if (ExistingAbility && ExistingAbility->IsValidLowLevel())
        {
            ExistingAbility->Destroy();
        }
    }
    GrantedAbilities.Empty();
    TSet<FName> GrantedAbilityIds;
    const int32 AllowedAbilityCount = GetAllowedAbilityCount();
    const int32 AbilityCountToGrant = AllowedAbilityCount <= 0
        ? 0
        : FMath::Min(AllowedAbilityCount, CharacterData->Abilities.Num());

    for (int32 AbilityIndex = 0; AbilityIndex < AbilityCountToGrant; ++AbilityIndex)
    {
        URoundAbilityData* AbilityData = CharacterData->Abilities[AbilityIndex];
        if (!AbilityData)
        {
            UE_LOG(LogRoundCharacterData, Error, TEXT("Null AbilityData in CharacterData - Character=%s CharacterData=%s"),
                *GetName(),
                *GetNameSafe(CharacterData));
            continue;
        }

        URoundAbilityData* ResolvedAbilityData = AbilityData;
        const FName AbilityId = ResolvedAbilityData->AbilityId;
        if (!AbilityId.IsNone() && GrantedAbilityIds.Contains(AbilityId))
        {
            UE_LOG(LogRoundCharacterData, Warning, TEXT("Skipping duplicate AbilityData - Character=%s AbilityId=%s CharacterData=%s"),
                *GetName(),
                *AbilityId.ToString(),
                *GetNameSafe(CharacterData));
            continue;
        }

        TSubclassOf<ARoundAbilityActor> AbilityClass = ResolvedAbilityData->AbilityClass;
        if (!AbilityClass)
        {
            AbilityClass = ResolveAbilityClassById(ResolvedAbilityData->AbilityId);
        }
        if (!AbilityClass)
        {
            UE_LOG(LogRoundCharacterData, Error, TEXT("No AbilityClass found - Character=%s AbilityId=%s AbilityData=%s"),
                *GetName(),
                *ResolvedAbilityData->AbilityId.ToString(),
                *GetNameSafe(ResolvedAbilityData));
            continue;
        }

        ARoundAbilityActor* AbilityActor = GetWorld()->SpawnActor<ARoundAbilityActor>(AbilityClass);
        if (!AbilityActor)
        {
            UE_LOG(LogRoundCharacterData, Error, TEXT("Failed to spawn AbilityActor - Character=%s AbilityClass=%s AbilityId=%s"),
                *GetName(),
                *GetNameSafe(AbilityClass),
                *ResolvedAbilityData->AbilityId.ToString());
            continue;
        }

        AbilityActor->AbilityData = ResolvedAbilityData;
        AbilityActor->OwningCharacter = this;
        AbilityActor->RowName = ResolvedAbilityData->AbilityId;
        AbilityActor->OnGrantedToCharacter();
        AbilityActor->InitializeCharges();
        AbilityActor->RefreshRuntimeState();
        AbilityActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
        GrantedAbilities.Add(AbilityActor);
        UE_LOG(
            LogRoundCharacterData,
            Log,
            TEXT("GrantDefaultAbilities Character=%s AbilityIndex=%d AbilityId=%s RuntimeClass=%s DataClass=%s Asset=%s"),
            *GetName(),
            AbilityIndex,
            *ResolvedAbilityData->AbilityId.ToString(),
            *GetNameSafe(AbilityActor->GetClass()),
            *GetNameSafe(ResolvedAbilityData->AbilityClass),
            *GetNameSafe(ResolvedAbilityData));
        if (!AbilityId.IsNone())
        {
            GrantedAbilityIds.Add(AbilityId);
        }
    }

    ApplyAbilitySkillSelectionsToGrantedAbilities();
}

int32 ARoundCharacter::GetAllowedAbilityCount() const
{
    return CharacterData ? CharacterData->Abilities.Num() : 0;
}

void ARoundCharacter::OnRep_CharacterRowName()
{
    InitializeCharacter();
}

void ARoundCharacter::OnRep_CharacterDataAsset()
{
    InitializeCharacter();
}

void ARoundCharacter::OnRep_CharacterPassiveSelections()
{
    InitializeCharacter();
}

void ARoundCharacter::OnRep_AbilitySkillSelections()
{
    InitializeCharacter();
}

void ARoundCharacter::OnRep_GrantedAbilities()
{
    ApplyAbilitySkillSelectionsToGrantedAbilities();
    NotifyRoundStateChanged();
}

TSoftObjectPtr<URoundCharacterData> ARoundCharacter::ResolveCharacterDataAsset() const
{
    if (!CharacterDataAsset.ToSoftObjectPath().IsValid() && !CharacterRowName.IsNone())
    {
        const TSoftObjectPtr<URoundCharacterData> ResolvedAsset = RoundCharacterRegistry::FindCharacterDataAssetById(CharacterRowName);
        if (ResolvedAsset.ToSoftObjectPath().IsValid())
        {
            return ResolvedAsset;
        }
    }

    return CharacterDataAsset;
}

void ARoundCharacter::LoadConfiguredCharacterData()
{
    TSoftObjectPtr<URoundCharacterData> ConfiguredAsset = ResolveCharacterDataAsset();
    if (!ConfiguredAsset.ToSoftObjectPath().IsValid())
    {
        UE_LOG(LogRoundCharacterData, Error, TEXT("No CharacterDataAsset configured - Character=%s Class=%s Row=%s"),
            *GetName(),
            *GetClass()->GetName(),
            *CharacterRowName.ToString());
        return;
    }

    URoundCharacterData* LoadedCharacterData = ConfiguredAsset.LoadSynchronous();
    if (!LoadedCharacterData)
    {
        UE_LOG(LogRoundCharacterData, Error, TEXT("Failed to load CharacterDataAsset - Character=%s Path=%s"),
            *GetName(),
            *ConfiguredAsset.ToSoftObjectPath().ToString());
        return;
    }

    CharacterData = LoadedCharacterData;
    if (CharacterData->Abilities.Num() == 0)
    {
        if (!IsExpectedNoAbilityCharacter(CharacterData))
        {
            UE_LOG(LogRoundCharacterData, Error, TEXT("Loaded CharacterData without abilities - Character=%s CharacterData=%s CharacterId=%s"),
                *GetName(),
                *GetNameSafe(CharacterData),
                *CharacterData->CharacterId.ToString());
        }
    }
}

bool ARoundCharacter::TryResolveCharacterPassiveLevels(TArray<FRoundSkillpointDetails>& OutLevels) const
{
    OutLevels.Reset();

    if (!CharacterData)
    {
        return false;
    }

    if (CharacterData->CharacterProgression)
    {
        for (const FRoundSkillpointTier& Tier : CharacterData->CharacterProgression->Tiers)
        {
            OutLevels.Append(Tier.Choices);
        }

        return OutLevels.Num() > 0;
    }

    if (!CharacterData->SkillpointsTable)
    {
        return false;
    }

    UDataTable* SkillpointTable = CharacterData->SkillpointsTable;
    if (SkillpointTable->GetRowStruct() == FRoundSkillpointDetails::StaticStruct())
    {
        TArray<FRoundSkillpointDetails*> Rows;
        SkillpointTable->GetAllRows(TEXT("RoundCharacter.ResolveCharacterPassiveLevels"), Rows);
        for (const FRoundSkillpointDetails* Row : Rows)
        {
            if (Row)
            {
                OutLevels.Add(*Row);
            }
        }
    }
    else if (SkillpointTable->GetRowStruct() == FRoundSkillpointsDetails::StaticStruct())
    {
        TArray<FRoundSkillpointsDetails*> Rows;
        SkillpointTable->GetAllRows(TEXT("RoundCharacter.ResolveCharacterPassiveLevels"), Rows);
        for (const FRoundSkillpointsDetails* Row : Rows)
        {
            if (Row)
            {
                OutLevels.Append(Row->Skillpoints);
            }
        }
    }

    return OutLevels.Num() > 0;
}

void ARoundCharacter::ApplyAbilitySkillSelectionsToGrantedAbilities()
{
    for (ARoundAbilityActor* AbilityActor : GrantedAbilities)
    {
        if (!AbilityActor)
        {
            continue;
        }

        AbilityActor->OwningCharacter = this;

        if (!AbilityActor->AbilityData && CharacterData)
        {
            const FName RequestedAbilityId = AbilityActor->RowName;
            TObjectPtr<URoundAbilityData>* MatchingAbilityData = CharacterData->Abilities.FindByPredicate(
                [RequestedAbilityId](const URoundAbilityData* Candidate)
                {
                    return Candidate && Candidate->AbilityId == RequestedAbilityId;
                });
            if (MatchingAbilityData)
            {
                AbilityActor->AbilityData = *MatchingAbilityData;
            }
        }

        const FName AbilityId = AbilityActor->AbilityData ? AbilityActor->AbilityData->AbilityId : AbilityActor->RowName;
        AbilityActor->Points = ResolveAbilitySkillMask(AbilityId);
    }
}

int64 ARoundCharacter::ResolveAbilitySkillMask(FName AbilityId) const
{
    const FString AbilityKey = NormalizeSkillLookupKey(AbilityId.ToString());
    for (const FRoundSkillEntry& SkillEntry : AbilitySkillSelections)
    {
        if (NormalizeSkillLookupKey(SkillEntry.Name) == AbilityKey)
        {
            return SkillEntry.Points;
        }
    }

    return 0;
}

void ARoundCharacter::RebuildCharacterPassiveValues()
{
    CharacterPassiveValues.Reset();

    if (!CharacterData || CharacterPassiveSelections == 0)
    {
        return;
    }

    TArray<FRoundSkillpointDetails> PassiveLevels;
    if (!TryResolveCharacterPassiveLevels(PassiveLevels))
    {
        UE_LOG(LogRoundCharacterData, Error, TEXT("Character passive selections configured but no SkillpointsTable rows found - Character=%s CharacterData=%s"),
            *GetName(),
            *GetNameSafe(CharacterData));
        return;
    }

    for (int32 TierIndex = 0; TierIndex < 4; ++TierIndex)
    {
        const int32 Selection = GetPackedTierSelectionFromMask(CharacterPassiveSelections, TierIndex);
        if (Selection <= 0)
        {
            break;
        }

        const int32 RowIndex = (TierIndex * 3) + (Selection - 1);
        if (!PassiveLevels.IsValidIndex(RowIndex))
        {
            UE_LOG(LogRoundCharacterData, Error, TEXT("Character passive selection points to missing row - Character=%s Tier=%d Selection=%d RowIndex=%d Table=%s"),
                *GetName(),
                TierIndex,
                Selection,
                RowIndex,
                *GetNameSafe(CharacterData->SkillpointsTable));
            continue;
        }

        for (const TPair<FString, double>& Pair : PassiveLevels[RowIndex].Values)
        {
            CharacterPassiveValues.FindOrAdd(Pair.Key) += Pair.Value;
        }
    }
}

void ARoundCharacter::ApplyCharacterPassiveStats(FRoundStatBlock& InOutStats) const
{
    auto GetPassiveInt = [this](const TCHAR* Key) -> int32
    {
        return FMath::RoundToInt(GetPassiveValueFromKey(Key));
    };

    InOutStats.MaxHealth = FMath::Max(1, InOutStats.MaxHealth + GetPassiveInt(TEXT("MaxHealthAdd")));
    InOutStats.Attack = FMath::Max(0, InOutStats.Attack + GetPassiveInt(TEXT("AttackAdd")));
    InOutStats.Defense = FMath::Max(0, InOutStats.Defense + GetPassiveInt(TEXT("DefenseAdd")));
    InOutStats.MaxInitiative = FMath::Max(1, InOutStats.MaxInitiative + GetPassiveInt(TEXT("MaxInitiativeAdd")));
    // Give every character one additional base vision tile. Character-specific
    // progression remains additive on top through VisionRangeAdd.
    InOutStats.VisionRange = FMath::Max(0, InOutStats.VisionRange +  GetPassiveInt(TEXT("VisionRangeAdd")));
    InOutStats.AttackRange = FMath::Max(1, InOutStats.AttackRange + GetPassiveInt(TEXT("AttackRangeAdd")));
    InOutStats.MoveCost = FMath::Max(1, InOutStats.MoveCost + GetPassiveInt(TEXT("MoveCostAdd")));
    InOutStats.AttackCost = FMath::Max(1, InOutStats.AttackCost + GetPassiveInt(TEXT("AttackCostAdd")));
    InOutStats.TurnDelay = FMath::Max(0, InOutStats.TurnDelay + GetPassiveInt(TEXT("TurnDelayAdd")));
}

double ARoundCharacter::GetPassiveValueFromKey(const FString& Key) const
{
    const TArray<FString> CandidateKeys = BuildInitiativeCompatibleLookupKeys(Key);
    for (const FString& CandidateKey : CandidateKeys)
    {
        bool bFoundValue = false;
        const double FoundValue = FindNumericValueCaseInsensitive(CharacterPassiveValues, CandidateKey, bFoundValue);
        if (bFoundValue)
        {
            return FoundValue;
        }
    }

    return 0.0;
}

double ARoundCharacter::GetIncomingDamageMultiplier(const ERoundDamageType DamageType) const
{
    double Multiplier = 1.0;
    Multiplier *= GetPassiveValueFromKey(TEXT("AllDamageMultiplier")) > 0.0 ? GetPassiveValueFromKey(TEXT("AllDamageMultiplier")) : 1.0;

    if (DamageType == ERoundDamageType::Fire || DamageType == ERoundDamageType::Ice || DamageType == ERoundDamageType::Lightning)
    {
        const double ElementalMultiplier = GetPassiveValueFromKey(TEXT("ElementalDamageMultiplier"));
        if (ElementalMultiplier > 0.0)
        {
            Multiplier *= ElementalMultiplier;
        }
    }

    const TCHAR* TypeKey = nullptr;
    switch (DamageType)
    {
    case ERoundDamageType::Physical: TypeKey = TEXT("PhysicalDamageMultiplier"); break;
    case ERoundDamageType::Magical: TypeKey = TEXT("MagicalDamageMultiplier"); break;
    case ERoundDamageType::Fire: TypeKey = TEXT("FireDamageMultiplier"); break;
    case ERoundDamageType::Ice: TypeKey = TEXT("IceDamageMultiplier"); break;
    case ERoundDamageType::Lightning: TypeKey = TEXT("LightningDamageMultiplier"); break;
    case ERoundDamageType::Holy: TypeKey = TEXT("HolyDamageMultiplier"); break;
    case ERoundDamageType::Nature: TypeKey = TEXT("NatureDamageMultiplier"); break;
    case ERoundDamageType::TrueDamage: TypeKey = TEXT("TrueDamageMultiplier"); break;
    default: break;
    }

    if (TypeKey)
    {
        const double TypeMultiplier = GetPassiveValueFromKey(TypeKey);
        if (TypeMultiplier > 0.0)
        {
            Multiplier *= TypeMultiplier;
        }
    }

    return FMath::Max(0.0, Multiplier);
}

int32 ARoundCharacter::GetIncomingBonusDefense(const ERoundDamageType DamageType) const
{
    return 0;
}

int32 ARoundCharacter::ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const
{
    int32 ModifiedDamage = FMath::Max(0, Amount);
    const TArray<TObjectPtr<ARoundEffectActor>> EffectsSnapshot = ActiveEffects;
    for (ARoundEffectActor* Effect : EffectsSnapshot)
    {
        if (!Effect)
        {
            continue;
        }

        ModifiedDamage = Effect->ModifyOutgoingDamage(const_cast<ARoundCharacter*>(this), TargetCharacter, ModifiedDamage, DamageType, DefenseIgnore);
    }

    return FMath::Max(0, ModifiedDamage);
}

void ARoundCharacter::HandlePassiveDamageTaken(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType)
{
}

void ARoundCharacter::HandleEnemyHitByCharacter(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType)
{
}

void ARoundCharacter::HandleDamageDealt(ARoundCharacter* EnemyCharacter, int32 AppliedDamage, ERoundDamageType DamageType)
{
}

bool ARoundCharacter::TryPreventDeathFromPassive(int32 AppliedDamage, ARoundCharacter* SourceCharacter, ERoundDamageType DamageType)
{
    return false;
}

int32 ARoundCharacter::Heal(int32 Amount)
{
    if (!Attributes)
    {
        return 0;
    }

    const int32 PreviousHealth = Attributes->CurrentHealth;
    Attributes->Heal(Amount);
    RefreshHealthbarWidget();
    const int32 HealedAmount = FMath::Max(0, Attributes->CurrentHealth - PreviousHealth);
    if (HealedAmount > 0)
    {
        if (ARoundGameState* RoundGameState = GetRoundGameState())
        {
            RoundGameState->RecordHeal(this, HealedAmount);
        }
    }
    return HealedAmount;
}

void ARoundCharacter::ApplyDelay(int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }

    if (Attributes)
    {
        Attributes->PassInitiative(Amount);
    }

    if (ARoundGameState* RoundGameState = GetRoundGameState())
    {
        RoundGameState->RecordDelayApplied(this, Amount);
    }

    NotifyRoundStateChanged();
}

int32 ARoundCharacter::ApplyRoundDamage(int32 Amount, ERoundDamageType DamageType, int32 DefenseIgnore, ARoundCharacter* SourceCharacter, bool& bDidDie)
{
    bDidDie = false;

    if (!Attributes)
    {
        return 0;
    }

    int32 ModifiedDamage = FMath::Max(0, Amount);
    const int32 PreMitigationInputDamage = ModifiedDamage;

    const TArray<TObjectPtr<ARoundEffectActor>> EffectsSnapshot = ActiveEffects;
    for (ARoundEffectActor* Effect : EffectsSnapshot)
    {
        if (!Effect)
        {
            continue;
        }

        ModifiedDamage = FMath::Clamp(
            Effect->ModifyIncomingDamageBeforeMitigation(this, SourceCharacter, ModifiedDamage, DamageType),
            0,
            PreMitigationInputDamage);
    }

    if (ARoundTerrainActor* TerrainActor = CurrentTile ? Cast<ARoundTerrainActor>(CurrentTile->TerrainObject) : nullptr)
    {
        ModifiedDamage = FMath::Max(0, TerrainActor->ModifyIncomingDamage(this, SourceCharacter, ModifiedDamage, DamageType));
    }

    if (GetWorld())
    {
        for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
        {
            if (ARoundCharacter* AuraOwner = *It)
            {
                for (ARoundEffectActor* Effect : AuraOwner->ActiveEffects)
                {
                    if (ARoundAuraEffect* AuraEffect = Cast<ARoundAuraEffect>(Effect))
                    {
                        ModifiedDamage = AuraEffect->ModifyIncomingDamageForCharacter(this, SourceCharacter, ModifiedDamage, DamageType, DefenseIgnore);
                    }
                }
            }
        }
    }

    for (ARoundEffectActor* Effect : EffectsSnapshot)
    {
        if (!Effect)
        {
            continue;
        }

        ModifiedDamage = Effect->ModifyIncomingDamage(this, SourceCharacter, ModifiedDamage, DamageType, DefenseIgnore);
    }

    ModifiedDamage = FMath::RoundToInt(static_cast<double>(ModifiedDamage) * GetIncomingDamageMultiplier(DamageType));
    if (const ARoundGameState* RoundGameState = GetRoundGameState())
    {
        ModifiedDamage = FMath::RoundToInt(static_cast<double>(ModifiedDamage) * RoundGameState->GetGlobalDamageEscalationMultiplier());
    }
    const int32 BonusDefense = GetIncomingBonusDefense(DamageType);
    const int32 RemainingDefense = FMath::Max(0, (Attributes->BaseStats.Defense + BonusDefense) - FMath::Max(0, DefenseIgnore));
    const double DamageMultiplier = 100.0 / (100.0 + static_cast<double>(RemainingDefense));
    const int32 FinalDamage = FMath::RoundToInt(static_cast<double>(ModifiedDamage) * DamageMultiplier);
    const int32 AppliedDamage = Attributes->ApplyFinalDamage(FinalDamage);
    if (AppliedDamage > 0)
    {
        if (SourceCharacter)
        {
            SourceCharacter->HandleDamageDealt(this, AppliedDamage, DamageType);
        }
        if (ARoundCharacter5* Character5Source = Cast<ARoundCharacter5>(SourceCharacter))
        {
            Character5Source->GainBlood(AppliedDamage);
        }
    }

    for (ARoundEffectActor* Effect : EffectsSnapshot)
    {
        if (Effect)
        {
            Effect->OnDamageTaken(this, SourceCharacter, AppliedDamage, DamageType);
        }
    }

    HandlePassiveDamageTaken(AppliedDamage, SourceCharacter, DamageType);

    if (!Attributes->IsAlive())
    {
        if (TryPreventDeathFromPassive(AppliedDamage, SourceCharacter, DamageType) && Attributes->IsAlive())
        {
            RefreshHealthbarWidget();
            return AppliedDamage;
        }

        bDidDie = true;
        OnCharacterDied.Broadcast(this);
        Die();
    }

    RefreshHealthbarWidget();

    return AppliedDamage;
}

int32 ARoundCharacter::ApplyDirectDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* SourceCharacter, bool& bDidDie)
{
    bDidDie = false;

    if (!Attributes)
    {
        return 0;
    }

    const int32 AppliedDamage = Attributes->ApplyFinalDamage(FMath::Max(0, Amount));
    if (AppliedDamage > 0)
    {
        if (SourceCharacter)
        {
            SourceCharacter->HandleDamageDealt(this, AppliedDamage, DamageType);
        }
        if (ARoundCharacter5* Character5Source = Cast<ARoundCharacter5>(SourceCharacter))
        {
            Character5Source->GainBlood(AppliedDamage);
        }
    }

    const TArray<TObjectPtr<ARoundEffectActor>> EffectsSnapshot = ActiveEffects;
    for (ARoundEffectActor* Effect : EffectsSnapshot)
    {
        if (Effect)
        {
            Effect->OnDamageTaken(this, SourceCharacter, AppliedDamage, DamageType);
        }
    }

    HandlePassiveDamageTaken(AppliedDamage, SourceCharacter, DamageType);

    if (!Attributes->IsAlive())
    {
        if (TryPreventDeathFromPassive(AppliedDamage, SourceCharacter, DamageType) && Attributes->IsAlive())
        {
            RefreshHealthbarWidget();
            return AppliedDamage;
        }

        bDidDie = true;
        OnCharacterDied.Broadcast(this);
        Die();
    }

    RefreshHealthbarWidget();
    return AppliedDamage;
}

int32 ARoundCharacter::ApplyUnreactiveFinalDamage(int32 Amount, bool& bDidDie)
{
    bDidDie = false;
    if (!Attributes) return 0;

    const int32 AppliedDamage = Attributes->ApplyFinalDamage(FMath::Max(0, Amount));
    if (!Attributes->IsAlive())
    {
        bDidDie = true;
        OnCharacterDied.Broadcast(this);
        Die();
    }
    RefreshHealthbarWidget();
    return AppliedDamage;
}

FRoundActionResult ARoundCharacter::AttackField(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (HasTurnActionBlock())
    {
        Result.Reason = TEXT("Character cannot act.");
        return Result;
    }

    if (!TargetTile)
    {
        Result.Reason = TEXT("Invalid attack tile.");
        return Result;
    }

    ARoundCharacter* TargetCharacter = TargetTile->OccupyingCharacter;
    AActor* TargetBoardObject = TargetTile->OccupyingFieldObject.Get();
    if (!TargetBoardObject || !TargetBoardObject->GetClass()->ImplementsInterface(URoundBoardDamageableInterface::StaticClass()) || !IRoundBoardDamageableInterface::Execute_CanReceiveBoardDamage(TargetBoardObject))
    {
        TargetBoardObject = TargetTile->TerrainObject;
        if (!TargetBoardObject || !TargetBoardObject->GetClass()->ImplementsInterface(URoundBoardDamageableInterface::StaticClass()) || !IRoundBoardDamageableInterface::Execute_CanReceiveBoardDamage(TargetBoardObject))
        {
            TargetBoardObject = nullptr;
        }
    }

    if (TargetCharacter && !EnemyField(TargetTile))
    {
        Result.Reason = TEXT("Target is not an enemy.");
        return Result;
    }

    if (!CurrentTile)
    {
        Result.Reason = TEXT("Character has no current tile.");
        return Result;
    }

    ARoundGameState* RoundGameState = GetRoundGameState();
    if (!RoundGameState)
    {
        Result.Reason = TEXT("No game state available.");
        return Result;
    }

    const bool bInRange = CurrentTile->GetDistance(TargetTile) <= GetAttackRange();
    if (!bInRange || ((!TargetCharacter || !EnemyField(TargetTile)) && !TargetBoardObject))
    {
        Result.Reason = TEXT("Target is not a legal action.");
        return Result;
    }

    FaceTargetTile(TargetTile);

    FRoundCoreBasicAttackPreview CorePreview;
    FString CorePreviewReason;
    const bool bHasCorePreview = TargetCharacter
        && RoundGameState->PreviewCoreBasicAttack(this, TargetCharacter, CorePreview, CorePreviewReason);
    const int32 InitiativeBeforeAttack = Attributes ? Attributes->CurrentInitiative : 0;
    const int32 TargetHealthBeforeAttack = TargetCharacter && TargetCharacter->Attributes
        ? TargetCharacter->Attributes->CurrentHealth
        : 0;

    if (!Attributes || !Attributes->SpendInitiative(GetAttackCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    int32 DefenseIgnore = 0;
    int32 Damage = Attributes->BaseStats.Attack;
    if (FMath::FRand() < GetCritChance())
    {
        Damage = FMath::RoundToInt(static_cast<double>(Damage) * GetCritModifier());
    }

    int32 DamageDone = 0;
    bool bDidDie = false;
    if (TargetCharacter)
    {
        const TArray<TObjectPtr<ARoundEffectActor>> BasicAttackEffectsSnapshot = ActiveEffects;
        for (ARoundEffectActor* Effect : BasicAttackEffectsSnapshot)
        {
            if (Effect) Damage = Effect->ModifyBasicAttackDamage(this, TargetCharacter, Damage);
        }
        Damage = ModifyOutgoingDamage(Damage, ERoundDamageType::Physical, TargetCharacter, DefenseIgnore);
        DamageDone = TargetCharacter->ApplyRoundDamage(Damage, ERoundDamageType::Physical, DefenseIgnore, this, bDidDie);
        HandleEnemyHitByCharacter(TargetCharacter, DamageDone, ERoundDamageType::Physical);
    }
    else if (TargetBoardObject)
    {
        Damage = ModifyOutgoingDamage(Damage, ERoundDamageType::Physical, nullptr, DefenseIgnore);
        const FRoundBoardDamageResult BoardDamageResult = IRoundBoardDamageableInterface::Execute_ApplyBoardDamage(
            TargetBoardObject,
            Damage,
            ERoundDamageType::Physical,
            DefenseIgnore,
            this);
        DamageDone = BoardDamageResult.AppliedDamage;
        bDidDie = BoardDamageResult.bDestroyed;
    }

    Result.bSuccess = true;
    Result.Reason = FString::Printf(TEXT("Attack executed for %d damage."), DamageDone);
    if (TargetCharacter)
    {
        RoundGameState->RecordBasicAttack(this, TargetCharacter, DamageDone);
    }

    const TArray<TObjectPtr<ARoundEffectActor>> AttackEffectsSnapshot = ActiveEffects;
    for (ARoundEffectActor* Effect : AttackEffectsSnapshot)
    {
        if (Effect) Effect->OnBasicAttackExecuted(this, TargetCharacter);
    }

    if (TargetCharacter)
    {
        if (!bHasCorePreview)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("RoundCoreAttackParity PreviewFailed Attacker=%s Target=%s Reason=%s"),
                *GetName(), *TargetCharacter->GetName(), *CorePreviewReason);
        }
        else
        {
            const int32 ActorInitiativeCost = FMath::Max(
                0, InitiativeBeforeAttack - (Attributes ? Attributes->CurrentInitiative : InitiativeBeforeAttack));
            const int32 ActorDamage = FMath::Max(
                0, TargetHealthBeforeAttack - (TargetCharacter->Attributes
                    ? TargetCharacter->Attributes->CurrentHealth
                    : TargetHealthBeforeAttack));
            const bool bParity = ActorInitiativeCost == CorePreview.InitiativeCost
                && ActorDamage == CorePreview.Damage
                && bDidDie == CorePreview.bTargetDied;
            if (bParity)
            {
                UE_LOG(
                    LogTemp,
                    Verbose,
                    TEXT("RoundCoreAttackParity Match Attacker=%s Target=%s ActorCost=%d CoreCost=%d ActorDamage=%d CoreDamage=%d ActorDied=%d CoreDied=%d CoreCritical=%d"),
                    *GetName(), *TargetCharacter->GetName(),
                    ActorInitiativeCost, CorePreview.InitiativeCost,
                    ActorDamage, CorePreview.Damage,
                    bDidDie ? 1 : 0, CorePreview.bTargetDied ? 1 : 0,
                    CorePreview.bCritical ? 1 : 0);
            }
            else
            {
                UE_LOG(
                    LogTemp,
                    Warning,
                    TEXT("RoundCoreAttackParity Mismatch Attacker=%s Target=%s ActorCost=%d CoreCost=%d ActorDamage=%d CoreDamage=%d ActorDied=%d CoreDied=%d CoreCritical=%d"),
                    *GetName(), *TargetCharacter->GetName(),
                    ActorInitiativeCost, CorePreview.InitiativeCost,
                    ActorDamage, CorePreview.Damage,
                    bDidDie ? 1 : 0, CorePreview.bTargetDied ? 1 : 0,
                    CorePreview.bCritical ? 1 : 0);
            }
        }
    }

    RoundGameState->SetVision();
    RoundGameState->OnBoardStateChanged.Broadcast();

    return Result;
}

FRoundActionResult ARoundCharacter::MoveToTile(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (HasTurnActionBlock())
    {
        Result.Reason = TEXT("Character cannot act.");
        return Result;
    }

    if (HasMovementBlock())
    {
        Result.Reason = TEXT("Character cannot move.");
        return Result;
    }

    if (!TargetTile || !Attributes)
    {
        Result.Reason = TEXT("Invalid move target.");
        return Result;
    }

    ARoundGameState* RoundGameState = GetRoundGameState();
    if (!RoundGameState)
    {
        Result.Reason = TEXT("No game state available.");
        return Result;
    }

    return RoundGameState->ExecuteCoreMove(this, TargetTile);
}

FRoundActionResult ARoundCharacter::MoveAlongPath(const TArray<ARoundGridTile*>& Path)
{
    FRoundActionResult Result;
    if (HasMovementBlock())
    {
        Result.Reason = TEXT("Character cannot move.");
        return Result;
    }

    if (!Attributes || !CurrentTile || Path.Num() == 0)
    {
        Result.Reason = TEXT("Invalid move path.");
        return Result;
    }

    ARoundGameState* RoundGameState = GetRoundGameState();
    if (!RoundGameState)
    {
        Result.Reason = TEXT("Missing game state.");
        return Result;
    }

    return RoundGameState->ExecuteCoreMovePath(this, Path);
}

void ARoundCharacter::SetField(ARoundGridTile* NewTile, bool bUpdateUI)
{
    if (!NewTile)
    {
        return;
    }

    if (CurrentTile == NewTile)
    {
        return;
    }

    if (ARoundCharacter* ExistingOccupant = NewTile->OccupyingCharacter)
    {
        if (ExistingOccupant != this)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("SetField blocked occupied destination - Character=%s CurrentTile=%s Destination=%s Occupant=%s"),
                *GetNameSafe(this),
                *GetNameSafe(CurrentTile.Get()),
                *GetNameSafe(NewTile),
                *GetNameSafe(ExistingOccupant));
            return;
        }
    }

    ARoundGridTile* PreviousTile = CurrentTile;

    if (CurrentTile && CurrentTile->OccupyingCharacter == this)
    {
        CurrentTile->OccupyingCharacter = nullptr;
        CurrentTile->ForceNetUpdate();
    }

    SetActorLocation(NewTile->GetActorLocation());
    NewTile->OccupyingCharacter = this;
    NewTile->ForceNetUpdate();
    CurrentTile = NewTile;
    UpdateFacingAfterTileChange(PreviousTile, NewTile);
    ForceNetUpdate();

    const TArray<TObjectPtr<ARoundEffectActor>> EffectsSnapshot = ActiveEffects;
    for (ARoundEffectActor* Effect : EffectsSnapshot)
    {
        if (Effect)
        {
            Effect->OnOwnerMoved(this, PreviousTile, NewTile);
        }
    }

    if (GetWorld())
    {
        for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
        {
            ARoundCharacter* Candidate = *It;
            if (!Candidate)
            {
                continue;
            }

            const TArray<TObjectPtr<ARoundEffectActor>> CandidateEffectsSnapshot = Candidate->ActiveEffects;
            for (ARoundEffectActor* Effect : CandidateEffectsSnapshot)
            {
                if (Effect)
                {
                    Effect->OnOtherCharacterMoved(Candidate, this, PreviousTile, NewTile);
                }
            }
        }
    }

    if (HasAuthority())
    {
        NewTile->NotifyCharacterEntered(this, PreviousTile);
    }

    if (bUpdateUI)
    {
        if (ARoundGameState* RoundGameState = GetRoundGameState())
        {
            if (HasAuthority())
            {
                RoundGameState->SetVision();
                RoundGameState->ForceNetUpdate();
            }
            RoundGameState->OnBoardStateChanged.Broadcast();
        }
    }
}

void ARoundCharacter::UpdateFacingAfterTileChange(ARoundGridTile* PreviousTile, ARoundGridTile* NewTile)
{
    if (!NewTile)
    {
        return;
    }

    if (PreviousTile && PreviousTile != NewTile)
    {
        FVector Direction = NewTile->GetActorLocation() - PreviousTile->GetActorLocation();
        Direction.Z = 0.0f;
        if (Direction.SizeSquared() > KINDA_SMALL_NUMBER)
        {
            SetActorRotation(Direction.Rotation());
            return;
        }

        return;
    }

    FaceWorldLocation(GetBoardCenterWorldLocation());
}

void ARoundCharacter::FaceTargetTile(const ARoundGridTile* TargetTile)
{
    if (!TargetTile)
    {
        return;
    }

    FaceWorldLocation(TargetTile->GetActorLocation());
}

void ARoundCharacter::FaceWorldLocation(const FVector& WorldLocation)
{
    const FVector CurrentLocation = GetActorLocation();
    FVector Direction = WorldLocation - CurrentLocation;
    Direction.Z = 0.0f;
    if (Direction.SizeSquared() <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    SetActorRotation(Direction.Rotation());
}

FVector ARoundCharacter::GetBoardCenterWorldLocation() const
{
    if (const ARoundGameState* RoundGameState = GetRoundGameState())
    {
        const TArray<TObjectPtr<ARoundGridTile>>& Fields = RoundGameState->Fields;
        if (Fields.Num() > 0)
        {
            FVector Sum = FVector::ZeroVector;
            int32 ValidCount = 0;
            for (const ARoundGridTile* Field : Fields)
            {
                if (!Field)
                {
                    continue;
                }

                Sum += Field->GetActorLocation();
                ++ValidCount;
            }

            if (ValidCount > 0)
            {
                return Sum / static_cast<double>(ValidCount);
            }
        }
    }

    return GetActorLocation() + (GetActorForwardVector() * 100.0f);
}

void ARoundCharacter::SetOwningTeamActor(ARoundTeamActor* InTeamActor)
{
    OwningTeamActor = InTeamActor;

    if (InTeamActor)
    {
        Team = InTeamActor->TeamId;
    }
}

bool ARoundCharacter::IsEnemyFor(const ARoundCharacter* OtherCharacter) const
{
    return OtherCharacter && OtherCharacter != this && OtherCharacter->Team != Team;
}

ARoundGameState* ARoundCharacter::GetRoundGameState() const
{
    return GetWorld() ? GetWorld()->GetGameState<ARoundGameState>() : nullptr;
}

ARoundTeamActor* ARoundCharacter::GetOwningTeamActor() const
{
    return OwningTeamActor;
}

bool ARoundCharacter::EnemyField(ARoundGridTile* TargetTile) const
{
    return TargetTile && IsEnemyFor(TargetTile->OccupyingCharacter);
}

bool ARoundCharacter::CanMoveTo(ARoundGridTile* TargetTile) const
{
    if (!TargetTile)
    {
        return false;
    }

    if (HasMovementBlock())
    {
        return false;
    }

    TArray<ARoundGridTile*> MovePath;
    int32 PathCost = 0;
    return TryBuildMovePath(TargetTile, MovePath, PathCost)
        && Attributes
        && Attributes->CurrentInitiative >= PathCost;
}

int32 ARoundCharacter::GetMoveCost() const
{
    int32 Cost = Attributes ? Attributes->BaseStats.MoveCost : 0;
    if (Cost <= 0)
    {
        Cost = FMath::TruncToInt(GetValueFromKey(TEXT("Movecost")));
    }
    if (Cost <= 0)
    {
        Cost = 1;
    }

    if (CurrentTile)
    {
        Cost = CurrentTile->GetTerrainMoveCost(Cost);
    }

    const TArray<TObjectPtr<ARoundEffectActor>> MoveCostEffectsSnapshot = ActiveEffects;
    for (ARoundEffectActor* Effect : MoveCostEffectsSnapshot)
    {
        if (!Effect)
        {
            continue;
        }

        Cost = Effect->GetMovecost(Cost);
    }

    return Cost;
}

bool ARoundCharacter::CanGetTurn() const
{
    const bool bIsPuppet = CharacterRowName.ToString().Contains(TEXT("Puppet"), ESearchCase::IgnoreCase);
    if (!IsAlive() || bIsPuppet || !Attributes)
    {
        return false;
    }

    if (const ARoundGameState* RoundGameState = GetRoundGameState())
    {
        if (RoundGameState->IsCharacterBlockedByObjectiveChannel(this))
        {
            return false;
        }
    }

    return InitiativeToTurn() >= Attributes->BaseStats.MaxInitiative;
}

int32 ARoundCharacter::InitiativeToTurn() const
{
    return Attributes ? (Attributes->CurrentInitiative - NativeDelayedActionInitiative) : MIN_int32;
}

int32 ARoundCharacter::RemainInitiative() const
{
    return Attributes ? FMath::Max(0, Attributes->CurrentInitiative) : 0;
}

double ARoundCharacter::GetValueFromKey(const FString& Key) const
{
    if (!CharacterData)
    {
        return 0.0;
    }

    if (const double* ExactValue = CharacterData->Details.Find(Key))
    {
        return *ExactValue;
    }

    for (const TPair<FString, double>& Pair : CharacterData->Details)
    {
        if (Pair.Key.Equals(Key, ESearchCase::IgnoreCase))
        {
            return Pair.Value;
        }
    }

    return 0.0;
}

double ARoundCharacter::GetCritChance() const
{
    return 0.05 + GetValueFromKey(TEXT("CritChance"));
}

double ARoundCharacter::GetCritModifier() const
{
    return 1.5 + GetValueFromKey(TEXT("CritModifier"));
}

int32 ARoundCharacter::GetAttackCost() const
{
    int32 AttackCost = Attributes ? Attributes->BaseStats.AttackCost : 0;
    if (AttackCost <= 0)
    {
        const double LegacyAttackCost = GetValueFromKey(TEXT("AttackCost"));
        AttackCost = LegacyAttackCost > 0.0 ? FMath::TruncToInt(LegacyAttackCost) : 1;
    }
    for (const ARoundEffectActor* Effect : ActiveEffects)
    {
        if (Effect) AttackCost = Effect->ModifyBasicAttackCost(const_cast<ARoundCharacter*>(this), AttackCost);
    }
    return FMath::Max(0, AttackCost);
}

int32 ARoundCharacter::GetAttackRange() const
{
    if (Attributes && Attributes->BaseStats.AttackRange > 0)
    {
        return Attributes->BaseStats.AttackRange;
    }

    const double RangeValue = GetValueFromKey(TEXT("Range"));
    if (RangeValue > 0.0)
    {
        return FMath::TruncToInt(RangeValue);
    }

    return 1;
}

int32 ARoundCharacter::GetTurnDelay() const
{
    const int32 TurnDelay = Attributes ? Attributes->BaseStats.TurnDelay : 0;
    if (TurnDelay > 0)
    {
        return TurnDelay;
    }

    const double LegacyTurnDelay = GetValueFromKey(TEXT("TurnDelay"));
    if (LegacyTurnDelay > 0.0)
    {
        return FMath::TruncToInt(LegacyTurnDelay);
    }

    return Attributes ? FMath::Max(1, Attributes->BaseStats.MaxInitiative) : 100;
}

TArray<ARoundGridTile*> ARoundCharacter::GetVisionFields() const
{
    TArray<ARoundGridTile*> VisionFields;
    if (!CurrentTile)
    {
        return VisionFields;
    }

    ARoundGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ARoundGameState>() : nullptr;
    if (!GameState || !GameState->GetCoreVisionFields(this, VisionFields))
    {
        VisionFields.Reset();
        VisionFields.Add(CurrentTile);
    }
    return VisionFields;
}

bool ARoundCharacter::HasVision(const ARoundTeamActor* TeamActor) const
{
    bool bHasVision = false;

    if (CurrentTile && TeamActor)
    {
        if (TeamActor->VisionFields.Num() > 0)
        {
            bHasVision = TeamActor->VisionFields.Contains(CurrentTile);
        }
        else if (GetWorld())
        {
            for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
            {
                ARoundCharacter* Character = *It;
                if (!Character || Character->Team != TeamActor->TeamId)
                {
                    continue;
                }

                if (Character->GetVisionFields().Contains(CurrentTile))
                {
                    bHasVision = true;
                    break;
                }
            }
        }
    }

    const TArray<TObjectPtr<ARoundEffectActor>> VisionEffectsSnapshot = ActiveEffects;
    for (ARoundEffectActor* Effect : VisionEffectsSnapshot)
    {
        if (!Effect)
        {
            continue;
        }

        bHasVision = Effect->SetHasVision(const_cast<ARoundCharacter*>(this), bHasVision);
    }

    return bHasVision;
}

bool ARoundCharacter::HasTurnActionBlock() const
{
    for (ARoundEffectActor* Effect : ActiveEffects)
    {
        if (Effect && Effect->PreventsTurnActions())
        {
            return true;
        }
    }

    return false;
}

bool ARoundCharacter::HasMovementBlock() const
{
    for (ARoundEffectActor* Effect : ActiveEffects)
    {
        if (Effect && Effect->PreventsMovement())
        {
            return true;
        }
    }

    return false;
}

int32 ARoundCharacter::ModifyInitiativeGainFromEffects(int32 Amount) const
{
    int32 ModifiedAmount = Amount;
    for (ARoundEffectActor* Effect : ActiveEffects)
    {
        if (Effect)
        {
            ModifiedAmount = Effect->ModifyInitiativeGain(ModifiedAmount);
        }
    }

    return FMath::Max(0, ModifiedAmount);
}

void ARoundCharacter::BeginTurn()
{
    const bool bSkipTurnDelay = bCoreTurnDelayAlreadyApplied;
    bCoreTurnDelayAlreadyApplied = false;
    if (HasAuthority() && Attributes && !bSkipTurnDelay)
    {
        const int32 TurnDelayCost = GetTurnDelay();
        if (TurnDelayCost > 0)
        {
            Attributes->PassInitiative(TurnDelayCost);
        }
    }

    const TArray<TObjectPtr<ARoundEffectActor>> EffectsSnapshot = ActiveEffects;
    for (ARoundEffectActor* Effect : EffectsSnapshot)
    {
        if (Effect)
        {
            Effect->OnTurnStarted(this);
        }
    }

    if (ARoundTerrainActor* TerrainActor = CurrentTile ? Cast<ARoundTerrainActor>(CurrentTile->TerrainObject) : nullptr)
    {
        TerrainActor->OnCharacterTurnStarted(this);
    }
}

void ARoundCharacter::BeginTurnFromCore()
{
    bCoreTurnDelayAlreadyApplied = true;
    BeginTurn();
    bCoreTurnDelayAlreadyApplied = false;
}

void ARoundCharacter::AdvanceGlobalInitiative(int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }

    const TArray<TObjectPtr<ARoundEffectActor>> EffectsSnapshot = ActiveEffects;
    for (ARoundEffectActor* Effect : EffectsSnapshot)
    {
        if (Effect)
        {
            Effect->OnInitiativeAdvanced(this, Amount);
        }
    }

    RecalculateNativeDelayedActionInitiative();
}

void ARoundCharacter::AddEffect(ARoundEffectActor* EffectActor)
{
    if (!EffectActor)
    {
        return;
    }

    for (ARoundEffectActor* ExistingEffect : ActiveEffects)
    {
        if (ExistingEffect && ExistingEffect->PreventsEffectApplication(EffectActor))
        {
            EffectActor->Destroy();
            return;
        }
    }

    ActiveEffects.AddUnique(EffectActor);
    UE_LOG(
        LogRoundEffectRuntime,
        Log,
        TEXT("AddEffect Character=%s Effect=%s EffectClass=%s ActiveEffects=%d"),
        *GetName(),
        *GetNameSafe(EffectActor),
        *GetNameSafe(EffectActor->GetClass()),
        ActiveEffects.Num());
    EffectActor->ApplyToCharacter(this);
    RecalculateNativeDelayedActionInitiative();
    NotifyRoundStateChanged();
}

bool ARoundCharacter::TryForcedSetField(ARoundGridTile* NewTile, ARoundCharacter* SourceCharacter, bool bUpdateUI)
{
    if (!NewTile || NewTile == CurrentTile)
    {
        return false;
    }

    SetField(NewTile, bUpdateUI);
    return CurrentTile == NewTile;
}

void ARoundCharacter::RemoveEffect(ARoundEffectActor* EffectActor)
{
    if (!EffectActor)
    {
        return;
    }

    ActiveEffects.Remove(EffectActor);
    UE_LOG(
        LogRoundEffectRuntime,
        Log,
        TEXT("RemoveEffect Character=%s Effect=%s EffectClass=%s ActiveEffects=%d"),
        *GetName(),
        *GetNameSafe(EffectActor),
        *GetNameSafe(EffectActor->GetClass()),
        ActiveEffects.Num());
    RecalculateNativeDelayedActionInitiative();
    NotifyRoundStateChanged();
}

void ARoundCharacter::RecalculateNativeDelayedActionInitiative()
{
    int32 HighestTurnBlockRemainingInitiative = 0;
    for (ARoundEffectActor* Effect : ActiveEffects)
    {
        if (!Effect)
        {
            continue;
        }

        HighestTurnBlockRemainingInitiative = FMath::Max(
            HighestTurnBlockRemainingInitiative,
            Effect->GetTurnBlockRemainingInitiative());
    }

    NativeDelayedActionInitiative = HighestTurnBlockRemainingInitiative;
}

FRoundActionResult ARoundCharacter::UseAbilityIndex(int32 AbilityIndex, ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (HasTurnActionBlock())
    {
        Result.Reason = TEXT("Character cannot act.");
        return Result;
    }

    if (!GrantedAbilities.IsValidIndex(AbilityIndex) || !GrantedAbilities[AbilityIndex])
    {
        Result.Reason = TEXT("Invalid ability index.");
        return Result;
    }

    ARoundAbilityActor* AbilityActor = GrantedAbilities[AbilityIndex];
    if (!AbilityActor)
    {
        Result.Reason = TEXT("Invalid ability.");
        return Result;
    }

    if (TargetTile)
    {
        FaceTargetTile(TargetTile);
    }

    UE_LOG(
        LogRoundCharacterData,
        Log,
        TEXT("UseAbilityIndex Character=%s AbilityIndex=%d AbilityId=%s RuntimeClass=%s TargetTile=%s"),
        *GetName(),
        AbilityIndex,
        *(AbilityActor->AbilityData ? AbilityActor->AbilityData->AbilityId.ToString() : AbilityActor->RowName.ToString()),
        *GetNameSafe(AbilityActor->GetClass()),
        *GetNameSafe(TargetTile));
    Result = AbilityActor->ExecuteAbility(TargetTile);
    UE_LOG(
        LogRoundCharacterData,
        Log,
        TEXT("UseAbilityIndex Result Character=%s AbilityIndex=%d AbilityId=%s RuntimeClass=%s Success=%d Reason=%s"),
        *GetName(),
        AbilityIndex,
        *(AbilityActor->AbilityData ? AbilityActor->AbilityData->AbilityId.ToString() : AbilityActor->RowName.ToString()),
        *GetNameSafe(AbilityActor->GetClass()),
        Result.bSuccess ? 1 : 0,
        *Result.Reason);
    if (ARoundGameState* RoundGameState = GetRoundGameState())
    {
        RoundGameState->RecordAbilityUse(
            this,
            AbilityActor->AbilityData ? AbilityActor->AbilityData->AbilityId : NAME_None,
            TargetTile,
            TargetTile ? TargetTile->OccupyingCharacter : nullptr,
            Result);

        if (Result.bSuccess)
        {
            RoundGameState->SetVision();
            RoundGameState->OnBoardStateChanged.Broadcast();
        }
    }

    return Result;
}

ERoundButtonState ARoundCharacter::UseableAbilityVisisbility(bool bUseable, ERoundButtonState InputState) const
{
    if (InputState == ERoundButtonState::Enabled)
    {
        return bUseable ? ERoundButtonState::Enabled : ERoundButtonState::Disabled;
    }

    return InputState;
}

void ARoundCharacter::EndTurn()
{
    ClearHoverFields();

    ARoundGameState* RoundGameState = GetRoundGameState();
    if (!RoundGameState)
    {
        return;
    }

    RoundGameState->RecordTurnEnded(this);

    const TArray<TObjectPtr<ARoundEffectActor>> EffectsSnapshot = ActiveEffects;
    for (ARoundEffectActor* Effect : EffectsSnapshot)
    {
        if (Effect)
        {
            Effect->OnTurnEnded(this);
        }
    }

    RoundGameState->AdvanceCoreTurn(this);
    RoundGameState->OnBoardStateChanged.Broadcast();
}

void ARoundCharacter::Die()
{
    MulticastHandleDeathVisuals();
    RefreshHealthbarWidget();

    if (ARoundGameState* RoundGameState = GetRoundGameState())
    {
        RoundGameState->CharacterDie(this);
        RoundGameState->SetVision();
    }
}

void ARoundCharacter::RefreshHealthbarWidget()
{
    if (!HealthbarWidgetComponent)
    {
        return;
    }

    if (!HealthbarWidgetComponent->GetWidgetClass())
    {
        if (UClass* HealthbarWidgetClass = RoundGameDataRegistryLookup::ResolveHealthbarWidgetClass().Get())
        {
            HealthbarWidgetComponent->SetWidgetClass(HealthbarWidgetClass);
        }
    }

    HealthbarWidgetComponent->InitWidget();
    HealthbarWidgetComponent->SetHiddenInGame(false);
    HealthbarWidgetComponent->SetVisibility(true);

    if (URoundHealthbarWidget* HealthbarWidget = Cast<URoundHealthbarWidget>(HealthbarWidgetComponent->GetUserWidgetObject()))
    {
        HealthbarWidget->SetCharacter(this);
        return;
    }

    if (UUserWidget* UserWidget = HealthbarWidgetComponent->GetWidget())
    {
        if (URoundHealthbarWidget* HealthbarWidget = Cast<URoundHealthbarWidget>(UserWidget))
        {
            HealthbarWidget->SetCharacter(this);
        }
    }
}

void ARoundCharacter::MulticastHandleDeathVisuals_Implementation()
{
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);
}

bool ARoundCharacter::IsAlive() const
{
    return Attributes && Attributes->IsAlive();
}

bool ARoundCharacter::TryBuildMovePath(ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutPath, int32& OutCost, const bool bTreatTilesWithoutVisionAsEmpty) const
{
    OutPath.Reset();
    OutCost = 0;

    if (!CurrentTile || !TargetTile || TargetTile == CurrentTile || HasMovementBlock())
    {
        return false;
    }

    const bool bTargetBlockedByOccupant = TargetTile->OccupyingCharacter != nullptr
        && (!bTreatTilesWithoutVisionAsEmpty || TargetTile->bHasVision);
    if (TargetTile->TerrainType == ERoundTerrainType::Wall || bTargetBlockedByOccupant)
    {
        return false;
    }

    const ARoundGameState* RoundGameState = GetRoundGameState();
    if (!RoundGameState)
    {
        return false;
    }

    TMap<TObjectPtr<ARoundGridTile>, int32> BestCosts;
    TMap<TObjectPtr<ARoundGridTile>, TObjectPtr<ARoundGridTile>> PreviousField;
    TArray<TObjectPtr<ARoundGridTile>> OpenSet;
    OpenSet.Add(CurrentTile);
    BestCosts.Add(CurrentTile, 0);

    while (OpenSet.Num() > 0)
    {
        int32 BestIndex = 0;
        int32 BestScore = MAX_int32;
        for (int32 Index = 0; Index < OpenSet.Num(); ++Index)
        {
            const int32* CandidateScore = BestCosts.Find(OpenSet[Index]);
            if (CandidateScore && *CandidateScore < BestScore)
            {
                BestIndex = Index;
                BestScore = *CandidateScore;
            }
        }

        ARoundGridTile* Current = OpenSet[BestIndex];
        OpenSet.RemoveAtSwap(BestIndex);
        if (!Current)
        {
            continue;
        }

        if (Current == TargetTile)
        {
            break;
        }

        if (Current != CurrentTile && Current->TerrainStopsMovement())
        {
            continue;
        }

        for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(Current))
        {
            if (!Neighbor || !Neighbor->IsMoveable())
            {
                continue;
            }

            const bool bNeighborBlockedByOccupant = Neighbor->OccupyingCharacter != nullptr
                && Neighbor != this->CurrentTile
                && (!bTreatTilesWithoutVisionAsEmpty || Neighbor->bHasVision);
            if (Neighbor != TargetTile && (Neighbor->TerrainType == ERoundTerrainType::Wall || bNeighborBlockedByOccupant))
            {
                continue;
            }

            const int32 StepCost = Neighbor->GetTerrainMoveCost(GetMoveCost());
            if (StepCost == MAX_int32)
            {
                continue;
            }

            const int32 NewCost = BestScore + StepCost;
            const int32 ExistingCost = BestCosts.FindRef(Neighbor);
            if (!BestCosts.Contains(Neighbor) || NewCost < ExistingCost)
            {
                BestCosts.Add(Neighbor, NewCost);
                PreviousField.Add(Neighbor, Current);
                OpenSet.AddUnique(Neighbor);
            }
        }
    }

    const int32* FinalCost = BestCosts.Find(TargetTile);
    if (!FinalCost)
    {
        return false;
    }

    OutCost = *FinalCost;
    if (OutCost <= 0)
    {
        return false;
    }

    ARoundGridTile* TraceField = TargetTile;
    while (TraceField && TraceField != CurrentTile)
    {
        OutPath.Insert(TraceField, 0);
        const TObjectPtr<ARoundGridTile>* Previous = PreviousField.Find(TraceField);
        TraceField = Previous ? Previous->Get() : nullptr;
    }

    return OutPath.Num() > 0;
}

void ARoundCharacter::ClearHoverFields()
{
    for (ARoundGridTile* Field : HoverFields)
    {
        if (Field)
        {
            Field->OnEndHover();
        }
    }

    HoverFields.Reset();
}

void ARoundCharacter::HoverFieldArray(const TArray<ARoundGridTile*>& FieldsToHover, const ERoundHoverFieldState State)
{
    ClearHoverFields();

    for (ARoundGridTile* Field : FieldsToHover)
    {
        if (!Field)
        {
            continue;
        }

        HoverFields.AddUnique(Field);
        Field->SetHoverState(State);
    }
}

void ARoundCharacter::AttackHover(ARoundGridTile* TargetTile)
{
    ClearHoverFields();

    if (!TargetTile || !CurrentTile)
    {
        return;
    }

    HoverFields.AddUnique(TargetTile);
    const bool bInRange = CurrentTile->GetDistance(TargetTile) <= GetAttackRange();
    const bool bCanAttack = bInRange && EnemyField(TargetTile);
    TargetTile->SetHoverState(bCanAttack ? ERoundHoverFieldState::Ok : ERoundHoverFieldState::NotOk);
}
