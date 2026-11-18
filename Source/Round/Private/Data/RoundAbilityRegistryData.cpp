#include "Data/RoundAbilityRegistryData.h"

#include "Abilities/RoundArcaneShiftAbility.h"
#include "Abilities/RoundDeathMarkAbility.h"
#include "Abilities/RoundRendAbility.h"
#include "Abilities/RoundSmokeStepAbility.h"
#include "Abilities/RoundStringSnareAbility.h"
#include "Abilities/RoundSuppressingFireAbility.h"
#include "Actors/RoundAbilityActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/RoundAbilityData.h"
#include "Data/RoundSkillProgressionData.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundAbilityRegistryImport, Log, All);

namespace
{
struct FRoundImportedAbilityRow
{
    FString Name;
    FString Description;
    FString SkillpointDescription;
    FString Values;
    FString FieldState;
    FString AbilityClass;
    FString DamageType;
};

FString NormalizeSkillKey(FString Value)
{
    Value = Value.TrimStartAndEnd();
    Value.ReplaceInline(TEXT("DA_Ability_"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("DA_SkillProgression_"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("BP_"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("_C"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("Round"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("Ability"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("Details"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("_Details"), TEXT(""), ESearchCase::IgnoreCase);
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

    if (Result == TEXT("iceball"))
    {
        return TEXT("icebeam");
    }
    if (Result == TEXT("spin"))
    {
        return TEXT("spintowin");
    }
    if (Result == TEXT("hookshot"))
    {
        return TEXT("hookshot");
    }

    return Result;
}

TArray<FString> ParseCsvLine(const FString& Line)
{
    TArray<FString> Result;
    FString Current;
    bool bInQuotes = false;

    for (int32 Index = 0; Index < Line.Len(); ++Index)
    {
        const TCHAR Character = Line[Index];
        if (Character == TEXT('"'))
        {
            const bool bEscapedQuote = bInQuotes && Index + 1 < Line.Len() && Line[Index + 1] == TEXT('"');
            if (bEscapedQuote)
            {
                Current.AppendChar(TEXT('"'));
                ++Index;
            }
            else
            {
                bInQuotes = !bInQuotes;
            }
        }
        else if (Character == TEXT(',') && !bInQuotes)
        {
            Result.Add(Current);
            Current.Reset();
        }
        else
        {
            Current.AppendChar(Character);
        }
    }

    Result.Add(Current);
    return Result;
}

bool LoadCsv(const FString& FilePath, TArray<TMap<FString, FString>>& OutRows)
{
    FString Content;
    if (!FFileHelper::LoadFileToString(Content, *FilePath))
    {
        return false;
    }

    TArray<FString> Lines;
    Content.ParseIntoArrayLines(Lines, true);
    if (Lines.Num() < 2)
    {
        return false;
    }

    const TArray<FString> Headers = ParseCsvLine(Lines[0]);
    for (int32 LineIndex = 1; LineIndex < Lines.Num(); ++LineIndex)
    {
        if (Lines[LineIndex].TrimStartAndEnd().IsEmpty())
        {
            continue;
        }

        const TArray<FString> Fields = ParseCsvLine(Lines[LineIndex]);
        TMap<FString, FString> Row;
        for (int32 HeaderIndex = 0; HeaderIndex < Headers.Num(); ++HeaderIndex)
        {
            Row.Add(Headers[HeaderIndex], Fields.IsValidIndex(HeaderIndex) ? Fields[HeaderIndex] : FString());
        }

        OutRows.Add(MoveTemp(Row));
    }

    return OutRows.Num() > 0;
}

FString GetLastPathToken(const FString& Path)
{
    FString Token = FPaths::GetBaseFilename(Path);
    if (Token.IsEmpty())
    {
        const int32 SlashIndex = Path.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
        Token = SlashIndex != INDEX_NONE ? Path.Mid(SlashIndex + 1) : Path;
    }

    Token.ReplaceInline(TEXT("'"), TEXT(""));
    return Token;
}

TArray<FString> BuildAliases(const FRoundImportedAbilityRow& Row)
{
    TArray<FString> Aliases;
    Aliases.AddUnique(NormalizeSkillKey(Row.Name));
    Aliases.AddUnique(NormalizeSkillKey(GetLastPathToken(Row.AbilityClass)));
    Aliases.AddUnique(NormalizeSkillKey(GetLastPathToken(Row.SkillpointDescription)));
    return Aliases;
}

TMap<FString, double> ParseDoubleMap(FString ValueString)
{
    TMap<FString, double> Result;
    ValueString = ValueString.TrimStartAndEnd();
    if (ValueString.IsEmpty() || ValueString == TEXT("()"))
    {
        return Result;
    }

    const FRegexPattern PairPattern(TEXT(R"(\(\"([^\"]+)\"\s*,\s*(-?[0-9]+(?:\.[0-9]+)?)\))"));
    FRegexMatcher Matcher(PairPattern, ValueString);
    while (Matcher.FindNext())
    {
        const FString Key = Matcher.GetCaptureGroup(1);
        const FString Number = Matcher.GetCaptureGroup(2);
        Result.Add(Key, FCString::Atod(*Number));
    }

    return Result;
}

ERoundDamageType ParseDamageType(const FString& Value)
{
    if (Value.Equals(TEXT("Fire"), ESearchCase::IgnoreCase))
    {
        return ERoundDamageType::Fire;
    }
    if (Value.Equals(TEXT("Ice"), ESearchCase::IgnoreCase))
    {
        return ERoundDamageType::Ice;
    }
    if (Value.Equals(TEXT("Lightning"), ESearchCase::IgnoreCase))
    {
        return ERoundDamageType::Lightning;
    }
    if (Value.Equals(TEXT("Holy"), ESearchCase::IgnoreCase))
    {
        return ERoundDamageType::Holy;
    }
    if (Value.Equals(TEXT("Ground"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("Rock"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("Stone"), ESearchCase::IgnoreCase))
    {
        return ERoundDamageType::Ground;
    }
    if (Value.Equals(TEXT("Nature"), ESearchCase::IgnoreCase))
    {
        return ERoundDamageType::Nature;
    }
    if (Value.Equals(TEXT("TrueDamage"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("True"), ESearchCase::IgnoreCase))
    {
        return ERoundDamageType::TrueDamage;
    }
    if (Value.Equals(TEXT("Magical"), ESearchCase::IgnoreCase) || Value.Equals(TEXT("Magic"), ESearchCase::IgnoreCase))
    {
        return ERoundDamageType::Magical;
    }

    return ERoundDamageType::Physical;
}

ERoundFieldState ParseFieldState(const FString& Value)
{
    if (Value.Equals(TEXT("Empty"), ESearchCase::IgnoreCase))
    {
        return ERoundFieldState::Empty;
    }
    if (Value.Equals(TEXT("Character"), ESearchCase::IgnoreCase))
    {
        return ERoundFieldState::Character;
    }
    if (Value.Equals(TEXT("Enemy"), ESearchCase::IgnoreCase))
    {
        return ERoundFieldState::Enemy;
    }
    if (Value.Equals(TEXT("Ally"), ESearchCase::IgnoreCase))
    {
        return ERoundFieldState::Ally;
    }
    if (Value.Equals(TEXT("Self"), ESearchCase::IgnoreCase))
    {
        return ERoundFieldState::Self;
    }

    return ERoundFieldState::Any;
}

TArray<FString> SplitTopLevelItems(FString Value)
{
    TArray<FString> Items;
    Value = Value.TrimStartAndEnd();
    if (Value.Len() < 2)
    {
        return Items;
    }

    if (Value.StartsWith(TEXT("(")) && Value.EndsWith(TEXT(")")))
    {
        Value = Value.Mid(1, Value.Len() - 2);
    }

    int32 Depth = 0;
    bool bInQuotes = false;
    int32 ItemStart = INDEX_NONE;

    for (int32 Index = 0; Index < Value.Len(); ++Index)
    {
        const TCHAR Character = Value[Index];
        const bool bEscapedQuote = bInQuotes && Character == TEXT('"') && Index + 1 < Value.Len() && Value[Index + 1] == TEXT('"');
        if (bEscapedQuote)
        {
            ++Index;
            continue;
        }

        if (Character == TEXT('"'))
        {
            bInQuotes = !bInQuotes;
            continue;
        }

        if (bInQuotes)
        {
            continue;
        }

        if (Character == TEXT('('))
        {
            if (Depth == 0)
            {
                ItemStart = Index;
            }
            ++Depth;
        }
        else if (Character == TEXT(')'))
        {
            --Depth;
            if (Depth == 0 && ItemStart != INDEX_NONE)
            {
                Items.Add(Value.Mid(ItemStart, Index - ItemStart + 1));
                ItemStart = INDEX_NONE;
            }
        }
    }

    return Items;
}

FString ExtractQuotedValue(const FString& Source, const FString& Prefix)
{
    const int32 PrefixIndex = Source.Find(Prefix, ESearchCase::IgnoreCase);
    if (PrefixIndex == INDEX_NONE)
    {
        return FString();
    }

    const int32 StartIndex = PrefixIndex + Prefix.Len();
    const int32 EndIndex = Source.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, StartIndex);
    if (EndIndex == INDEX_NONE)
    {
        return FString();
    }

    return Source.Mid(StartIndex, EndIndex - StartIndex);
}

FString ExtractBalancedValue(const FString& Source, const FString& Prefix)
{
    const int32 PrefixIndex = Source.Find(Prefix, ESearchCase::IgnoreCase);
    if (PrefixIndex == INDEX_NONE)
    {
        return FString();
    }

    const int32 StartIndex = Source.Find(TEXT("("), ESearchCase::IgnoreCase, ESearchDir::FromStart, PrefixIndex + Prefix.Len());
    if (StartIndex == INDEX_NONE)
    {
        return FString();
    }

    int32 Depth = 0;
    bool bInQuotes = false;
    for (int32 Index = StartIndex; Index < Source.Len(); ++Index)
    {
        const TCHAR Character = Source[Index];
        const bool bEscapedQuote = bInQuotes && Character == TEXT('"') && Index + 1 < Source.Len() && Source[Index + 1] == TEXT('"');
        if (bEscapedQuote)
        {
            ++Index;
            continue;
        }

        if (Character == TEXT('"'))
        {
            bInQuotes = !bInQuotes;
            continue;
        }

        if (bInQuotes)
        {
            continue;
        }

        if (Character == TEXT('('))
        {
            ++Depth;
        }
        else if (Character == TEXT(')'))
        {
            --Depth;
            if (Depth == 0)
            {
                return Source.Mid(StartIndex, Index - StartIndex + 1);
            }
        }
    }

    return FString();
}

FRoundSkillpointDetails ParseSkillpointDetails(const FString& Item)
{
    FRoundSkillpointDetails Result;
    Result.Description = ExtractQuotedValue(Item, TEXT("Description=\""));
    Result.Values = ParseDoubleMap(ExtractBalancedValue(Item, TEXT("Values=")));
    return Result;
}

TArray<FRoundSkillpointDetails> ParseProgressionLevels(const TArray<TMap<FString, FString>>& Rows)
{
    TArray<FRoundSkillpointDetails> Levels;
    for (const TMap<FString, FString>& Row : Rows)
    {
        const FString* SkillpointsValue = Row.Find(TEXT("Skillpoints"));
        if (!SkillpointsValue)
        {
            continue;
        }

        for (const FString& Item : SplitTopLevelItems(*SkillpointsValue))
        {
            Levels.Add(ParseSkillpointDetails(Item));
        }
    }

    return Levels;
}

template <typename AssetType>
TMap<FString, AssetType*> LoadAssetsByNormalizedName()
{
    TMap<FString, AssetType*> Result;

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.ClassPaths.Add(AssetType::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;
    Filter.PackagePaths.Add(TEXT("/Game"));
    Filter.bRecursivePaths = true;

    TArray<FAssetData> Assets;
    AssetRegistryModule.Get().GetAssets(Filter, Assets);
    for (const FAssetData& AssetData : Assets)
    {
        if (AssetType* Asset = Cast<AssetType>(AssetData.GetAsset()))
        {
            Result.Add(NormalizeSkillKey(Asset->GetName()), Asset);
        }
    }

    return Result;
}

TMap<FString, TSubclassOf<ARoundAbilityActor>> BuildAbilityClassMap()
{
    TMap<FString, TSubclassOf<ARoundAbilityActor>> Result;
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

        Result.Add(NormalizeSkillKey(Class->GetName()), Class);
    }

    return Result;
}

bool IsCharacterTargetFieldState(const ERoundFieldState FieldState)
{
    return FieldState == ERoundFieldState::Character
        || FieldState == ERoundFieldState::Enemy
        || FieldState == ERoundFieldState::Ally
        || FieldState == ERoundFieldState::Self;
}

bool IsTileTargetFieldState(const ERoundFieldState FieldState)
{
    return FieldState == ERoundFieldState::Any || FieldState == ERoundFieldState::Empty;
}

FRoundSkillpointDetails MakeAbilitySkillpoint(const TCHAR* Title, const TCHAR* Description, std::initializer_list<TPair<FString, double>> Values)
{
    FRoundSkillpointDetails Result;
    Result.Title = FText::FromString(Title);
    Result.Description = Description;
    for (const TPair<FString, double>& Pair : Values)
    {
        Result.Values.Add(Pair.Key, Pair.Value);
    }
    return Result;
}

#if WITH_EDITOR
template <typename AssetType>
AssetType* FindOrCreateDataAsset(const FString& PackagePath, const FString& AssetName)
{
    const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
    if (AssetType* ExistingAsset = LoadObject<AssetType>(nullptr, *ObjectPath))
    {
        return ExistingAsset;
    }

    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return nullptr;
    }

    AssetType* NewAsset = NewObject<AssetType>(Package, *AssetName, RF_Public | RF_Standalone);
    if (!NewAsset)
    {
        return nullptr;
    }

    FAssetRegistryModule::AssetCreated(NewAsset);
    Package->MarkPackageDirty();
    NewAsset->MarkPackageDirty();
    return NewAsset;
}

struct FExtendedAbilityDefinition
{
    const TCHAR* SkillKey;
    const TCHAR* AbilityAssetName;
    const TCHAR* ProgressionAssetName;
    const TCHAR* DisplayName;
    const TCHAR* Description;
    TSubclassOf<ARoundAbilityActor> AbilityClass;
    int32 InitiativeCost = 0;
    int32 Cooldown = 0;
    int32 Range = 1;
    int32 BaseDamage = 0;
    ERoundDamageType DamageType = ERoundDamageType::Physical;
    ERoundFieldState FieldState = ERoundFieldState::Enemy;
    bool bRequiresTargetCharacter = true;
    bool bRequiresTargetTile = false;
    TMap<FString, double> Values;
};

void UpsertAbilityRegistryEntry(URoundAbilityRegistryData* RegistryData, URoundAbilityData* AbilityData)
{
    if (!RegistryData || !AbilityData)
    {
        return;
    }

    TObjectPtr<URoundAbilityData>* ExistingEntry = RegistryData->Entries.FindByPredicate([AbilityData](const TObjectPtr<URoundAbilityData>& Candidate)
        {
            return Candidate == AbilityData
                || (Candidate && Candidate->AbilityId == AbilityData->AbilityId);
        });

    if (!ExistingEntry)
    {
        RegistryData->Entries.Add(AbilityData);
    }
    else
    {
        *ExistingEntry = AbilityData;
    }
}

bool EnsureExtendedAbilityDefinition(
    URoundAbilityRegistryData* RegistryData,
    const FExtendedAbilityDefinition& Definition,
    URoundAbilityData*& OutAbilityData,
    URoundSkillProgressionData*& OutProgressionData)
{
    static const FString AbilityFolder = TEXT("/Game/TopDown/Data/Abilities");
    static const FString ProgressionFolder = TEXT("/Game/TopDown/Data/SkillProgression");

    OutAbilityData = FindOrCreateDataAsset<URoundAbilityData>(
        FString::Printf(TEXT("%s/%s"), *AbilityFolder, Definition.AbilityAssetName),
        Definition.AbilityAssetName);
    OutProgressionData = FindOrCreateDataAsset<URoundSkillProgressionData>(
        FString::Printf(TEXT("%s/%s"), *ProgressionFolder, Definition.ProgressionAssetName),
        Definition.ProgressionAssetName);

    if (!OutAbilityData || !OutProgressionData)
    {
        return false;
    }

    OutAbilityData->Modify();
    OutAbilityData->AbilityClass = Definition.AbilityClass;
    OutAbilityData->AbilityId = FName(Definition.SkillKey);
    OutAbilityData->DisplayName = FText::FromString(Definition.DisplayName);
    OutAbilityData->Description = FText::FromString(Definition.Description);
    OutAbilityData->InitiativeCost = Definition.InitiativeCost;
    OutAbilityData->Cooldown = Definition.Cooldown;
    OutAbilityData->Range = Definition.Range;
    OutAbilityData->BaseDamage = Definition.BaseDamage;
    OutAbilityData->DamageType = Definition.DamageType;
    OutAbilityData->FieldState = Definition.FieldState;
    OutAbilityData->bRequiresTargetCharacter = Definition.bRequiresTargetCharacter;
    OutAbilityData->bRequiresTargetTile = Definition.bRequiresTargetTile;
    OutAbilityData->Values = Definition.Values;
    OutAbilityData->SkillProgression = OutProgressionData;
    OutAbilityData->MarkPackageDirty();

    OutProgressionData->Modify();
    OutProgressionData->SkillId = OutAbilityData->AbilityId;
    OutProgressionData->DisplayName = FText::FromString(Definition.DisplayName);
    OutProgressionData->Description = FText::FromString(Definition.Description);
    OutProgressionData->MarkPackageDirty();

    UpsertAbilityRegistryEntry(RegistryData, OutAbilityData);
    return true;
}
#endif
}

URoundAbilityRegistryData::URoundAbilityRegistryData()
{
}

void URoundAbilityRegistryData::ImportFromCurrentExport()
{
#if WITH_EDITOR
    Modify();

    const FString ImportDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / ImportRelativeDirectory);
    const FString SkillDetailsPath = ImportDirectory / TEXT("skilldetails.csv");

    TArray<TMap<FString, FString>> SkillRows;
    if (!LoadCsv(SkillDetailsPath, SkillRows))
    {
        UE_LOG(LogRoundAbilityRegistryImport, Warning, TEXT("Failed to load skill details csv from %s"), *SkillDetailsPath);
        return;
    }

    const TMap<FString, URoundAbilityData*> AbilityAssets = LoadAssetsByNormalizedName<URoundAbilityData>();
    const TMap<FString, URoundSkillProgressionData*> ProgressionAssets = LoadAssetsByNormalizedName<URoundSkillProgressionData>();
    const TMap<FString, TSubclassOf<ARoundAbilityActor>> AbilityClasses = BuildAbilityClassMap();

    Entries.Reset();

    for (const TMap<FString, FString>& CsvRow : SkillRows)
    {
        FRoundImportedAbilityRow Row;
        Row.Name = CsvRow.FindRef(TEXT("Name"));
        Row.Description = CsvRow.FindRef(TEXT("Description"));
        Row.SkillpointDescription = CsvRow.FindRef(TEXT("SkillpointDescription"));
        Row.Values = CsvRow.FindRef(TEXT("Values"));
        Row.FieldState = CsvRow.FindRef(TEXT("FieldState"));
        Row.AbilityClass = CsvRow.FindRef(TEXT("AbilityClass"));
        Row.DamageType = CsvRow.FindRef(TEXT("DamageType"));

        const TArray<FString> Aliases = BuildAliases(Row);

        URoundAbilityData* AbilityData = nullptr;
        for (const FString& Alias : Aliases)
        {
            if (URoundAbilityData* const* Found = AbilityAssets.Find(Alias))
            {
                AbilityData = *Found;
                break;
            }
        }

        if (!AbilityData)
        {
            UE_LOG(LogRoundAbilityRegistryImport, Warning, TEXT("No RoundAbilityData asset found for skill %s"), *Row.Name);
            continue;
        }

        AbilityData->Modify();
        AbilityData->AbilityId = FName(Row.Name);
        AbilityData->DisplayName = FText::FromString(Row.Name);
        AbilityData->Description = FText::FromString(Row.Description);
        AbilityData->Values = ParseDoubleMap(Row.Values);
        AbilityData->DamageType = ParseDamageType(Row.DamageType);
        AbilityData->FieldState = ParseFieldState(Row.FieldState);
        AbilityData->BaseDamage = FMath::RoundToInt(AbilityData->Values.FindRef(TEXT("Damage")));
        AbilityData->Cooldown = FMath::RoundToInt(AbilityData->Values.FindRef(TEXT("Cooldown")));
        AbilityData->Range = FMath::RoundToInt(AbilityData->Values.FindRef(TEXT("Range")));
        AbilityData->InitiativeCost = FMath::RoundToInt(AbilityData->Values.FindRef(TEXT("InitativeCost")));
        AbilityData->bRequiresTargetCharacter = IsCharacterTargetFieldState(AbilityData->FieldState);
        AbilityData->bRequiresTargetTile = IsTileTargetFieldState(AbilityData->FieldState);

        URoundSkillProgressionData* Progression = nullptr;
        for (const FString& Alias : Aliases)
        {
            if (URoundSkillProgressionData* const* Found = ProgressionAssets.Find(Alias))
            {
                Progression = *Found;
                break;
            }
        }

        if (Progression)
        {
            AbilityData->SkillProgression = Progression;
        }
        else
        {
            UE_LOG(LogRoundAbilityRegistryImport, Warning, TEXT("No RoundSkillProgressionData asset found for skill %s"), *Row.Name);
        }

        if (Progression)
        {
            TArray<FString> CandidateFiles;
            for (const FString& Alias : Aliases)
            {
                if (!Alias.IsEmpty())
                {
                    CandidateFiles.AddUnique(ImportDirectory / FString::Printf(TEXT("%s.csv"), *Alias));
                }
            }

            FString MatchingProgressionCsv;
            for (const FString& CandidateFile : CandidateFiles)
            {
                if (IFileManager::Get().FileExists(*CandidateFile))
                {
                    MatchingProgressionCsv = CandidateFile;
                    break;
                }
            }

            if (!MatchingProgressionCsv.IsEmpty())
            {
                TArray<TMap<FString, FString>> ProgressionRows;
                if (LoadCsv(MatchingProgressionCsv, ProgressionRows))
                {
                    Progression->Modify();
                    Progression->SkillId = AbilityData->AbilityId;
                    Progression->DisplayName = AbilityData->DisplayName;
                    Progression->Description = AbilityData->Description;
                    Progression->Levels = ParseProgressionLevels(ProgressionRows);
                    Progression->MarkPackageDirty();
                }
            }
            else
            {
                UE_LOG(LogRoundAbilityRegistryImport, Warning, TEXT("No progression csv found for skill %s"), *Row.Name);
            }
        }

        Entries.Add(AbilityData);

        for (const FString& Alias : Aliases)
        {
            if (const TSubclassOf<ARoundAbilityActor>* FoundClass = AbilityClasses.Find(Alias))
            {
                AbilityData->AbilityClass = *FoundClass;
                break;
            }
        }

        if (!AbilityData->AbilityClass)
        {
            UE_LOG(LogRoundAbilityRegistryImport, Warning, TEXT("No native ability class found for skill %s"), *Row.Name);
        }

        AbilityData->MarkPackageDirty();
    }

    MarkPackageDirty();
    UE_LOG(LogRoundAbilityRegistryImport, Log, TEXT("Imported %d ability entries from %s"), Entries.Num(), *ImportDirectory);
#endif
}

void URoundAbilityRegistryData::RefreshEntriesFromAbilityAssets()
{
#if WITH_EDITOR
    Modify();
    Entries.Reset();

    const TMap<FString, URoundAbilityData*> AbilityAssets = LoadAssetsByNormalizedName<URoundAbilityData>();
    TArray<URoundAbilityData*> SortedAssets;
    AbilityAssets.GenerateValueArray(SortedAssets);
    SortedAssets.Sort([](const URoundAbilityData& Left, const URoundAbilityData& Right)
    {
        const int32 LeftNumericId = Left.AbilityNumericId;
        const int32 RightNumericId = Right.AbilityNumericId;
        if (LeftNumericId != RightNumericId)
        {
            if (LeftNumericId <= 0)
            {
                return false;
            }
            if (RightNumericId <= 0)
            {
                return true;
            }

            return LeftNumericId < RightNumericId;
        }

        const FString LeftKey = !Left.AbilityId.IsNone() ? Left.AbilityId.ToString() : Left.GetName();
        const FString RightKey = !Right.AbilityId.IsNone() ? Right.AbilityId.ToString() : Right.GetName();
        return LeftKey < RightKey;
    });

    TSet<FString> AddedKeys;
    for (URoundAbilityData* AbilityData : SortedAssets)
    {
        if (!AbilityData)
        {
            continue;
        }

        const FString Key = NormalizeSkillKey(!AbilityData->AbilityId.IsNone() ? AbilityData->AbilityId.ToString() : AbilityData->GetName());
        if (Key.IsEmpty() || AddedKeys.Contains(Key))
        {
            continue;
        }

        AddedKeys.Add(Key);
        Entries.Add(AbilityData);
    }

    MarkPackageDirty();
    UE_LOG(LogRoundAbilityRegistryImport, Log, TEXT("RefreshEntriesFromAbilityAssets rebuilt %d registry entries"), Entries.Num());
#endif
}

void URoundAbilityRegistryData::NormalizeCharacter4MageDamageTypes()
{
#if WITH_EDITOR
    Modify();

    const TMap<FString, URoundAbilityData*> AbilityAssets = LoadAssetsByNormalizedName<URoundAbilityData>();
    struct FDamageTypeFixup
    {
        const TCHAR* AbilityKey;
        ERoundDamageType DamageType;
    };

    const FDamageTypeFixup Fixups[] =
    {
        { TEXT("fireball"), ERoundDamageType::Fire },
        { TEXT("icebeam"), ERoundDamageType::Ice },
        { TEXT("lightningstrike"), ERoundDamageType::Lightning },
        { TEXT("rockblast"), ERoundDamageType::Ground },
    };

    int32 FixedCount = 0;
    for (const FDamageTypeFixup& Fixup : Fixups)
    {
        if (URoundAbilityData* const* FoundAbility = AbilityAssets.Find(Fixup.AbilityKey))
        {
            URoundAbilityData* AbilityData = *FoundAbility;
            if (!AbilityData)
            {
                continue;
            }

            AbilityData->Modify();
            AbilityData->DamageType = Fixup.DamageType;
            AbilityData->MarkPackageDirty();
            ++FixedCount;

            UE_LOG(LogRoundAbilityRegistryImport, Log, TEXT("NormalizeCharacter4MageDamageTypes set %s to DamageType=%d"),
                *GetNameSafe(AbilityData),
                static_cast<int32>(Fixup.DamageType));
        }
        else
        {
            UE_LOG(LogRoundAbilityRegistryImport, Warning, TEXT("NormalizeCharacter4MageDamageTypes could not find ability asset for key %s"),
                Fixup.AbilityKey);
        }
    }

    MarkPackageDirty();
    UE_LOG(LogRoundAbilityRegistryImport, Log, TEXT("NormalizeCharacter4MageDamageTypes updated %d Character 4 ability assets"), FixedCount);
#endif
}

void URoundAbilityRegistryData::PopulateCharacter1AbilityProgressionDefaults()
{
#if WITH_EDITOR
    Modify();

    const TMap<FString, URoundSkillProgressionData*> ProgressionAssets = LoadAssetsByNormalizedName<URoundSkillProgressionData>();
    const TMap<FString, URoundAbilityData*> AbilityAssets = LoadAssetsByNormalizedName<URoundAbilityData>();

    struct FCharacter1AbilityDefaults
    {
        const TCHAR* SkillKey;
        const TCHAR* DisplayName;
        const TCHAR* Description;
        TArray<FRoundSkillpointDetails> Levels;
    };

    const TArray<FCharacter1AbilityDefaults> Defaults = {
        {
            TEXT("knockback"),
            TEXT("Knockback"),
            TEXT("Stoesst Ziele zurueck und kontrolliert Positionen."),
            {
                MakeAbilitySkillpoint(TEXT("Wucht"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 100.0}}),
                MakeAbilitySkillpoint(TEXT("Eisenstoss"), TEXT("Erhoeht Knockback um {KnockbackDistance}."), {{TEXT("KnockbackDistance"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Vorbote"), TEXT("Das Ziel erleidet bis zum Ende des naechsten Zuges dieses Ziels {IncomingDamageMultiplierOnHit}x Schaden."), {{TEXT("IncomingDamageMultiplierOnHit"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Aufprall"), TEXT("Stoppt das Ziel an einem Hindernis oder Character, erleidet es zusaetzlich {WallBonusDamage} Schaden."), {{TEXT("WallBonusDamage"), 180.0}}),
                MakeAbilitySkillpoint(TEXT("Unwucht"), TEXT("Das Ziel verliert nach Knockback {TargetInitiativeLossOnHit} Initiative."), {{TEXT("TargetInitiativeLossOnHit"), 20.0}}),
                MakeAbilitySkillpoint(TEXT("Schutzstoss"), TEXT("Das Ziel verursacht bis zum Ende des naechsten Zuges dieses Ziels nur {OutgoingDamageMultiplierAfterHit}x Schaden."), {{TEXT("OutgoingDamageMultiplierAfterHit"), 0.9}}),
                MakeAbilitySkillpoint(TEXT("Jaegerinstinkt"), TEXT("Gegen bereits debuffte Ziele verursacht Knockback {DamageMultiplierVsDebuffed}x Schaden."), {{TEXT("DamageMultiplierVsDebuffed"), 1.2}}),
                MakeAbilitySkillpoint(TEXT("Raumgewinn"), TEXT("Nach Knockback darf Character 1 sich {SelfStepTowardTarget} Feld in Richtung des Ziels bewegen, ohne Initiative zu bezahlen."), {{TEXT("SelfStepTowardTarget"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Kettenzug"), TEXT("Verringert ChainCost um {-ChainCost}, erhoeht aber den Cooldown um {Cooldown}."), {{TEXT("ChainCost"), -1.0}, {TEXT("Cooldown"), 40.0}}),
                MakeAbilitySkillpoint(TEXT("Hartnaeckig"), TEXT("Das Ziel erhaelt nach Knockback Slow {ApplySlowOnHitStrength} fuer {ApplySlowOnHitDuration} Zug."), {{TEXT("ApplySlowOnHitStrength"), 0.2}, {TEXT("ApplySlowOnHitDuration"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Gegenhalt"), TEXT("Erleidet Character 1 vom Ziel Schaden, erleidet er fuer {SelfIncomingDamageMultiplierFromTargetDuration} Zug nur {SelfIncomingDamageMultiplierFromTarget}x Schaden."), {{TEXT("SelfIncomingDamageMultiplierFromTarget"), 0.8}, {TEXT("SelfIncomingDamageMultiplierFromTargetDuration"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Rammschlag"), TEXT("Erhoeht den Schaden um {Damage}, erhoeht aber die Initiativekosten um {InitiativeCost}."), {{TEXT("Damage"), 120.0}, {TEXT("InitiativeCost"), 10.0}})
            }
        },
        {
            TEXT("delay"),
            TEXT("Delay"),
            TEXT("Verzoegert Gegner und zerstoert ihren Turn-Rhythmus."),
            {
                MakeAbilitySkillpoint(TEXT("Tiefe Verzoegerung"), TEXT("Erhoeht Delay um {DelayAmount}."), {{TEXT("DelayAmount"), 12.0}}),
                MakeAbilitySkillpoint(TEXT("Stoerimpuls"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Schwungklau"), TEXT("Character 1 erhaelt {SelfInitiativeGainOnUse} Initiative."), {{TEXT("SelfInitiativeGainOnUse"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Taktbruch"), TEXT("Gegen bereits debuffte Ziele erhoeht sich Delay um weitere {ExtraDelayVsDebuffed}."), {{TEXT("ExtraDelayVsDebuffed"), 12.0}}),
                MakeAbilitySkillpoint(TEXT("Unterbrechen"), TEXT("Ziele mit hoher Initiative verursachen bis zum Ende des naechsten Zuges dieses Ziels nur {OutgoingDamageMultiplierOnHighInitiativeTarget}x Schaden."), {{TEXT("OutgoingDamageMultiplierOnHighInitiativeTarget"), 0.8}}),
                MakeAbilitySkillpoint(TEXT("Bremsfeld"), TEXT("Gegner im Umkreis von {AoEInitiativeLossRadius} Feld um das Ziel verlieren {AoEInitiativeLossAmount} Initiative."), {{TEXT("AoEInitiativeLossRadius"), 1.0}, {TEXT("AoEInitiativeLossAmount"), 8.0}}),
                MakeAbilitySkillpoint(TEXT("Kettenriss"), TEXT("Weitere Debuffs auf dem Ziel halten {IncomingEffectDurationAddOnTarget} Zug laenger."), {{TEXT("IncomingEffectDurationAddOnTarget"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Erschwerter Zugriff"), TEXT("Das Ziel erhaelt bis zum Ende des naechsten Zuges dieses Ziels +{MoveCostAddOnTarget} MoveCost."), {{TEXT("MoveCostAddOnTarget"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Sparsame Ausfuehrung"), TEXT("Verringert Initiativekosten um {-InitiativeCost}, verringert aber Delay um {-DelayAmount}."), {{TEXT("InitiativeCost"), -5.0}, {TEXT("DelayAmount"), -5.0}}),
                MakeAbilitySkillpoint(TEXT("Nachhall"), TEXT("Wird ein bereits verzoegertes Ziel erneut von Character 1 getroffen, verliert es zusaetzlich {AdditionalInitiativeLossIfAlreadyDelayed} Initiative."), {{TEXT("AdditionalInitiativeLossIfAlreadyDelayed"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Zielerfassung"), TEXT("Das Ziel kann bis zum Ende des naechsten Zuges dieses Ziels keine Bewegungsboni erhalten."), {{TEXT("BlockMovementBonusesOnTarget"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Kurzer Takt"), TEXT("Verringert den Cooldown um {-Cooldown}, verringert aber den Schaden um {-Damage}."), {{TEXT("Cooldown"), -40.0}, {TEXT("Damage"), -50.0}})
            }
        },
        {
            TEXT("shackle"),
            TEXT("Shackle"),
            TEXT("Haelt Gegner fest und erschwert jede Bewegung."),
            {
                MakeAbilitySkillpoint(TEXT("Schwere Fessel"), TEXT("Erhoeht Slow auf {SlowStrength}."), {{TEXT("SlowStrength"), 0.15}}),
                MakeAbilitySkillpoint(TEXT("Lange Fessel"), TEXT("Erhoeht die Dauer um {SlowDuration} Zug."), {{TEXT("SlowDuration"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Schwacher Stand"), TEXT("Gefesselte Ziele verursachen nur {OutgoingDamageMultiplierWhileShackled}x Schaden."), {{TEXT("OutgoingDamageMultiplierWhileShackled"), 0.88}}),
                MakeAbilitySkillpoint(TEXT("Bleierne Schritte"), TEXT("Immer wenn sich ein gefesseltes Ziel bewegt, verliert es {InitiativeLossOnMove} Initiative."), {{TEXT("InitiativeLossOnMove"), 8.0}}),
                MakeAbilitySkillpoint(TEXT("Ausweglos"), TEXT("Gefesselte Ziele koennen keine Bewegungsboni erhalten."), {{TEXT("BlockMovementBonuses"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Unter Druck"), TEXT("Character 1 verursacht gegen gefesselte Ziele {IncomingDamageMultiplierFromCauserWhileShackled}x Schaden."), {{TEXT("IncomingDamageMultiplierFromCauserWhileShackled"), 1.2}}),
                MakeAbilitySkillpoint(TEXT("Stoerender Griff"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Restfessel"), TEXT("Wenn Shackle endet, verliert das Ziel {InitiativeLossOnExpire} Initiative."), {{TEXT("InitiativeLossOnExpire"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Sparsame Fessel"), TEXT("Verringert Initiativekosten um {-InitiativeCost}, verringert aber die Dauer um {-SlowDuration} Zug."), {{TEXT("InitiativeCost"), -5.0}, {TEXT("SlowDuration"), -1.0}}),
                MakeAbilitySkillpoint(TEXT("Schwere Kette"), TEXT("Gegen bereits debuffte Ziele haelt Shackle {SlowDuration} Zug laenger."), {{TEXT("SlowDuration"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Haltepunkt"), TEXT("Solange Shackle aktiv ist, erleidet Character 1 vom Ziel nur {SelfIncomingDamageMultiplierFromTargetWhileShackled}x Schaden."), {{TEXT("SelfIncomingDamageMultiplierFromTargetWhileShackled"), 0.85}}),
                MakeAbilitySkillpoint(TEXT("Kurzer Takt"), TEXT("Verringert den Cooldown um {-Cooldown}, erhoeht aber die Initiativekosten um {InitiativeCost}."), {{TEXT("Cooldown"), -40.0}, {TEXT("InitiativeCost"), 5.0}})
            }
        },
        {
            TEXT("bind"),
            TEXT("Bind"),
            TEXT("Bestrafte Bewegung mit Trigger-Schaden und Turn-Control."),
            {
                MakeAbilitySkillpoint(TEXT("Schwere Bindung"), TEXT("Erhoeht den Trigger-Schaden um {Damage}."), {{TEXT("Damage"), 120.0}}),
                MakeAbilitySkillpoint(TEXT("Tiefer Ruck"), TEXT("Erhoeht den Trigger-Delay um {DelayAmount}."), {{TEXT("DelayAmount"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Lauernde Bindung"), TEXT("Erhoeht die Dauer um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 40.0}}),
                MakeAbilitySkillpoint(TEXT("Widerhaken"), TEXT("Wenn Bind ausloest, verursacht das Ziel bis zum Ende des naechsten Zuges dieses Ziels nur {TriggerOutgoingDamageMultiplier}x Schaden."), {{TEXT("TriggerOutgoingDamageMultiplier"), 0.85}}),
                MakeAbilitySkillpoint(TEXT("Fester Griff"), TEXT("Ein gebundenes Ziel kann keine Bewegungsboni erhalten."), {{TEXT("BlockMovementBonusesOnTarget"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Kettenstoss"), TEXT("Wenn Bind ausloest, verlieren Gegner im Umkreis von {TriggerAoEInitiativeLossRadius} Feld ebenfalls {TriggerAoEInitiativeLossAmount} Initiative."), {{TEXT("TriggerAoEInitiativeLossRadius"), 1.0}, {TEXT("TriggerAoEInitiativeLossAmount"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Hetzjagd"), TEXT("Bereits debuffte Ziele erleiden {TriggerDamageMultiplierVsDebuffed}x Trigger-Schaden."), {{TEXT("TriggerDamageMultiplierVsDebuffed"), 1.2}}),
                MakeAbilitySkillpoint(TEXT("Jagdzeichen"), TEXT("Wenn Bind ausloest, erhaelt Character 1 {TriggerInitiativeGainForCauser} Initiative."), {{TEXT("TriggerInitiativeGainForCauser"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Sparsame Falle"), TEXT("Verringert Initiativekosten um {-InitiativeCost}, verringert aber den Trigger-Schaden um {-Damage}."), {{TEXT("InitiativeCost"), -5.0}, {TEXT("Damage"), -60.0}}),
                MakeAbilitySkillpoint(TEXT("Schwere Fesselung"), TEXT("Wenn Bind ausloest, erhaelt das Ziel Slow {TriggerSlowAmount} fuer {TriggerSlowDuration} Zug."), {{TEXT("TriggerSlowAmount"), 0.2}, {TEXT("TriggerSlowDuration"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Nachbeben"), TEXT("Der Trigger verlangsamt auch die Umgebung und haelt {DurationInitiative} Initiative laenger."), {{TEXT("DurationInitiative"), 40.0}, {TEXT("TriggerAoEInitiativeLossRadius"), 1.0}, {TEXT("TriggerAoEInitiativeLossAmount"), 6.0}}),
                MakeAbilitySkillpoint(TEXT("Kurzer Takt"), TEXT("Verringert den Cooldown um {-Cooldown}, verringert aber die Dauer um {-DurationInitiative} Initiative."), {{TEXT("Cooldown"), -40.0}, {TEXT("DurationInitiative"), -40.0}})
            }
        },
        {
            TEXT("taunt"),
            TEXT("Taunt"),
            TEXT("Zwingt Gegner auf Character 1 und bestraft falsche Ziele."),
            {
                MakeAbilitySkillpoint(TEXT("Tiefer Spott"), TEXT("Erhoeht den Schadensmalus gegen andere Ziele auf {ReductionAmount}."), {{TEXT("ReductionAmount"), 0.15}}),
                MakeAbilitySkillpoint(TEXT("Langer Spott"), TEXT("Erhoeht die Dauer um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 40.0}}),
                MakeAbilitySkillpoint(TEXT("Einschuechterung"), TEXT("Das Ziel verliert sofort {ImmediateInitiativeLoss} Initiative."), {{TEXT("ImmediateInitiativeLoss"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Offene Deckung"), TEXT("Solange Taunt aktiv ist, erleidet das Ziel von Character 1 {IncomingDamageMultiplierFromCauserWhileTaunted}x Schaden."), {{TEXT("IncomingDamageMultiplierFromCauserWhileTaunted"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Verunsicherung"), TEXT("Greift das Ziel jemand anderen an, verliert es danach {InitiativeLossWhenIgnoringCauser} Initiative."), {{TEXT("InitiativeLossWhenIgnoringCauser"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Drohkulisse"), TEXT("Bereits debuffte Ziele bleiben {ExtraDurationVsDebuffed} Zug laenger provoziert."), {{TEXT("ExtraDurationVsDebuffed"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Standhalten"), TEXT("Solange Taunt aktiv ist, erleidet Character 1 vom Ziel nur {SelfIncomingDamageMultiplierFromTargetWhileTaunted}x Schaden."), {{TEXT("SelfIncomingDamageMultiplierFromTargetWhileTaunted"), 0.85}}),
                MakeAbilitySkillpoint(TEXT("Dominanz"), TEXT("Greift das Ziel Character 1 an, erhaelt Character 1 {InitiativeGainForCauserWhenAttacked} Initiative."), {{TEXT("InitiativeGainForCauserWhenAttacked"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Sparsame Provokation"), TEXT("Verringert Initiativekosten um {-InitiativeCost}, verringert aber die Dauer um {-DurationInitiative} Initiative."), {{TEXT("InitiativeCost"), -5.0}, {TEXT("DurationInitiative"), -40.0}}),
                MakeAbilitySkillpoint(TEXT("Blossstellung"), TEXT("Solange Taunt aktiv ist, erleidet das Ziel {IncomingDamageMultiplierWhileTaunted}x Schaden."), {{TEXT("IncomingDamageMultiplierWhileTaunted"), 1.1}}),
                MakeAbilitySkillpoint(TEXT("Taktischer Druck"), TEXT("Erhoeht den direkten Schaden um {Damage}."), {{TEXT("Damage"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Kurzer Takt"), TEXT("Verringert den Cooldown um {-Cooldown}, der direkte Schaden entfaellt aber."), {{TEXT("Cooldown"), -40.0}, {TEXT("Damage"), -80.0}})
            }
        }
    };

    int32 UpdatedProgressions = 0;
    for (const FCharacter1AbilityDefaults& DefaultEntry : Defaults)
    {
        URoundSkillProgressionData* Progression = nullptr;
        if (const URoundSkillProgressionData* const* FoundProgression = ProgressionAssets.Find(DefaultEntry.SkillKey))
        {
            Progression = const_cast<URoundSkillProgressionData*>(*FoundProgression);
        }

        URoundAbilityData* AbilityData = nullptr;
        if (const URoundAbilityData* const* FoundAbility = AbilityAssets.Find(DefaultEntry.SkillKey))
        {
            AbilityData = const_cast<URoundAbilityData*>(*FoundAbility);
        }

        if (!Progression || !AbilityData)
        {
            UE_LOG(LogRoundAbilityRegistryImport, Warning, TEXT("PopulateCharacter1AbilityProgressionDefaults skipped %s - Progression=%s Ability=%s"),
                DefaultEntry.SkillKey,
                *GetNameSafe(Progression),
                *GetNameSafe(AbilityData));
            continue;
        }

        Progression->Modify();
        Progression->SkillId = AbilityData->AbilityId;
        Progression->DisplayName = FText::FromString(DefaultEntry.DisplayName);
        Progression->Description = FText::FromString(DefaultEntry.Description);
        Progression->Levels = DefaultEntry.Levels;
        Progression->MarkPackageDirty();

        AbilityData->Modify();
        AbilityData->SkillProgression = Progression;
        AbilityData->MarkPackageDirty();
        ++UpdatedProgressions;
    }

    MarkPackageDirty();
    UE_LOG(LogRoundAbilityRegistryImport, Log, TEXT("PopulateCharacter1AbilityProgressionDefaults updated %d Character 1 ability progressions"), UpdatedProgressions);
#endif
}

void URoundAbilityRegistryData::PopulateCharacter2AbilityProgressionDefaults()
{
#if WITH_EDITOR
    Modify();

    URoundAbilityData* CreatedAbilityData = nullptr;
    URoundSkillProgressionData* CreatedProgressionData = nullptr;
    EnsureExtendedAbilityDefinition(
        this,
        {
            TEXT("SuppressingFire"),
            TEXT("DA_Ability_SuppressingFire"),
            TEXT("DA_SkillProgression_SuppressingFire"),
            TEXT("SuppressingFire"),
            TEXT("Unterdrueckt Ziele auf Distanz und erschwert jede Flucht."),
            ARoundSuppressingFireAbility::StaticClass(),
            20,
            210,
            4,
            140,
            ERoundDamageType::Physical,
            ERoundFieldState::Enemy,
            true,
            false,
            {
                {TEXT("MoveCostAddOnTarget"), 2.0}
            }
        },
        CreatedAbilityData,
        CreatedProgressionData);

    const TMap<FString, URoundSkillProgressionData*> ProgressionAssets = LoadAssetsByNormalizedName<URoundSkillProgressionData>();
    const TMap<FString, URoundAbilityData*> AbilityAssets = LoadAssetsByNormalizedName<URoundAbilityData>();

    struct FCharacter2AbilityDefaults
    {
        const TCHAR* SkillKey;
        const TCHAR* DisplayName;
        const TCHAR* Description;
        TArray<FRoundSkillpointDetails> Levels;
    };

    const TArray<FCharacter2AbilityDefaults> Defaults = {
        {
            TEXT("snipe"),
            TEXT("Snipe"),
            TEXT("Praeziser Fernschuss fuer Initiativekontrolle und Picks."),
            {
                MakeAbilitySkillpoint(TEXT("Aufgerissene Deckung"), TEXT("Snipe reduziert die Defense des Ziels fuer {DefenseReductionDurationInitiative} Initiative um {DefenseReductionOnHit}."), {{TEXT("DefenseReductionOnHit"), 20.0}, {TEXT("DefenseReductionDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Tiefer Schuss"), TEXT("Erhoeht den Schaden pro fehlender Initiative um {InitativeScaling}."), {{TEXT("InitativeScaling"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Zielmarkierung"), TEXT("Der naechste Treffer von Character 2 verursacht {MarkNextHitDamageMultiplier}x Schaden und verbraucht die Markierung."), {{TEXT("MarkNextHitDamageMultiplier"), 1.15}, {TEXT("MarkDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Freie Linie"), TEXT("Ziele ohne benachbarte Verbuendete erleiden {DamageMultiplierVsIsolated}x Schaden."), {{TEXT("DamageMultiplierVsIsolated"), 1.2}}),
                MakeAbilitySkillpoint(TEXT("Durchschuss"), TEXT("Ein Gegner direkt hinter dem Ziel erleidet {SecondaryTargetDamageMultiplier}x Snipe-Schaden."), {{TEXT("SecondaryTargetDamageMultiplier"), 0.6}}),
                MakeAbilitySkillpoint(TEXT("Distanzmesser"), TEXT("Ein Treffer auf maximaler Reichweite erstattet einmal pro Zug die verbrauchte Ammo."), {{TEXT("RefundAmmoAtMaxRange"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Sauberer Treffer"), TEXT("Besiegt Snipe das Ziel, wird die verbrauchte Ammo erstattet."), {{TEXT("RefundAmmoOnKill"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Nadelpunkt"), TEXT("Fehlen dem Ziel mindestens {RequiredMissingInitiativeForDefenseIgnore} Initiative, ignoriert Snipe {ConditionalDefenseIgnore} Defense."), {{TEXT("RequiredMissingInitiativeForDefenseIgnore"), 40.0}, {TEXT("ConditionalDefenseIgnore"), 25.0}}),
                MakeAbilitySkillpoint(TEXT("Kammerdruck"), TEXT("Wird Snipe mit der letzten Ammo abgefeuert, wird das Ziel um {LastAmmoKnockbackDistance} Feld zurueckgestossen."), {{TEXT("LastAmmoKnockbackDistance"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Nachbeben"), TEXT("Trifft Snipe ein bereits debufftes Ziel, wird dessen aeltester entfernbarer Debuff um {ExtendDebuffDurationInitiative} Initiative beziehungsweise einen Zug verlaengert."), {{TEXT("ExtendDebuffDurationInitiative"), 40.0}}),
                MakeAbilitySkillpoint(TEXT("Hinrichtung"), TEXT("Ziele unter {LowHealthThresholdPercent}% Leben erleiden {DamageMultiplierVsLowHealth}x Schaden. Ein Kill reduziert den Cooldown einmal pro Zug um {CooldownReductionOnKill}."), {{TEXT("LowHealthThresholdPercent"), 35.0}, {TEXT("DamageMultiplierVsLowHealth"), 1.25}, {TEXT("CooldownReductionOnKill"), 40.0}}),
                MakeAbilitySkillpoint(TEXT("Kalter Lauf"), TEXT("Solange Snipe einsatzbereit ist, erhaelt er pro {ReadyDamageInitiativeInterval} vergangener Initiative {ReadyDamagePercentPerInterval}x Schaden, maximal {ReadyDamagePercentCap}x."), {{TEXT("ReadyDamageInitiativeInterval"), 3.0}, {TEXT("ReadyDamagePercentPerInterval"), 0.01}, {TEXT("ReadyDamagePercentCap"), 0.3}})
            }
        },
        {
            TEXT("concentrate"),
            TEXT("Concentrate"),
            TEXT("Bereitet den naechsten Treffer vor und verstaerkt Timing, Kontrolle und Rhythmus."),
            {
                MakeAbilitySkillpoint(TEXT("Kammerpflege"), TEXT("Wird Concentrate mit 0 Ammo aktiviert, wird nach 10 Initiative 1 Ammo geladen."), {{TEXT("ReloadAmmoAfterDelay"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Fester Fokus"), TEXT("Concentrate hat keine Zeitbegrenzung, verfaellt aber bei Bewegung."), {{TEXT("UnlimitedUntilMove"), 1.0}, {TEXT("ExpireOnMove"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Unter Spannung"), TEXT("Solange Concentrate aktiv ist, blockiert es Ammo-Regeneration und erhaelt pro 10 Initiative 5% Bonus, maximal 20%."), {{TEXT("BlockAmmoRegeneration"), 1.0}, {TEXT("ChargeInterval"), 10.0}, {TEXT("ChargeBonusPerInterval"), 0.05}, {TEXT("MaxChargeBonus"), 0.2}}),
                MakeAbilitySkillpoint(TEXT("Panzerbruch"), TEXT("Der verstaerkte Treffer reduziert Defense fuer 80 Initiative um 20."), {{TEXT("DefenseReductionOnHit"), 20.0}, {TEXT("DefenseReductionDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Rhythmusbruch"), TEXT("Der verstaerkte Treffer entfernt 20% der aktuellen Initiative des Ziels."), {{TEXT("CurrentInitiativeLossFraction"), 0.2}}),
                MakeAbilitySkillpoint(TEXT("Rueckstossstellung"), TEXT("Nach dem verstaerkten Treffer ist eine angrenzende Bewegung kostenlos."), {{TEXT("GrantFreeStepAfterHit"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Sauberer Abzug"), TEXT("Wird Concentrate durch einen normalen Angriff verbraucht, kostet dieser keine Ammo."), {{TEXT("BasicAttackConsumesNoAmmo"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Systemueberlastung"), TEXT("Die verbrauchende Faehigkeit reduziert ihren Cooldown nach dem Treffer um 30 Initiative."), {{TEXT("AbilityCooldownReductionOnConsume"), 30.0}}),
                MakeAbilitySkillpoint(TEXT("Schockwelle"), TEXT("Angrenzende Gegner erleiden 35% des verstaerkten Schadens."), {{TEXT("SplashDamageFraction"), 0.35}}),
                MakeAbilitySkillpoint(TEXT("Durchgezogen"), TEXT("Besiegt der verstaerkte Treffer sein Ziel, bleibt Concentrate ohne Ausloesungslimit erhalten."), {{TEXT("PersistOnKill"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Magazin entladen"), TEXT("Nach einem verstaerkten normalen Angriff wird die gesamte verbleibende Ammo als kostenlose normale Angriffe mit 50% Schaden verschossen."), {{TEXT("DumpMagazineAfterBasic"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Doppelfokus"), TEXT("Concentrate verstaerkt zwei Treffer; der zweite erhaelt 50% des Bonus."), {{TEXT("EmpoweredHitCount"), 2.0}, {TEXT("SecondHitBonusFraction"), 0.5}})
            }
        },
        {
            TEXT("aim"),
            TEXT("Aim"),
            TEXT("Kontrolliert Raeume, bestraft Bewegung und oeffnet Schussfenster."),
            {
                MakeAbilitySkillpoint(TEXT("Doppelvisier"), TEXT("Aim erhaelt 2 Aufladungen. Jede platziert ein Radius-0-Feld mit 75% Schaden; maximal zwei Felder gleichzeitig."), {{TEXT("Charges"), 2.0}, {TEXT("DamageRange"), -1.0}, {TEXT("TriggerDamageMultiplier"), 0.75}}),
                MakeAbilitySkillpoint(TEXT("Nadeloehr"), TEXT("Aim besitzt Radius 0 und ignoriert {DefenseIgnore} Defense."), {{TEXT("DamageRange"), -1.0}, {TEXT("DefenseIgnore"), 30.0}}),
                MakeAbilitySkillpoint(TEXT("Geladene Falle"), TEXT("Aim verbraucht {AmmoCost} Ammo und verursacht {TriggerDamageMultiplier}x Schaden."), {{TEXT("AmmoCost"), 1.0}, {TEXT("TriggerDamageMultiplier"), 1.4}}),
                MakeAbilitySkillpoint(TEXT("Bremsdraht"), TEXT("Wenn Aim ausloest, verliert das Ziel {TriggerInitiativeLoss} Initiative."), {{TEXT("TriggerInitiativeLoss"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Erschuettert"), TEXT("Der naechste schadensverursachende Angriff oder Skill des Ziels verursacht 30% weniger Schaden."), {{TEXT("NextOutgoingDamageMultiplier"), 0.7}}),
                MakeAbilitySkillpoint(TEXT("Aufgedeckt"), TEXT("Der naechste Treffer von Character 2 verursacht 20% mehr Schaden."), {{TEXT("MarkDamageMultiplier"), 1.2}, {TEXT("MarkDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Ruhige Falle"), TEXT("Bleibt Character 2 seit der Platzierung stehen, verursacht der Trigger 20% mehr Schaden."), {{TEXT("BonusIfOwnerStayed"), 1.0}, {TEXT("StayedDamageMultiplier"), 1.2}}),
                MakeAbilitySkillpoint(TEXT("Offene Wunde"), TEXT("Aim verlaengert den aeltesten entfernbaren Debuff um 40 Initiative beziehungsweise einen Zug."), {{TEXT("ExtendDebuffDurationInitiative"), 40.0}}),
                MakeAbilitySkillpoint(TEXT("Sauberer Fang"), TEXT("Ein Kill gibt 1 Ammo zurueck und reduziert den Aim-Cooldown um 40 Initiative."), {{TEXT("AmmoOnKill"), 1.0}, {TEXT("CooldownReductionOnKill"), 40.0}}),
                MakeAbilitySkillpoint(TEXT("Nachleuchten"), TEXT("Das Feld kann ein zweites Mal mit 50% Schaden ausloesen."), {{TEXT("PersistTriggerCount"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Deckungsfeuer"), TEXT("Ueberlebt das Ziel, feuert Character 2 einen kostenlosen normalen Angriff mit 50% Schaden und 1 Ammo Kosten."), {{TEXT("CoverFire"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Verfolgungsmarke"), TEXT("Ueberlebt das Ziel, erleidet es bei seiner naechsten Bewegung 50% des Aim-Schadens."), {{TEXT("ApplyTrackingMark"), 1.0}})
            }
        },
        {
            TEXT("piercingshot"),
            TEXT("PiercingShot"),
            TEXT("Linienangriff fuer Druck auf mehrere Ziele und harte Durchbrueche."),
            {
                MakeAbilitySkillpoint(TEXT("Panzerbohrer"), TEXT("Alle getroffenen Ziele werden behandelt, als haetten sie {DefenseIgnore} weniger Defense."), {{TEXT("DefenseIgnore"), 20.0}}),
                MakeAbilitySkillpoint(TEXT("Schockmunition"), TEXT("Jeder getroffene Charakter verliert {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Splitterkern"), TEXT("Wird ein Feldobjekt zerstoert, erleiden angrenzende Gegner {DestroyedObjectSplashDamageMultiplier}x Schaden. Jeder Gegner kann pro Schuss nur einmal davon getroffen werden."), {{TEXT("DestroyedObjectSplashDamageMultiplier"), 0.4}}),
                MakeAbilitySkillpoint(TEXT("Wuchtlinie"), TEXT("Das erste getroffene Ziel wird um {KnockbackFirstTargetDistance} Feld zurueckgestossen, wenn das Feld frei ist."), {{TEXT("KnockbackFirstTargetDistance"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Kreuzungspunkt"), TEXT("Der zweite und jeder weitere getroffene Charakter erleidet {LaterTargetDamageMultiplier}x Schaden."), {{TEXT("LaterTargetDamageMultiplier"), 1.2}}),
                MakeAbilitySkillpoint(TEXT("Langlauf"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Verkettete Wunden"), TEXT("Getroffene Gegner werden markiert. Der naechste normale Angriff auf einen markierten Gegner verursacht {ChainMarkDamageMultiplier}x Angriffsschaden an allen anderen markierten Gegnern und entfernt alle Markierungen."), {{TEXT("ChainMarkDamageMultiplier"), 0.3}}),
                MakeAbilitySkillpoint(TEXT("Umlenkgeschoss"), TEXT("Nach dem ersten Treffer aendert das Projektil seine Richtung zum naechsten Gegner innerhalb der verbleibenden Reichweite."), {{TEXT("RedirectAfterFirstHit"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Zielspur"), TEXT("Getroffene Gegner werden fuer {TargetMarkDurationInitiative} Initiative markiert. Normale Angriffe von Character 2 erhalten gegen sie {AttackRangeVsMarked} Reichweite."), {{TEXT("TargetMarkDurationInitiative"), 80.0}, {TEXT("AttackRangeVsMarked"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Doppelladung"), TEXT("Wenn nach dem Schuss Ammo verbleibt, wird dieselbe Schussbahn mit {RepeatShotDamageMultiplier}x Schaden erneut abgefeuert. Kostet 1 Ammo, aber keine Initiative."), {{TEXT("RepeatShotDamageMultiplier"), 0.5}}),
                MakeAbilitySkillpoint(TEXT("Endloser Durchschuss"), TEXT("Jeder Treffer an einem Charakter oder zerstoerbaren Objekt setzt die verbleibende Reichweite des Projektils auf die volle Reichweite zurueck."), {{TEXT("ResetRangeOnEveryHit"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Panzerbrechende Vorbereitung"), TEXT("Jeder Treffer erzeugt einen Stack. Der naechste normale Angriff verbraucht alle Stacks und reduziert die Defense des Ziels pro Stack um {DefenseReductionPerHitStack} fuer {DefenseReductionDurationInitiative} Initiative."), {{TEXT("DefenseReductionPerHitStack"), 5.0}, {TEXT("DefenseReductionDurationInitiative"), 80.0}})
            }
        },
        {
            TEXT("suppressingfire"),
            TEXT("SuppressingFire"),
            TEXT("Unterdrueckt Ziele auf Distanz und nimmt ihnen Raum zum Entkommen."),
            {
                MakeAbilitySkillpoint(TEXT("Schweres Feuer"), TEXT("Suppressing Fire verursacht {Damage} zusaetzlichen Schaden."), {{TEXT("Damage"), 70.0}}),
                MakeAbilitySkillpoint(TEXT("Niederhalten"), TEXT("Der naechste normale Angriff des Ziels kostet {BasicAttackCostAdd} zusaetzliche Initiative."), {{TEXT("BasicAttackCostAdd"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Markiermunition"), TEXT("Das Ziel bleibt fuer das gesamte Team sichtbar und kann waehrend der Unterdrueckung nicht unsichtbar werden."), {{TEXT("RevealToAllTeams"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Feuerleitung"), TEXT("Der erste Treffer jedes verbuendeten Charakters verlaengert die Unterdrueckung um {AllyHitDurationExtension} Initiative."), {{TEXT("AllyHitDurationExtension"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Offene Deckung"), TEXT("Reduziert die Defense des unterdrueckten Ziels um {DefenseReduction}."), {{TEXT("DefenseReduction"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Kreuzfeuer"), TEXT("Nach dem ersten Treffer eines Verbuendeten kostet Character 2s naechster normaler Angriff gegen das Ziel keine Ammo."), {{TEXT("FreeFollowupAfterAllyHit"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Nachfuehren"), TEXT("Bewegt sich das Ziel, wird der Cooldown von Suppressing Fire einmalig um {CooldownReductionOnMove} Initiative reduziert."), {{TEXT("CooldownReductionOnMove"), 30.0}}),
                MakeAbilitySkillpoint(TEXT("Bestaetigter Abschuss"), TEXT("Stirbt das Ziel waehrend der Unterdrueckung, erhaelt Character 2 {AmmoOnKill} Ammo."), {{TEXT("AmmoOnKill"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Vorbelastetes Ziel"), TEXT("Gegen ein bereits debufftes Ziel verbraucht Suppressing Fire keine Ammo."), {{TEXT("FreeAmmoVsDebuffed"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Unterdrueckungskette"), TEXT("Stirbt das Ziel, springt die Unterdrueckung mit verbleibender Dauer auf den naechsten Gegner innerhalb von {ChainRange} Feldern ueber."), {{TEXT("ChainOnKill"), 1.0}, {TEXT("ChainRange"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Ueberwachungsfeuer"), TEXT("Bei der ersten Bewegung des Ziels feuert Character 2 einen normalen Angriff mit {ReactionDamageMultiplier}x Schaden. Kostet 1 Ammo und kann kritisch treffen."), {{TEXT("ReactionDamageMultiplier"), 0.5}}),
                MakeAbilitySkillpoint(TEXT("Kopf unten"), TEXT("Das Ziel kann sich nicht bewegen, bis es erneut Schaden von einem Charakter erleidet. Danach bleibt der normale Bewegungskosten-Malus bestehen."), {{TEXT("RootUntilDamaged"), 1.0}})
            }
        }
    };

    int32 UpdatedProgressions = 0;
    for (const FCharacter2AbilityDefaults& DefaultEntry : Defaults)
    {
        URoundSkillProgressionData* Progression = nullptr;
        if (const URoundSkillProgressionData* const* FoundProgression = ProgressionAssets.Find(DefaultEntry.SkillKey))
        {
            Progression = const_cast<URoundSkillProgressionData*>(*FoundProgression);
        }

        URoundAbilityData* AbilityData = nullptr;
        if (const URoundAbilityData* const* FoundAbility = AbilityAssets.Find(DefaultEntry.SkillKey))
        {
            AbilityData = const_cast<URoundAbilityData*>(*FoundAbility);
        }

        if (!Progression || !AbilityData)
        {
            UE_LOG(LogRoundAbilityRegistryImport, Warning, TEXT("PopulateCharacter2AbilityProgressionDefaults skipped %s - Progression=%s Ability=%s"),
                DefaultEntry.SkillKey,
                *GetNameSafe(Progression),
                *GetNameSafe(AbilityData));
            continue;
        }

        Progression->Modify();
        Progression->SkillId = AbilityData->AbilityId;
        Progression->DisplayName = FText::FromString(DefaultEntry.DisplayName);
        Progression->Description = FText::FromString(DefaultEntry.Description);
        Progression->Levels = DefaultEntry.Levels;
        Progression->MarkPackageDirty();

        AbilityData->Modify();
        AbilityData->SkillProgression = Progression;
        AbilityData->MarkPackageDirty();
        ++UpdatedProgressions;
    }

    MarkPackageDirty();
    UE_LOG(LogRoundAbilityRegistryImport, Log, TEXT("PopulateCharacter2AbilityProgressionDefaults updated %d Character 2 ability progressions"), UpdatedProgressions);
#endif
}

void URoundAbilityRegistryData::PopulateCharacter3AbilityProgressionDefaults()
{
#if WITH_EDITOR
    Modify();

    URoundAbilityData* CreatedAbilityData = nullptr;
    URoundSkillProgressionData* CreatedProgressionData = nullptr;
    EnsureExtendedAbilityDefinition(
        this,
        {
            TEXT("DeathMark"),
            TEXT("DA_Ability_DeathMark"),
            TEXT("DA_SkillProgression_DeathMark"),
            TEXT("Death Mark"),
            TEXT("Markiert ein Ziel und erhoeht waehrend der gesamten Dauer den Schaden von Character 3 gegen dieses Ziel."),
            ARoundDeathMarkAbility::StaticClass(),
            15,
            220,
            4,
            0,
            ERoundDamageType::Physical,
            ERoundFieldState::Enemy,
            true,
            true,
            {{TEXT("ComboGainOnTrigger"), 1.0}, {TEXT("DurationInitiative"), 80.0}, {TEXT("DamageMultiplierFromCauser"), 1.15}}
        },
        CreatedAbilityData,
        CreatedProgressionData);

    const TMap<FString, URoundSkillProgressionData*> ProgressionAssets = LoadAssetsByNormalizedName<URoundSkillProgressionData>();
    const TMap<FString, URoundAbilityData*> AbilityAssets = LoadAssetsByNormalizedName<URoundAbilityData>();

    struct FCharacter3AbilityDefaults
    {
        const TCHAR* SkillKey;
        const TCHAR* DisplayName;
        const TCHAR* Description;
        TArray<FRoundSkillpointDetails> Levels;
    };

    const TArray<FCharacter3AbilityDefaults> Defaults = {
        {
            TEXT("armorpen"),
            TEXT("Armorpen"),
            TEXT("Baut Druck und Combo auf und oeffnet Ziele fuer den Kill."),
            {
                MakeAbilitySkillpoint(TEXT("Tiefer Schnitt"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 60.0}}),
                MakeAbilitySkillpoint(TEXT("Brechstahl"), TEXT("Erhoeht ignorierte Defense um {Armorpen}."), {{TEXT("Armorpen"), 20.0}}),
                MakeAbilitySkillpoint(TEXT("Harte Schale"), TEXT("Besitzt das Ziel vor dem Treffer mehr Defense als Character 3, gewaehrt Armorpen {BonusComboVsHigherDefense} zusaetzliche Combo."), {{TEXT("BonusComboVsHigherDefense"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Schnelle Oeffnung"), TEXT("Verringert den Cooldown um {-Cooldown} Initiative."), {{TEXT("Cooldown"), -40.0}}),
                MakeAbilitySkillpoint(TEXT("Ruestungsbruch"), TEXT("Reduziert die Defense des Ziels fuer {DefenseReductionDurationInitiative} Initiative um {DefenseReduction}."), {{TEXT("DefenseReduction"), 15.0}, {TEXT("DefenseReductionDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Bekannte Schwaeche"), TEXT("Trifft Armorpen ein bereits debufftes Ziel, erhaelt Character 3 {InitiativeRefundVsDebuffed} Initiative zurueck."), {{TEXT("InitiativeRefundVsDebuffed"), 5.0}}),
                MakeAbilitySkillpoint(TEXT("Tiefe Wunde"), TEXT("Ueberlebt das Ziel, erhaelt es fuer {BleedDurationInitiative} Initiative Bleed in Hoehe von {BleedDamageFractionOnSurvive}x des verursachten Schadens."), {{TEXT("BleedDamageFractionOnSurvive"), 0.2}, {TEXT("BleedDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Durchtrennter Schlagarm"), TEXT("Der naechste normale Angriff des Ziels innerhalb von {NextBasicAttackReductionDurationInitiative} Initiative verursacht nur {NextBasicAttackDamageMultiplier}x Schaden."), {{TEXT("NextBasicAttackDamageMultiplier"), 0.7}, {TEXT("NextBasicAttackReductionDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Perfekter Aufbau"), TEXT("Erreicht Character 3 durch Armorpen sein Combo-Maximum, wird der aktuelle Cooldown von Armorpen um {CooldownReductionOnReachMaxCombo} Initiative reduziert."), {{TEXT("CooldownReductionOnReachMaxCombo"), 60.0}}),
                MakeAbilitySkillpoint(TEXT("Magiebrecher"), TEXT("Entfernt vor dem Treffer den aeltesten entfernbaren Buff des Ziels."), {{TEXT("RemoveOldestRemovableBuff"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Geteilte Bresche"), TEXT("Markiert das Ziel fuer {SharedBreachDurationInitiative} Initiative. Der erste Treffer jedes verbuendeten Characters ignoriert zusaetzlich so viel Defense wie Armorpen beim urspruenglichen Treffer."), {{TEXT("SharedBreachDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Doppelstich"), TEXT("Ueberlebt das Ziel, folgt ein zweiter Treffer mit {SecondStrikeAppliedDamageFraction}x des verursachten Schadens. Er loest keine weiteren Treffer- oder Passiveffekte aus."), {{TEXT("SecondStrikeAppliedDamageFraction"), 0.5}})
            }
        },
        {
            TEXT("shadowstrike"),
            TEXT("Shadowstrike"),
            TEXT("Bringt Character 3 in Position, gibt Sicherheit und setzt Killfenster auf."),
            {
                MakeAbilitySkillpoint(TEXT("Tiefer Schnitt"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 50.0}}),
                MakeAbilitySkillpoint(TEXT("Kurzer Schatten"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -40.0}}),
                MakeAbilitySkillpoint(TEXT("Langer Sprung"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Schneller Anschluss"), TEXT("Der erste normale Angriff gegen das Ziel kostet bis zum Ende des naechsten eigenen Zuges {FollowupBasicAttackCostReduction} weniger Initiative."), {{TEXT("FollowupBasicAttackCostReduction"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Schattenschritt"), TEXT("Nach Shadowstrike ist die naechste angrenzende Bewegung in diesem Zug kostenlos."), {{TEXT("GrantFreeAdjacentMove"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Fluechtiger Koerper"), TEXT("Der naechste eingehende Treffer vor dem naechsten eigenen Zug verursacht nur {NextIncomingDamageMultiplier}x Schaden."), {{TEXT("NextIncomingDamageMultiplier"), 0.75}}),
                MakeAbilitySkillpoint(TEXT("Isoliertes Ziel"), TEXT("Befindet sich kein weiterer Gegner im Umkreis von {IsolationRadius}, gewaehrt Shadowstrike {BonusComboVsIsolated} zusaetzliche Combo."), {{TEXT("IsolationRadius"), 2.0}, {TEXT("BonusComboVsIsolated"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Kalter Auftakt"), TEXT("Wird Shadowstrike mit 0 Combo eingesetzt, gewaehrt er {BonusComboAtZero} zusaetzliche Combo."), {{TEXT("BonusComboAtZero"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Vorbereiteter Abschluss"), TEXT("Erreicht Shadowstrike das Combo-Maximum, wird der Cooldown von Execute um {ExecuteCooldownReductionOnReachMaxCombo} Initiative reduziert."), {{TEXT("ExecuteCooldownReductionOnReachMaxCombo"), 50.0}}),
                MakeAbilitySkillpoint(TEXT("Schattenjagd"), TEXT("Gegen Ziele mit einem aktiven Debuff erhaelt Shadowstrike {RangeVsDebuffed} Reichweite."), {{TEXT("RangeVsDebuffed"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Schattenwunde"), TEXT("Ueberlebt das Ziel, wird es fuer {ShadowWoundDurationInitiative} Initiative markiert. Der naechste Treffer von Character 3 verursacht {ShadowWoundBonusDamage} Bonusschaden und gewaehrt {ShadowWoundComboGain} Combo."), {{TEXT("ShadowWoundDurationInitiative"), 80.0}, {TEXT("ShadowWoundBonusDamage"), 100.0}, {TEXT("ShadowWoundComboGain"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Durch das Ziel"), TEXT("Character 3 landet auf der vom Ausgangspunkt abgewandten Seite und Shadowstrike ignoriert {DefenseIgnoreOppositeLanding} Defense."), {{TEXT("LandOppositeTarget"), 1.0}, {TEXT("DefenseIgnoreOppositeLanding"), 30.0}})
            }
        },
        {
            TEXT("execute"),
            TEXT("Execute"),
            TEXT("Verwertet Combo und beendet angeschlagene Ziele."),
            {
                MakeAbilitySkillpoint(TEXT("Blutgeruch"), TEXT("Gegen Ziele mit hoechstens 35% Leben erhaelt Execute {RangeVsLowHealth} Reichweite."), {{TEXT("BonusRangeHealthThreshold"), 0.35}, {TEXT("RangeVsLowHealth"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Comboentladung"), TEXT("Erhoeht den Schaden pro verbrauchter Combo um {DamagePerCombopoint}."), {{TEXT("DamagePerCombopoint"), 20.0}}),
                MakeAbilitySkillpoint(TEXT("Schnell bereit"), TEXT("Verringert den Cooldown um {-Cooldown} Initiative."), {{TEXT("Cooldown"), -60.0}}),
                MakeAbilitySkillpoint(TEXT("Hinrichtung"), TEXT("Gegen Ziele mit hoechstens 35% Leben verursacht Execute {DamageMultiplierVsLowHealth}x Schaden."), {{TEXT("LowHealthDamageThreshold"), 0.35}, {TEXT("DamageMultiplierVsLowHealth"), 1.2}}),
                MakeAbilitySkillpoint(TEXT("Vorbereiteter Tod"), TEXT("Gegen ein bereits debufftes Ziel ignoriert Execute {DefenseIgnoreVsDebuffed} Defense."), {{TEXT("DefenseIgnoreVsDebuffed"), 25.0}}),
                MakeAbilitySkillpoint(TEXT("Schneller Abgang"), TEXT("Toetet Execute sein Ziel, erhaelt Character 3 {InitiativeGainOnKill} Initiative."), {{TEXT("InitiativeGainOnKill"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Durch die Ruestung"), TEXT("Verbraucht Execute mindestens {ComboThresholdForDefenseIgnore} Combo, ignoriert der Treffer zusaetzlich {DefenseIgnoreAtComboThreshold} Defense."), {{TEXT("ComboThresholdForDefenseIgnore"), 3.0}, {TEXT("DefenseIgnoreAtComboThreshold"), 30.0}}),
                MakeAbilitySkillpoint(TEXT("Nicht verschwendet"), TEXT("Ueberlebt das Ziel, erhaelt Character 3 {ComboRefundOnSurvive} der verbrauchten Combo zurueck."), {{TEXT("ComboRefundOnSurvive"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Fluchtweg"), TEXT("Toetet Execute sein Ziel, ist die naechste angrenzende Bewegung in diesem Zug kostenlos."), {{TEXT("GrantFreeAdjacentMoveOnKill"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Sauberer Abschluss"), TEXT("Toetet Execute sein Ziel, erhaelt Character 3 die Haelfte der verbrauchten Combo abgerundet zurueck."), {{TEXT("ComboRefundOnKillDivisor"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Zurueckgehaltene Klinge"), TEXT("Execute verbraucht hoechstens {MaxComboConsumed} Combo. Darueber hinaus vorhandene Combo bleibt erhalten und traegt nicht zum Schaden bei."), {{TEXT("MaxComboConsumed"), 3.0}}),
                MakeAbilitySkillpoint(TEXT("Alles oder nichts"), TEXT("Execute kann nur bei maximaler Combo eingesetzt werden. Der Schaden pro Combo wird verdoppelt und der Treffer ignoriert zusaetzlich {DefenseIgnoreAtMaxCombo} Defense."), {{TEXT("RequireMaxCombo"), 1.0}, {TEXT("ComboDamageMultiplier"), 2.0}, {TEXT("DefenseIgnoreAtMaxCombo"), 40.0}})
            }
        },
        {
            TEXT("hookshot"),
            TEXT("HookShot"),
            TEXT("Zieht Character 3 direkt auf ein freies Feld und eroeffnet neue Angriffs- und Fluchtwege."),
            {
                MakeAbilitySkillpoint(TEXT("Lange Kette"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Schneller Einzug"), TEXT("Verringert den Cooldown um {-Cooldown} Initiative."), {{TEXT("Cooldown"), -70.0}}),
                MakeAbilitySkillpoint(TEXT("Weiche Landung"), TEXT("Nach Hook Shot ist die naechste angrenzende normale Bewegung in diesem Zug kostenlos."), {{TEXT("GrantFreeAdjacentMove"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Leichter Haken"), TEXT("Verringert die Initiativekosten um {-InitiativeCost}."), {{TEXT("InitiativeCost"), -5.0}}),
                MakeAbilitySkillpoint(TEXT("Weiter Schwung"), TEXT("Bewegt Hook Shot Character 3 mindestens {ComboDistanceThreshold} Felder weit, erhaelt er {ComboGainAtDistance} Combo."), {{TEXT("ComboDistanceThreshold"), 3.0}, {TEXT("ComboGainAtDistance"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Ausweichende Landung"), TEXT("Der naechste eingehende Treffer vor dem naechsten Zug verursacht nur {NextIncomingDamageMultiplier}x Schaden."), {{TEXT("NextIncomingDamageMultiplier"), 0.75}}),
                MakeAbilitySkillpoint(TEXT("Lautlose Ankunft"), TEXT("Nach Hook Shot wird Character 3 fuer {InvisibilityDurationInitiative} Initiative unsichtbar. Die Unsichtbarkeit endet, sobald er Schaden verursacht."), {{TEXT("InvisibilityDurationInitiative"), 40.0}}),
                MakeAbilitySkillpoint(TEXT("Gezogene Klinge"), TEXT("Der erste normale Angriff nach Hook Shot in diesem Zug kostet {NextBasicAttackCostReduction} weniger Initiative."), {{TEXT("NextBasicAttackCostReduction"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Rettungshaken"), TEXT("Hook Shot kann auf das Feld eines Verbuendeten gezielt werden und tauscht die Positionen beider Characters."), {{TEXT("CanSwapWithAlly"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Doppelhaken"), TEXT("Hook Shot erhaelt {Charges} Aufladungen mit gemeinsamem Aufladezyklus, aber {-Range} Reichweite."), {{TEXT("Charges"), 2.0}, {TEXT("Range"), -1.0}}),
                MakeAbilitySkillpoint(TEXT("Schattenanker"), TEXT("Nach Hook Shot kann Character 3 bis zum Ende dieses Zuges einmal kostenlos auf das Ausgangsfeld zurueckkehren, sofern es frei ist."), {{TEXT("CreateReturnAnchor"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Fluchtroute"), TEXT("Toetet Character 3 nach Hook Shot im selben Zug einen Gegner, wird der aktuelle Cooldown von Hook Shot einmal pro Zug vollstaendig zurueckgesetzt."), {{TEXT("ResetCooldownOnKillAfterUse"), 1.0}})
            }
        },
        {
            TEXT("deathmark"),
            TEXT("Death Mark"),
            TEXT("Markiert ein Fokusziel und verstaerkt den Schaden von Character 3 waehrend der gesamten Dauer."),
            {
                MakeAbilitySkillpoint(TEXT("Weites Ziel"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Kurze Jagd"), TEXT("Verringert den Cooldown um {-Cooldown} Initiative."), {{TEXT("Cooldown"), -50.0}}),
                MakeAbilitySkillpoint(TEXT("Bekannte Beute"), TEXT("Wird ein bereits debufftes Ziel markiert, erhaelt Character 3 {InitiativeRefundVsDebuffed} Initiative zurueck."), {{TEXT("InitiativeRefundVsDebuffed"), 5.0}}),
                MakeAbilitySkillpoint(TEXT("Tiefe Markierung"), TEXT("Erhoeht den Schadensmultiplikator von Death Mark um {DamageMultiplierFromCauser}."), {{TEXT("DamageMultiplierFromCauser"), 0.05}}),
                MakeAbilitySkillpoint(TEXT("Schneller Aufbau"), TEXT("Der erste Treffer von Character 3 gegen das markierte Ziel gewaehrt {ComboGainOnTrigger} zusaetzliche Combo."), {{TEXT("ComboGainOnTrigger"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Verfolgung"), TEXT("Bewegt sich das markierte Ziel, wird die verbleibende Dauer einmalig um {MoveDurationExtension} Initiative verlaengert."), {{TEXT("MoveDurationExtension"), 30.0}}),
                MakeAbilitySkillpoint(TEXT("Keine Deckung"), TEXT("Death Mark entfernt bestehende Unsichtbarkeit des Ziels und verhindert neue Unsichtbarkeit waehrend seiner Dauer."), {{TEXT("PreventInvisibility"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Bereit zur Hinrichtung"), TEXT("Erreicht Character 3 durch Death Mark maximale Combo, wird der aktuelle Cooldown von Execute um {ExecuteCooldownReductionOnMaxCombo} Initiative reduziert."), {{TEXT("ExecuteCooldownReductionOnMaxCombo"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Unvollendete Jagd"), TEXT("Laeuft Death Mark aus, waehrend das Ziel lebt, wird sein aktueller Cooldown um {DeathMarkCooldownReductionOnExpiry} Initiative reduziert."), {{TEXT("DeathMarkCooldownReductionOnExpiry"), 100.0}}),
                MakeAbilitySkillpoint(TEXT("Wiederkehrende Gelegenheit"), TEXT("Der erste Treffer von Character 3 in jedem eigenen Zug gegen das markierte Ziel gewaehrt Combo, statt nur der erste Treffer der gesamten Markierung."), {{TEXT("GrantComboEachCauserTurn"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Todesurteil"), TEXT("Hat das markierte Ziel hoechstens 35% Leben, erhoeht sich der Schadensmultiplikator zusaetzlich um {LowHealthDamageMultiplierAdd}."), {{TEXT("LowHealthThreshold"), 0.35}, {TEXT("LowHealthDamageMultiplierAdd"), 0.15}}),
                MakeAbilitySkillpoint(TEXT("Kettenjagd"), TEXT("Stirbt das markierte Ziel durch Character 3, springt Death Mark einmal auf den naechsten Gegner innerhalb von {TransferRangeOnKill} Feldern ueber und behaelt seine Restdauer."), {{TEXT("TransferRangeOnKill"), 4.0}})
            }
        }
    };

    int32 UpdatedProgressions = 0;
    for (const FCharacter3AbilityDefaults& DefaultEntry : Defaults)
    {
        URoundSkillProgressionData* Progression = nullptr;
        if (const URoundSkillProgressionData* const* FoundProgression = ProgressionAssets.Find(DefaultEntry.SkillKey))
        {
            Progression = const_cast<URoundSkillProgressionData*>(*FoundProgression);
        }

        URoundAbilityData* AbilityData = nullptr;
        if (const URoundAbilityData* const* FoundAbility = AbilityAssets.Find(DefaultEntry.SkillKey))
        {
            AbilityData = const_cast<URoundAbilityData*>(*FoundAbility);
        }

        if (!Progression || !AbilityData)
        {
            UE_LOG(LogRoundAbilityRegistryImport, Warning, TEXT("PopulateCharacter3AbilityProgressionDefaults skipped %s - Progression=%s Ability=%s"),
                DefaultEntry.SkillKey,
                *GetNameSafe(Progression),
                *GetNameSafe(AbilityData));
            continue;
        }

        Progression->Modify();
        Progression->SkillId = AbilityData->AbilityId;
        Progression->DisplayName = FText::FromString(DefaultEntry.DisplayName);
        Progression->Description = FText::FromString(DefaultEntry.Description);
        Progression->Levels = DefaultEntry.Levels;
        Progression->MarkPackageDirty();

        AbilityData->Modify();
        AbilityData->SkillProgression = Progression;
        if (FCString::Stricmp(DefaultEntry.SkillKey, TEXT("hookshot")) == 0)
        {
            AbilityData->Description = FText::FromString(TEXT("Zieht Character 3 direkt auf ein freies, bewegbares Feld."));
            AbilityData->InitiativeCost = 15;
            AbilityData->Cooldown = 350;
            AbilityData->Range = 4;
            AbilityData->BaseDamage = 0;
            AbilityData->FieldState = ERoundFieldState::Empty;
            AbilityData->bRequiresTargetCharacter = false;
            AbilityData->bRequiresTargetTile = true;
        }
        else if (FCString::Stricmp(DefaultEntry.SkillKey, TEXT("deathmark")) == 0)
        {
            AbilityData->Description = FText::FromString(TEXT("Markiert ein Ziel und erhoeht waehrend der gesamten Dauer den Schaden von Character 3 gegen dieses Ziel."));
            AbilityData->InitiativeCost = 15;
            AbilityData->Cooldown = 220;
            AbilityData->Range = 4;
            AbilityData->BaseDamage = 0;
            AbilityData->FieldState = ERoundFieldState::Enemy;
            AbilityData->bRequiresTargetCharacter = true;
            AbilityData->bRequiresTargetTile = true;
            AbilityData->Values.FindOrAdd(TEXT("ComboGainOnTrigger")) = 1.0;
            AbilityData->Values.FindOrAdd(TEXT("DurationInitiative")) = 80.0;
            AbilityData->Values.FindOrAdd(TEXT("DamageMultiplierFromCauser")) = 1.15;
        }
        AbilityData->MarkPackageDirty();
        ++UpdatedProgressions;
    }

    MarkPackageDirty();
    UE_LOG(LogRoundAbilityRegistryImport, Log, TEXT("PopulateCharacter3AbilityProgressionDefaults updated %d Character 3 ability progressions"), UpdatedProgressions);
#endif
}

void URoundAbilityRegistryData::PopulateCharacter4AbilityProgressionDefaults()
{
#if WITH_EDITOR
    Modify();

    URoundAbilityData* CreatedAbilityData = nullptr;
    URoundSkillProgressionData* CreatedProgressionData = nullptr;
    EnsureExtendedAbilityDefinition(
        this,
        {
            TEXT("ArcaneShift"),
            TEXT("DA_Ability_ArcaneShift"),
            TEXT("DA_SkillProgression_ArcaneShift"),
            TEXT("ArcaneShift"),
            TEXT("Verlegt den Magier neu und oeffnet neue Zauberfenster."),
            ARoundArcaneShiftAbility::StaticClass(),
            20,
            210,
            3,
            0,
            ERoundDamageType::Magical,
            ERoundFieldState::Empty,
            false,
            true,
            {}
        },
        CreatedAbilityData,
        CreatedProgressionData);

    const TMap<FString, URoundSkillProgressionData*> ProgressionAssets = LoadAssetsByNormalizedName<URoundSkillProgressionData>();
    const TMap<FString, URoundAbilityData*> AbilityAssets = LoadAssetsByNormalizedName<URoundAbilityData>();

    struct FCharacter4AbilityDefaults
    {
        const TCHAR* SkillKey;
        const TCHAR* DisplayName;
        const TCHAR* Description;
        TArray<FRoundSkillpointDetails> Levels;
    };

    const TArray<FCharacter4AbilityDefaults> Defaults = {
        {
            TEXT("fireball"),
            TEXT("Fireball"),
            TEXT("Flaechenschaden, Zonenaufbau und Druck aus sicherer Distanz."),
            {
                MakeAbilitySkillpoint(TEXT("Schwere Flamme"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 100.0}}),
                MakeAbilitySkillpoint(TEXT("Kurze Zuendzeit"), TEXT("Verringert den Cooldown um {-Cooldown} Initiative."), {{TEXT("Cooldown"), -50.0}}),
                MakeAbilitySkillpoint(TEXT("Brennender Kern"), TEXT("Ein Gegner direkt auf dem Zielfeld erleidet {CenterDamageAdd} zusaetzlichen Schaden. Der Schaden auf umliegenden Feldern bleibt unveraendert."), {{TEXT("CenterDamageAdd"), 150.0}}),
                MakeAbilitySkillpoint(TEXT("Weite Detonation"), TEXT("Erhoeht den Explosionsradius um {DamageRange}. Ziele im neu hinzugekommenen aeusseren Ring erleiden nur {OuterRingDamageMultiplier}x Schaden."), {{TEXT("DamageRange"), 1.0}, {TEXT("OuterRingDamageMultiplier"), 0.6}}),
                MakeAbilitySkillpoint(TEXT("Feuerstelle"), TEXT("Das Zielfeld wird unabhaengig von seinem vorherigen Terrain zu Fire-Terrain."), {{TEXT("CreateFireTerrainOnTarget"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Druckwelle"), TEXT("Getroffene Gegner ausserhalb des Zielfeldes werden nach dem Schaden {KnockbackDistance} Feld vom Explosionszentrum weggestossen, sofern das Zielfeld frei und begehbar ist."), {{TEXT("KnockbackDistance"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Nachbrenner"), TEXT("Nach {AfterburnDelayInitiative} Initiative explodiert dasselbe Gebiet erneut und verursacht {AfterburnDamageMultiplier}x Schaden. Die zweite Explosion loest keine Fireball-Spezialeffekte aus."), {{TEXT("AfterburnDelayInitiative"), 40.0}, {TEXT("AfterburnDamageMultiplier"), 0.5}}),
                MakeAbilitySkillpoint(TEXT("Ueberhitzen"), TEXT("Erhoeht die Initiativekosten um {InitiativeCost}, verringert den Cooldown aber um {-Cooldown} Initiative."), {{TEXT("InitiativeCost"), 15.0}, {TEXT("Cooldown"), -100.0}}),
                MakeAbilitySkillpoint(TEXT("Explosiver Rueckstoss"), TEXT("Nach der Explosion bewegt sich Character 4 kostenlos {SelfRecoilDistance} Feld direkt vom Zielfeld weg, sofern dieses Feld frei und begehbar ist."), {{TEXT("SelfRecoilDistance"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Meteor"), TEXT("Fireball schlaegt erst nach {DelayedExecutionInitiative} Initiative ein. Dafuer erhaelt er {Damage} Schaden und {DamageRange} Explosionsradius. Bis zum Einschlag kann Character 4 keinen neuen Zug beginnen."), {{TEXT("DelayedExecutionInitiative"), 50.0}, {TEXT("Damage"), 300.0}, {TEXT("DamageRange"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Inferno"), TEXT("Getroffene Gegner erleiden fuer {BurnDurationInitiative} Initiative zu Beginn ihres Zuges jeweils {BurnDamageOnTurnStart} Feuerschaden."), {{TEXT("BurnDamageOnTurnStart"), 70.0}, {TEXT("BurnDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Schmelzpunkt"), TEXT("Jeder getroffene Gegner verliert nach dem Treffer fuer {DefenseReductionDurationInitiative} Initiative {DefenseReductionOnHit} Defense. Der ausloesende Fireball profitiert noch nicht davon."), {{TEXT("DefenseReductionOnHit"), 25.0}, {TEXT("DefenseReductionDurationInitiative"), 80.0}})
            }
        },
        {
            TEXT("icebeam"),
            TEXT("Icebeam"),
            TEXT("Zuverlaessige Einzelzielkontrolle, Bewegungseinschraenkung und Vorbereitung weiterer Zauber."),
            {
                MakeAbilitySkillpoint(TEXT("Schneidende Kaelte"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Schneller Frost"), TEXT("Verringert den Cooldown um {-Cooldown} Initiative."), {{TEXT("Cooldown"), -50.0}}),
                MakeAbilitySkillpoint(TEXT("Langer Winter"), TEXT("Der Slow haelt {SlowDuration} Initiative laenger."), {{TEXT("SlowDuration"), 30.0}}),
                MakeAbilitySkillpoint(TEXT("Kaelteschock"), TEXT("Das getroffene Ziel verliert sofort {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Eisfessel"), TEXT("Die erste Bewegung des Ziels waehrend des Slows kostet zusaetzlich {FirstMoveInitiativeLoss} Initiative. Danach ist die Eisfessel verbraucht."), {{TEXT("FirstMoveInitiativeLoss"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Splitterfrost"), TEXT("Gegner direkt neben dem Ziel erhalten {SplashSlowAmount} Slow fuer {SplashSlowDurationInitiative} Initiative. Sie erleiden keinen Schaden."), {{TEXT("SplashSlowAmount"), 0.25}, {TEXT("SplashSlowDurationInitiative"), 50.0}}),
                MakeAbilitySkillpoint(TEXT("Frostkette"), TEXT("Icebeam springt zum naechsten anderen Gegner innerhalb von {ChainSearchRange} Feldern. Dieser erleidet {ChainDamageMultiplier}x Schaden und erhaelt {ChainSlowAmount} Slow fuer {ChainSlowDurationInitiative} Initiative."), {{TEXT("ChainSearchRange"), 2.0}, {TEXT("ChainDamageMultiplier"), 0.6}, {TEXT("ChainSlowAmount"), 0.25}, {TEXT("ChainSlowDurationInitiative"), 50.0}}),
                MakeAbilitySkillpoint(TEXT("Frostbruch"), TEXT("Der naechste Schadensskill von Character 4 gegen das verlangsamte Ziel verbraucht den Slow und verursacht {FrostbreakDamage} zusaetzlichen Eisschaden. Normale Angriffe loesen Frostbruch nicht aus."), {{TEXT("FrostbreakDamage"), 150.0}}),
                MakeAbilitySkillpoint(TEXT("Permafrost"), TEXT("Der Slow haelt maximal {SlowDuration} Initiative, endet aber vorzeitig, nachdem sich das Ziel insgesamt {SlowEndsAfterMovedFields} Felder bewegt hat."), {{TEXT("SlowDuration"), 70.0}, {TEXT("SlowEndsAfterMovedFields"), 3.0}}),
                MakeAbilitySkillpoint(TEXT("Durchdringender Frost"), TEXT("Icebeam verlaeuft nach dem Ziel {PiercingAdditionalRange} Felder weiter. Weitere Gegner erleiden {PiercingDamageMultiplier}x Schaden und erhalten den vollen Slow."), {{TEXT("PiercingAdditionalRange"), 3.0}, {TEXT("PiercingDamageMultiplier"), 0.7}}),
                MakeAbilitySkillpoint(TEXT("Absoluter Nullpunkt"), TEXT("Ist das Ziel bereits von Icebeam verlangsamt, wird der Slow entfernt und es kann sich {RootDurationInitiative} Initiative lang nicht bewegen."), {{TEXT("RootIfAlreadySlowed"), 1.0}, {TEXT("RootDurationInitiative"), 60.0}}),
                MakeAbilitySkillpoint(TEXT("Sproedes Eis"), TEXT("Das getroffene Ziel erleidet nach Icebeam fuer {BrittleDurationInitiative} Initiative {BrittleIncomingDamageMultiplier}x Schaden aus allen Quellen."), {{TEXT("BrittleIncomingDamageMultiplier"), 1.2}, {TEXT("BrittleDurationInitiative"), 80.0}})
            }
        },
        {
            TEXT("lightningstrike"),
            TEXT("Lightningstrike"),
            TEXT("Linienzauber fuer Tempo, Initiativdruck und flexible Schadensfenster."),
            {
                MakeAbilitySkillpoint(TEXT("Sturmbolzen"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Kurzer Strom"), TEXT("Verringert den Cooldown um {-Cooldown} Initiative."), {{TEXT("Cooldown"), -40.0}}),
                MakeAbilitySkillpoint(TEXT("Langer Lichtbogen"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Leitender Schlag"), TEXT("Jeder getroffene Gegner verliert {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Ueberladung"), TEXT("Trifft Lightningstrike mindestens 2 Gegner, erhaelt Character 4 {InitiativeGainOnMultiHit} Initiative."), {{TEXT("InitiativeGainOnMultiHit"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Endverzweigung"), TEXT("Am Endfeld springt der Blitz auf noch nicht getroffene angrenzende Gegner ueber und verursacht {EndpointForkDamageMultiplier}x Schaden."), {{TEXT("EndpointForkDamageMultiplier"), 0.5}}),
                MakeAbilitySkillpoint(TEXT("Steigende Spannung"), TEXT("Jeder zuvor getroffene Gegner erhoeht den Schaden gegen den naechsten Gegner der Linie um {DamageMultiplierAddPerPreviousTarget} des Grundschadens."), {{TEXT("DamageMultiplierAddPerPreviousTarget"), 0.2}}),
                MakeAbilitySkillpoint(TEXT("Statische Ladung"), TEXT("Getroffene Gegner werden fuer {StaticChargeDurationInitiative} Initiative geladen. Ihre naechste Bewegung verursacht {StaticChargeMoveDamage} Blitzschaden und verbraucht die Ladung."), {{TEXT("StaticChargeMoveDamage"), 120.0}, {TEXT("StaticChargeDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Panzerblitz"), TEXT("Lightningstrike ignoriert {DefenseIgnore} Defense."), {{TEXT("DefenseIgnore"), 25.0}}),
                MakeAbilitySkillpoint(TEXT("Kettenblitz"), TEXT("Nach dem letzten getroffenen Gegner springt der Blitz zum naechsten noch nicht getroffenen Gegner innerhalb von {FinalChainRange} Feldern und verursacht {FinalChainDamageMultiplier}x Schaden."), {{TEXT("FinalChainRange"), 3.0}, {TEXT("FinalChainDamageMultiplier"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Rueckstrom"), TEXT("Jeder getroffene Gegner reduziert den aktuellen Cooldown um {CooldownReductionPerEnemyHit} Initiative, maximal um {MaximumHitCooldownReduction}."), {{TEXT("CooldownReductionPerEnemyHit"), 30.0}, {TEXT("MaximumHitCooldownReduction"), 90.0}}),
                MakeAbilitySkillpoint(TEXT("Gewitterkern"), TEXT("Erhoeht den Schaden um {Damage}, aber auch die Initiativekosten um {InitiativeCost}."), {{TEXT("Damage"), 200.0}, {TEXT("InitiativeCost"), 10.0}})
            }
        },
        {
            TEXT("rockblast"),
            TEXT("Rockblast"),
            TEXT("Raumkontrolle, Blockaden und sichere Vorbereitung fuer den naechsten Zauber."),
            {
                MakeAbilitySkillpoint(TEXT("Schwerer Brocken"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 90.0}}),
                MakeAbilitySkillpoint(TEXT("Schneller Einschlag"), TEXT("Verringert den Cooldown um {-Cooldown} Initiative."), {{TEXT("Cooldown"), -60.0}}),
                MakeAbilitySkillpoint(TEXT("Weiter Wurf"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Breiter Wall"), TEXT("Erzeugt {AdditionalRockCount} zusaetzliches Rocks-Feld neben dem Einschlag, falls moeglich."), {{TEXT("AdditionalRockCount"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Erschuetterung"), TEXT("Das getroffene Ziel verliert {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Felsensplitter"), TEXT("Trifft Rockblast ein bestehendes Rocks-Feld, wird es zu Rubble und angrenzende Gegner erleiden {ShatterSplashDamageMultiplier}x Rockblast-Schaden."), {{TEXT("ShatterSplashDamageMultiplier"), 0.6}}),
                MakeAbilitySkillpoint(TEXT("Zermalmender Einschlag"), TEXT("Steht das Ziel neben Rocks oder einer Wall, erleidet es {BonusDamageNearRockOrWall} zusaetzlichen Schaden."), {{TEXT("BonusDamageNearRockOrWall"), 150.0}}),
                MakeAbilitySkillpoint(TEXT("Steingefaengnis"), TEXT("Ist das Ziel nach Rockblast von mindestens {RequiredAdjacentRocks} Rocks-Feldern umgeben, kann es sich {RockPrisonDurationInitiative} Initiative lang nicht bewegen."), {{TEXT("RootIfSurroundedByRocks"), 1.0}, {TEXT("RequiredAdjacentRocks"), 3.0}, {TEXT("RockPrisonDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Steindeckung"), TEXT("Erzeugt Rockblast mindestens ein Rocks-Feld, erleidet Character 4 fuer {RockCoverDurationInitiative} Initiative nur {SelfIncomingDamageMultiplierAfterRockCreate}x Schaden."), {{TEXT("SelfIncomingDamageMultiplierAfterRockCreate"), 0.8}, {TEXT("RockCoverDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Festung"), TEXT("Erzeugt bis zu {AdditionalRockCount} zusaetzliche Rocks-Felder neben dem Einschlag."), {{TEXT("AdditionalRockCount"), 3.0}}),
                MakeAbilitySkillpoint(TEXT("Erdrutsch"), TEXT("Gegner neben neu erzeugten Rocks erleiden einmalig {DamageNearCreatedRocks} Ground-Schaden und werden ein freies Feld vom Einschlag weggestossen."), {{TEXT("DamageNearCreatedRocks"), 120.0}}),
                MakeAbilitySkillpoint(TEXT("Seismisches Echo"), TEXT("Nach {RockEchoDelayInitiative} Initiative verursachen die neu erzeugten, noch bestehenden Rocks jeweils {RockEchoDamage} Schaden an angrenzenden Gegnern. Jeder Gegner wird nur einmal getroffen."), {{TEXT("RockEchoDelayInitiative"), 40.0}, {TEXT("RockEchoDamage"), 150.0}})
            }
        },
        {
            TEXT("arcaneshift"),
            TEXT("ArcaneShift"),
            TEXT("Verlegt den Magier neu und oeffnet neue Zauberfenster."),
            {
                MakeAbilitySkillpoint(TEXT("Weiter Sprung"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Kurzer Riss"), TEXT("Verringert den Cooldown um {-Cooldown} Initiative."), {{TEXT("Cooldown"), -40.0}}),
                MakeAbilitySkillpoint(TEXT("Leichter Shift"), TEXT("Verringert die Initiativekosten um {-InitiativeCost}."), {{TEXT("InitiativeCost"), -5.0}}),
                MakeAbilitySkillpoint(TEXT("Stoerender Abgang"), TEXT("Gegner neben dem Startfeld verlieren {InitiativeLossAroundStart} Initiative."), {{TEXT("InitiativeLossAroundStart"), 12.0}}),
                MakeAbilitySkillpoint(TEXT("Phasenschutz"), TEXT("Der naechste Treffer innerhalb von {NextHitProtectionDurationInitiative} Initiative verursacht nur {NextHitIncomingDamageMultiplier}x Schaden."), {{TEXT("NextHitIncomingDamageMultiplier"), 0.8}, {TEXT("NextHitProtectionDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Positionswechsel"), TEXT("Arcane Shift kann auf einen Verbuendeten gezielt werden und tauscht dessen Position mit Character 4."), {{TEXT("CanSwapWithAlly"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Sichere Ankunft"), TEXT("Steht nach dem Shift kein Gegner neben Character 4, erhaelt er {InitiativeRefundIfNoAdjacentEnemy} Initiative zurueck."), {{TEXT("InitiativeRefundIfNoAdjacentEnemy"), 12.0}}),
                MakeAbilitySkillpoint(TEXT("Arkane Welle"), TEXT("Gegner neben dem Zielfeld erleiden nach dem Shift {DamageAroundDestination} magischen Schaden."), {{TEXT("DamageAroundDestination"), 120.0}}),
                MakeAbilitySkillpoint(TEXT("Phasenspur"), TEXT("Gegner neben Start- oder Zielfeld erhalten fuer {ShiftEndpointDebuffDurationInitiative} Initiative +{MoveCostAddNearShiftEndpoints} MoveCost."), {{TEXT("MoveCostAddNearShiftEndpoints"), 2.0}, {TEXT("ShiftEndpointDebuffDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Nachbild"), TEXT("Der naechste Treffer innerhalb von {NextHitProtectionDurationInitiative} Initiative verursacht keinen Schaden."), {{TEXT("NegateNextHit"), 1.0}, {TEXT("NextHitProtectionDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Raumbruch"), TEXT("Gegner neben dem Start- oder Zielfeld erleiden {SpatialRuptureDamage} magischen Schaden. Ein Gegner kann nur einmal getroffen werden."), {{TEXT("SpatialRuptureDamage"), 200.0}}),
                MakeAbilitySkillpoint(TEXT("Weitsprung"), TEXT("Erhoeht die Reichweite um {Range}, aber auch den Cooldown um {Cooldown} Initiative."), {{TEXT("Range"), 3.0}, {TEXT("Cooldown"), 60.0}})
            }
        }
    };

    int32 UpdatedProgressions = 0;
    for (const FCharacter4AbilityDefaults& DefaultEntry : Defaults)
    {
        URoundSkillProgressionData* Progression = nullptr;
        if (const URoundSkillProgressionData* const* FoundProgression = ProgressionAssets.Find(DefaultEntry.SkillKey))
        {
            Progression = const_cast<URoundSkillProgressionData*>(*FoundProgression);
        }

        URoundAbilityData* AbilityData = nullptr;
        if (const URoundAbilityData* const* FoundAbility = AbilityAssets.Find(DefaultEntry.SkillKey))
        {
            AbilityData = const_cast<URoundAbilityData*>(*FoundAbility);
        }

        if (!Progression || !AbilityData)
        {
            UE_LOG(LogRoundAbilityRegistryImport, Warning, TEXT("PopulateCharacter4AbilityProgressionDefaults skipped %s - Progression=%s Ability=%s"),
                DefaultEntry.SkillKey,
                *GetNameSafe(Progression),
                *GetNameSafe(AbilityData));
            continue;
        }

        Progression->Modify();
        Progression->SkillId = AbilityData->AbilityId;
        Progression->DisplayName = FText::FromString(DefaultEntry.DisplayName);
        Progression->Description = FText::FromString(DefaultEntry.Description);
        Progression->Levels = DefaultEntry.Levels;
        Progression->MarkPackageDirty();

        AbilityData->Modify();
        AbilityData->SkillProgression = Progression;
        AbilityData->MarkPackageDirty();
        ++UpdatedProgressions;
    }

    MarkPackageDirty();
    UE_LOG(LogRoundAbilityRegistryImport, Log, TEXT("PopulateCharacter4AbilityProgressionDefaults updated %d Character 4 ability progressions"), UpdatedProgressions);
#endif
}

void URoundAbilityRegistryData::PopulateCharacter5AbilityProgressionDefaults()
{
#if WITH_EDITOR
    Modify();

    URoundAbilityData* CreatedAbilityData = nullptr;
    URoundSkillProgressionData* CreatedProgressionData = nullptr;
    EnsureExtendedAbilityDefinition(
        this,
        {
            TEXT("Rend"),
            TEXT("DA_Ability_Rend"),
            TEXT("DA_SkillProgression_Rend"),
            TEXT("Rend"),
            TEXT("Reisst Wunden auf, jagt verwundete Ziele und laesst sie ausbluten."),
            ARoundRendAbility::StaticClass(),
            20,
            200,
            1,
            170,
            ERoundDamageType::Physical,
            ERoundFieldState::Enemy,
            true,
            false,
            {
                {TEXT("BleedDamagePerTurn"), 45.0},
                {TEXT("BleedDurationInitiative"), 80.0}
            }
        },
        CreatedAbilityData,
        CreatedProgressionData);

    const TMap<FString, URoundSkillProgressionData*> ProgressionAssets = LoadAssetsByNormalizedName<URoundSkillProgressionData>();
    const TMap<FString, URoundAbilityData*> AbilityAssets = LoadAssetsByNormalizedName<URoundAbilityData>();

    struct FCharacter5AbilityDefaults
    {
        const TCHAR* SkillKey;
        const TCHAR* DisplayName;
        const TCHAR* Description;
        TArray<FRoundSkillpointDetails> Levels;
    };

    const TArray<FCharacter5AbilityDefaults> Defaults = {
        {
            TEXT("dash"),
            TEXT("Dash"),
            TEXT("Schliesst Distanz, trifft ein Einzelziel und bringt Character 5 direkt in den Nahkampf."),
            {
                MakeAbilitySkillpoint(TEXT("Langer Anlauf"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Kurze Jagd"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -50.0}}),
                MakeAbilitySkillpoint(TEXT("Harter Aufprall"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Mitten rein"), TEXT("Nach Dash erleidet Character 5 fuer {EffectDurationInitiative} Initiative nur {SelfIncomingDamageMultiplierAfterUse}x Schaden."), {{TEXT("SelfIncomingDamageMultiplierAfterUse"), 0.8}, {TEXT("EffectDurationInitiative"), 60.0}}),
                MakeAbilitySkillpoint(TEXT("Festgenagelt"), TEXT("Das Ziel erhaelt fuer {EffectDurationInitiative} Initiative +{MoveCostAddOnTarget} MoveCost."), {{TEXT("MoveCostAddOnTarget"), 2.0}, {TEXT("EffectDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Blutspur"), TEXT("Character 5 erhaelt zusaetzliches Blut in Hoehe von {BonusBloodFractionOfDamage}x des mit Dash verursachten Schadens."), {{TEXT("BonusBloodFractionOfDamage"), 0.05}}),
                MakeAbilitySkillpoint(TEXT("Schockwelle"), TEXT("Gegner direkt neben dem Ziel erleiden {ImpactSplashDamageMultiplier}x Dash-Schaden."), {{TEXT("ImpactSplashDamageMultiplier"), 0.5}}),
                MakeAbilitySkillpoint(TEXT("Anlauf"), TEXT("Bewegt sich Dash mindestens {MomentumRequiredDistance} Felder, verursacht er {MomentumDamage} zusaetzlichen Schaden."), {{TEXT("MomentumRequiredDistance"), 3.0}, {TEXT("MomentumDamage"), 100.0}}),
                MakeAbilitySkillpoint(TEXT("Jagdrausch"), TEXT("Hat das Ziel nach dem Treffer hoechstens {InitiativeRefundHealthThreshold} seines Lebens, erhaelt Character 5 {InitiativeRefundVsLowHealth} Initiative zurueck."), {{TEXT("InitiativeRefundHealthThreshold"), 0.5}, {TEXT("InitiativeRefundVsLowHealth"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Durchziehen"), TEXT("Nach dem Treffer bewegt sich Character 5 auf ein freies Feld hinter dem Ziel, wenn eines vorhanden ist."), {{TEXT("CarryThrough"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Zerreissen"), TEXT("Der naechste Treffer von Character 5 gegen das Ziel verursacht innerhalb von {MarkDurationInitiative} Initiative {NextHitDamageMultiplier}x Schaden."), {{TEXT("NextHitDamageMultiplier"), 1.3}, {TEXT("MarkDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Bluthund"), TEXT("Besiegt Dash das Ziel, sinkt sein aktueller Cooldown um {CooldownReductionOnKill} Initiative."), {{TEXT("CooldownReductionOnKill"), 140.0}})
            }
        },
        {
            TEXT("decimate"),
            TEXT("Decimate"),
            TEXT("Ein langsamer, schwerer Nahkampfschlag, der besonders gegen sehr gesunde oder beinahe besiegte Ziele spezialisiert werden kann."),
            {
                MakeAbilitySkillpoint(TEXT("Schwerer Hieb"), TEXT("Erhoeht den Grundschaden um {Damage}."), {{TEXT("Damage"), 100.0}}),
                MakeAbilitySkillpoint(TEXT("Brecher"), TEXT("Erhoeht den Schaden um weitere {PercentDamage} der maximalen Lebenspunkte des Ziels."), {{TEXT("PercentDamage"), 0.05}}),
                MakeAbilitySkillpoint(TEXT("Kurzer Schwung"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -25.0}}),
                MakeAbilitySkillpoint(TEXT("Vampirschlag"), TEXT("Character 5 heilt sich um {LifestealFractionOnHit}x des verursachten Schadens."), {{TEXT("LifestealFractionOnHit"), 0.2}}),
                MakeAbilitySkillpoint(TEXT("Ruestungsbruch"), TEXT("Treffer lassen Character 5 fuer {DebuffDurationInitiative} Initiative {DefenseReduction} Defense des Ziels ignorieren."), {{TEXT("DefenseReduction"), 20.0}, {TEXT("DebuffDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Schaedelriss"), TEXT("Das Ziel verliert {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Spalter"), TEXT("Ziele mit mindestens {HighHealthThreshold} Leben erleiden {DamageMultiplierVsHighHealth}x Schaden."), {{TEXT("HighHealthThreshold"), 0.75}, {TEXT("DamageMultiplierVsHighHealth"), 1.25}}),
                MakeAbilitySkillpoint(TEXT("Henkerhieb"), TEXT("Ziele mit hoechstens {LowHealthThreshold} Leben erleiden {DamageMultiplierVsLowHealth}x Schaden."), {{TEXT("LowHealthThreshold"), 0.35}, {TEXT("DamageMultiplierVsLowHealth"), 1.35}}),
                MakeAbilitySkillpoint(TEXT("Blutopfer"), TEXT("Besitzt Character 5 mindestens {BloodCostForBonusDamage} Blut, verbraucht Decimate davon {BloodCostForBonusDamage} und verursacht {BonusDamageForBlood} zusaetzlichen Schaden."), {{TEXT("BloodCostForBonusDamage"), 25.0}, {TEXT("BonusDamageForBlood"), 200.0}}),
                MakeAbilitySkillpoint(TEXT("Abschlachten"), TEXT("Besiegt Decimate ein Ziel, erhaelt Character 5 {InitiativeGainOnKill} Initiative."), {{TEXT("InitiativeGainOnKill"), 20.0}}),
                MakeAbilitySkillpoint(TEXT("Ueberkill"), TEXT("Verursacht Decimate mehr Schaden als zum Toeten noetig war, erhaelt Character 5 {OverkillBloodFraction}x des Overkills als Blut."), {{TEXT("OverkillBloodFraction"), 0.5}}),
                MakeAbilitySkillpoint(TEXT("Breiter Spalt"), TEXT("Gegner direkt neben dem Ziel erleiden {CleaveDamageMultiplier}x Decimate-Grundschaden."), {{TEXT("CleaveDamageMultiplier"), 0.6}})
            }
        },
        {
            TEXT("spintowin"),
            TEXT("SpinToWin"),
            TEXT("Mehrziel-Druck fuer den direkten Nahkampf mitten im Gegnerteam."),
            {
                MakeAbilitySkillpoint(TEXT("Schwere Drehung"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Weiter Schwung"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Leichte Drehung"), TEXT("Verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("InitativeCost"), -8.0}}),
                MakeAbilitySkillpoint(TEXT("Blutkreis"), TEXT("Character 5 heilt sich pro getroffenem Gegner um {HealPerTarget} Leben."), {{TEXT("HealPerTarget"), 50.0}}),
                MakeAbilitySkillpoint(TEXT("Mitten im Gemetzel"), TEXT("Trifft SpinToWin mindestens 2 Gegner, erleidet Character 5 fuer {GuardDurationInitiative} Initiative nur {SelfIncomingDamageMultiplierIfMultiHit}x Schaden."), {{TEXT("SelfIncomingDamageMultiplierIfMultiHit"), 0.8}, {TEXT("GuardDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Kampfrausch"), TEXT("Fuer jeden getroffenen Gegner nach dem ersten erhaelt Character 5 {InitiativePerExtraTarget} Initiative, maximal 20."), {{TEXT("InitiativePerExtraTarget"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Fleischwolf"), TEXT("Ziele mit hoechstens {LowHealthThreshold} Leben erleiden {DamageMultiplierVsLowHealth}x Schaden."), {{TEXT("LowHealthThreshold"), 0.5}, {TEXT("DamageMultiplierVsLowHealth"), 1.25}}),
                MakeAbilitySkillpoint(TEXT("Blutsog"), TEXT("Character 5 erhaelt zusaetzliches Blut in Hoehe von {BonusBloodFractionOfDamage}x des insgesamt verursachten Schadens."), {{TEXT("BonusBloodFractionOfDamage"), 0.05}}),
                MakeAbilitySkillpoint(TEXT("Aufreissen"), TEXT("Getroffene Gegner bluten fuer {BleedDurationInitiative} Initiative und erleiden bei Zugbeginn {BleedDamagePerTurn} Schaden."), {{TEXT("BleedDamagePerTurn"), 35.0}, {TEXT("BleedDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Doppelwirbel"), TEXT("Nach {EchoDelayInitiative} Initiative wiederholt sich SpinToWin am aktuellen Standort mit {EchoDamageMultiplier}x Schaden."), {{TEXT("EchoDelayInitiative"), 40.0}, {TEXT("EchoDamageMultiplier"), 0.5}}),
                MakeAbilitySkillpoint(TEXT("Endloser Wirbel"), TEXT("Der aktuelle Cooldown sinkt pro getroffenem Gegner um {CooldownReductionPerTarget}, maximal {MaxCooldownReduction} Initiative."), {{TEXT("CooldownReductionPerTarget"), 25.0}, {TEXT("MaxCooldownReduction"), 100.0}}),
                MakeAbilitySkillpoint(TEXT("Roter Nebel"), TEXT("Trifft SpinToWin mindestens {RedMistRequiredTargets} Gegner, erhaelt Character 5 {RedMistBloodGain} Blut und {RedMistInitiativeGain} Initiative."), {{TEXT("RedMistRequiredTargets"), 3.0}, {TEXT("RedMistBloodGain"), 25.0}, {TEXT("RedMistInitiativeGain"), 15.0}})
            }
        },
        {
            TEXT("heal"),
            TEXT("Heal"),
            TEXT("Verbraucht gespeichertes Blut fuer Selbstheilung und entscheidet damit zwischen sofortigem Ueberleben und dem Behalten offensiver Blutboni."),
            {
                MakeAbilitySkillpoint(TEXT("Dichte Gerinnung"), TEXT("Erhoeht die Basisheilung um {BaseHeal}."), {{TEXT("BaseHeal"), 100.0}}),
                MakeAbilitySkillpoint(TEXT("Tiefer Zug"), TEXT("Erhoeht die Heilung pro verbrauchtem Blut um {HealFromBlood}."), {{TEXT("HealFromBlood"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Schnelle Gerinnung"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -50.0}}),
                MakeAbilitySkillpoint(TEXT("Vampirreserve"), TEXT("Nach Heal erhaelt Character 5 {BloodRetainFraction}x des verbrauchten Bluts zurueck."), {{TEXT("BloodRetainFraction"), 0.25}}),
                MakeAbilitySkillpoint(TEXT("Schmerzmittel"), TEXT("Nach Heal erleidet Character 5 fuer {EffectDurationInitiative} Initiative nur {IncomingDamageMultiplierAfterUse}x Schaden."), {{TEXT("IncomingDamageMultiplierAfterUse"), 0.75}, {TEXT("EffectDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Reines Blut"), TEXT("Heal entfernt einen entfernbaren Debuff von Character 5."), {{TEXT("CleanseOneDebuff"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Blutdruck"), TEXT("Nach Heal verursacht Character 5 fuer {EffectDurationInitiative} Initiative {OutgoingDamageMultiplierAfterUse}x Schaden."), {{TEXT("OutgoingDamageMultiplierAfterUse"), 1.2}, {TEXT("EffectDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Blutraub"), TEXT("Nach Heal heilt sich Character 5 fuer {EffectDurationInitiative} Initiative um {LifestealFractionAfterUse}x seines verursachten Schadens."), {{TEXT("LifestealFractionAfterUse"), 0.2}, {TEXT("EffectDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Schmerzensschrei"), TEXT("Nach Heal verlieren direkt benachbarte Gegner {AdjacentInitiativeLoss} Initiative."), {{TEXT("AdjacentInitiativeLoss"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Kontrollierte Transfusion"), TEXT("Heal verbraucht hoechstens {MaximumBloodConsumed} Blut; der Rest bleibt gespeichert."), {{TEXT("MaximumBloodConsumed"), 50.0}}),
                MakeAbilitySkillpoint(TEXT("Blutwelle"), TEXT("Direkt benachbarte Gegner erleiden Schaden in Hoehe von {AdjacentDamageFractionOfHealing}x der tatsaechlichen Heilung."), {{TEXT("AdjacentDamageFractionOfHealing"), 0.75}}),
                MakeAbilitySkillpoint(TEXT("Letzte Reserve"), TEXT("Unter {CriticalHealthThreshold} Leben wird die Heilung aus verbrauchtem Blut mit {BloodHealMultiplierWhileCritical} multipliziert."), {{TEXT("CriticalHealthThreshold"), 0.25}, {TEXT("BloodHealMultiplierWhileCritical"), 2.0}})
            }
        },
        {
            TEXT("rend"),
            TEXT("Rend"),
            TEXT("Reisst Wunden auf, jagt verwundete Ziele und laesst sie ausbluten."),
            {
                MakeAbilitySkillpoint(TEXT("Tiefer Schnitt"), TEXT("Erhoeht den direkten Schaden um {Damage}."), {{TEXT("Damage"), 70.0}}),
                MakeAbilitySkillpoint(TEXT("Starkes Ausbluten"), TEXT("Erhoeht den Bleed-Schaden pro Zug um {BleedDamagePerTurn}."), {{TEXT("BleedDamagePerTurn"), 25.0}}),
                MakeAbilitySkillpoint(TEXT("Schneller Schnitt"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -40.0}}),
                MakeAbilitySkillpoint(TEXT("Gerissene Sehne"), TEXT("Das Ziel erhaelt fuer {DebuffDurationInitiative} Initiative +{MoveCostAddOnTarget} MoveCost."), {{TEXT("MoveCostAddOnTarget"), 2.0}, {TEXT("DebuffDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Blutprobe"), TEXT("Rend erzeugt sofort zusaetzlich {ImmediateBloodGain} Blut."), {{TEXT("ImmediateBloodGain"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Langsame Gerinnung"), TEXT("Erhoeht die Bleed-Dauer um {BleedDurationInitiative} Initiative."), {{TEXT("BleedDurationInitiative"), 40.0}}),
                MakeAbilitySkillpoint(TEXT("Wunde aufreissen"), TEXT("Ist das Ziel bereits von Rend betroffen, verursacht die vorhandene Wunde sofort einen zusaetzlichen Bleed-Tick, bevor sie erneuert wird."), {{TEXT("ImmediateBleedTickOnReapply"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Bewegungsriss"), TEXT("Die erste Bewegung des Ziels waehrend Rend verursacht zusaetzlich {MovementTriggerDamage} Schaden."), {{TEXT("MovementTriggerDamage"), 120.0}}),
                MakeAbilitySkillpoint(TEXT("Offene Wunde"), TEXT("Der naechste Treffer von Character 5 gegen das Ziel verursacht innerhalb von {DebuffDurationInitiative} Initiative {IncomingDamageMultiplierFromCauser}x Schaden."), {{TEXT("IncomingDamageMultiplierFromCauser"), 1.2}, {TEXT("DebuffDurationInitiative"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Blutende Jagd"), TEXT("Jeder Bleed-Tick von Rend erzeugt zusaetzlich {BloodPerBleedTick} Blut."), {{TEXT("BloodPerBleedTick"), 8.0}}),
                MakeAbilitySkillpoint(TEXT("Verbluten"), TEXT("Erhoeht den Bleed-Schaden um {BleedDamagePerTurn} und seine Dauer um {BleedDurationInitiative} Initiative."), {{TEXT("BleedDamagePerTurn"), 45.0}, {TEXT("BleedDurationInitiative"), 40.0}}),
                MakeAbilitySkillpoint(TEXT("Nicht entkommen"), TEXT("Rend erhaelt {Range} Reichweite und das Ziel erhaelt fuer {DebuffDurationInitiative} Initiative +{MoveCostAddOnTarget} MoveCost."), {{TEXT("Range"), 1.0}, {TEXT("MoveCostAddOnTarget"), 2.0}, {TEXT("DebuffDurationInitiative"), 120.0}})
            }
        }
    };

    int32 UpdatedProgressions = 0;
    for (const FCharacter5AbilityDefaults& DefaultEntry : Defaults)
    {
        URoundSkillProgressionData* Progression = nullptr;
        if (const URoundSkillProgressionData* const* FoundProgression = ProgressionAssets.Find(DefaultEntry.SkillKey))
        {
            Progression = const_cast<URoundSkillProgressionData*>(*FoundProgression);
        }

        URoundAbilityData* AbilityData = nullptr;
        if (const URoundAbilityData* const* FoundAbility = AbilityAssets.Find(DefaultEntry.SkillKey))
        {
            AbilityData = const_cast<URoundAbilityData*>(*FoundAbility);
        }

        if (!Progression || !AbilityData)
        {
            UE_LOG(LogRoundAbilityRegistryImport, Warning, TEXT("PopulateCharacter5AbilityProgressionDefaults skipped %s - Progression=%s Ability=%s"),
                DefaultEntry.SkillKey,
                *GetNameSafe(Progression),
                *GetNameSafe(AbilityData));
            continue;
        }

        Progression->Modify();
        Progression->SkillId = AbilityData->AbilityId;
        Progression->DisplayName = FText::FromString(DefaultEntry.DisplayName);
        Progression->Description = FText::FromString(DefaultEntry.Description);
        Progression->Levels = DefaultEntry.Levels;
        Progression->MarkPackageDirty();

        AbilityData->Modify();
        AbilityData->Values.Remove(TEXT("Damage"));
        AbilityData->Values.Remove(TEXT("Cooldown"));
        AbilityData->Values.Remove(TEXT("InitativeCost"));
        AbilityData->Values.Remove(TEXT("InitiativeCost"));
        AbilityData->Values.Remove(TEXT("Range"));
        if (FString(DefaultEntry.SkillKey).Equals(TEXT("decimate"), ESearchCase::IgnoreCase))
        {
            AbilityData->Range = 1;
        }
        AbilityData->SkillProgression = Progression;
        AbilityData->MarkPackageDirty();
        ++UpdatedProgressions;
    }

    MarkPackageDirty();
    UE_LOG(LogRoundAbilityRegistryImport, Log, TEXT("PopulateCharacter5AbilityProgressionDefaults updated %d Character 5 ability progressions"), UpdatedProgressions);
#endif
}

void URoundAbilityRegistryData::PopulateCharacter6AbilityProgressionDefaults()
{
#if WITH_EDITOR
    Modify();

    URoundAbilityData* CreatedAbilityData = nullptr;
    URoundSkillProgressionData* CreatedProgressionData = nullptr;
    EnsureExtendedAbilityDefinition(
        this,
        {
            TEXT("StringSnare"),
            TEXT("DA_Ability_StringSnare"),
            TEXT("DA_SkillProgression_StringSnare"),
            TEXT("StringSnare"),
            TEXT("Spannt Faeden ueber das Feld und macht Ziele fuer Character 6 und die Puppe angreifbar."),
            ARoundStringSnareAbility::StaticClass(),
            20,
            220,
            3,
            0,
            ERoundDamageType::Magical,
            ERoundFieldState::Enemy,
            true,
            false,
            {
                {TEXT("DurationInitiative"), 80.0},
                {TEXT("MoveCostAddOnTarget"), 2.0}
            }
        },
        CreatedAbilityData,
        CreatedProgressionData);

    const TMap<FString, URoundSkillProgressionData*> ProgressionAssets = LoadAssetsByNormalizedName<URoundSkillProgressionData>();
    const TMap<FString, URoundAbilityData*> AbilityAssets = LoadAssetsByNormalizedName<URoundAbilityData>();

    struct FCharacter6AbilityDefaults
    {
        const TCHAR* SkillKey;
        const TCHAR* DisplayName;
        const TCHAR* Description;
        TArray<FRoundSkillpointDetails> Levels;
    };

    const TArray<FCharacter6AbilityDefaults> Defaults = {
        {
            TEXT("puppetswitch"),
            TEXT("PuppetSwitch"),
            TEXT("Positionswechsel, Rettung und das Oeffnen neuer Winkel fuer Character 6 und die Puppe."),
            {
                MakeAbilitySkillpoint(TEXT("Sofortige Ordnung"), TEXT("Character 6 erhaelt nach PuppetSwitch {InitiativeGainOnUse} Initiative."), {{TEXT("InitiativeGainOnUse"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Sichere Uebernahme"), TEXT("Character 6 erleidet bis zum Ende des Zuges nur {SelfIncomingDamageMultiplierAfterUse}x Schaden."), {{TEXT("SelfIncomingDamageMultiplierAfterUse"), 0.85}}),
                MakeAbilitySkillpoint(TEXT("Aggressiver Wechsel"), TEXT("Der naechste Treffer der Puppe in diesem Zug verursacht {NextPuppetHitDamageMultiplier}x Schaden."), {{TEXT("NextPuppetHitDamageMultiplier"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Rettungsgriff"), TEXT("Wird Character 6 aus der Nahkampfreichweite eines Gegners gezogen, verliert dieser {InitiativeLossOnLeaveMelee} Initiative."), {{TEXT("InitiativeLossOnLeaveMelee"), 12.0}}),
                MakeAbilitySkillpoint(TEXT("Raumtausch"), TEXT("Gegner neben dem neuen Feld der Puppe erhalten bis zum Ende ihres naechsten Zuges +{MoveCostAddNearPuppet} MoveCost."), {{TEXT("MoveCostAddNearPuppet"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Flankenoeffnung"), TEXT("Das dem neuen Feld der Puppe naechste Ziel verliert voruebergehend {DefenseIgnoreAddAgainstNearestEnemy} Defense gegen Character 6."), {{TEXT("DefenseIgnoreAddAgainstNearestEnemy"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Jagdwechsel"), TEXT("Character 6 verursacht gegen Ziele in der Naehe der Puppe bis zum Ende des Zuges {CharacterDamageMultiplierNearPuppet}x Schaden."), {{TEXT("CharacterDamageMultiplierNearPuppet"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Taktwechsel"), TEXT("Die naechste Aktion von Character 6 gibt {NextActionInitiativeRefund} Initiative zurueck."), {{TEXT("NextActionInitiativeRefund"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Kontrollwechsel"), TEXT("Gegner neben der Puppe koennen bis zum Ende ihres naechsten Zuges keine Bewegungsboni erhalten."), {{TEXT("BlockMovementBonusesNearPuppet"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Doppelposition"), TEXT("Ziele, die sowohl Character 6 als auch die Puppe bedrohen koennen, erleiden bis zum Ende des Zuges {BothThreatenDamageMultiplier}x Schaden von beiden."), {{TEXT("BothThreatenDamageMultiplier"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Fehltritt"), TEXT("Gegner neben Character 6 oder der Puppe verlieren nach PuppetSwitch {InitiativeLossNearNewPositions} Initiative."), {{TEXT("InitiativeLossNearNewPositions"), 8.0}}),
                MakeAbilitySkillpoint(TEXT("Kurzer Takt"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -30.0}})
            }
        },
        {
            TEXT("puppetslash"),
            TEXT("PuppetSlash"),
            TEXT("AoE-Kontrolle um die Puppe mit Setup fuer Character 6 und starken Duo-Fenstern."),
            {
                MakeAbilitySkillpoint(TEXT("Wirbelklingen"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 70.0}}),
                MakeAbilitySkillpoint(TEXT("Druckkreis"), TEXT("Getroffene Ziele verlieren {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Aufreissen"), TEXT("Character 6 und die Puppe ignorieren gegen getroffene Ziele voruebergehend {DefenseIgnoreDebuff} Defense."), {{TEXT("DefenseIgnoreDebuff"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Treibjagd"), TEXT("Character 6 verursacht gegen getroffene Ziele bis zum Ende des Zuges {CharacterDamageWindowMultiplier}x Schaden."), {{TEXT("CharacterDamageWindowMultiplier"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Schwere Beine"), TEXT("Getroffene Ziele erhalten bis zum Ende ihres naechsten Zuges +{MoveCostAddOnTarget} MoveCost."), {{TEXT("MoveCostAddOnTarget"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Festsetzen"), TEXT("Getroffene Ziele koennen bis zum Ende ihres naechsten Zuges keine Bewegungsboni erhalten."), {{TEXT("BlockMovementBonusesOnTarget"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Kreisschnitt"), TEXT("PuppetSlash trifft auch Ziele in Distanz 2. Diese erleiden {OuterRingDamageMultiplier}x Schaden."), {{TEXT("Range"), 1.0}, {TEXT("OuterRingDamageMultiplier"), 0.5}}),
                MakeAbilitySkillpoint(TEXT("Unruhe"), TEXT("Trifft PuppetSlash mindestens 2 Ziele, erhaelt Character 6 {InitiativeGainIfMultiHit} Initiative."), {{TEXT("InitiativeGainIfMultiHit"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Klingenfolge"), TEXT("Trifft PuppetSlash mindestens 2 Ziele, verursacht der naechste Treffer der Puppe {NextPuppetHitDamageMultiplierIfMultiHit}x Schaden."), {{TEXT("NextPuppetHitDamageMultiplierIfMultiHit"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Gemeinsame Jagd"), TEXT("Character 6 und die Puppe verursachen gegen getroffene Ziele bis zum Ende des Zuges {SharedHuntWindowMultiplier}x Schaden."), {{TEXT("SharedHuntWindowMultiplier"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Schutzkreis"), TEXT("Trifft PuppetSlash mindestens 2 Ziele, erleidet Character 6 bis zum Ende des naechsten Zuges nur {SelfIncomingDamageMultiplierIfMultiHit}x Schaden."), {{TEXT("SelfIncomingDamageMultiplierIfMultiHit"), 0.85}}),
                MakeAbilitySkillpoint(TEXT("Kurzer Takt"), TEXT("Verringert den Cooldown um {-Cooldown}, verringert aber den Schaden um {-Damage}."), {{TEXT("Cooldown"), -35.0}, {TEXT("Damage"), -40.0}})
            }
        },
        {
            TEXT("puppethit"),
            TEXT("PuppetHit"),
            TEXT("Praeziser Nahkampftreffer der Puppe, der Ziele oeffnen, umstellen oder als Engage dienen kann."),
            {
                MakeAbilitySkillpoint(TEXT("Praeziser Treffer"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 80.0}}),
                MakeAbilitySkillpoint(TEXT("Stoerschlag"), TEXT("Das Ziel verliert {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Durchstich"), TEXT("PuppetHit ignoriert {DefenseIgnore} Defense."), {{TEXT("DefenseIgnore"), 20.0}}),
                MakeAbilitySkillpoint(TEXT("Markierung"), TEXT("Character 6 verursacht gegen dieses Ziel bis zum Ende des Zuges {CharacterDamageWindowMultiplier}x Schaden."), {{TEXT("CharacterDamageWindowMultiplier"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Abschneiden"), TEXT("Das Ziel erhaelt bis zum Ende seines naechsten Zuges +{MoveCostAddOnTarget} MoveCost."), {{TEXT("MoveCostAddOnTarget"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Scharniertreffer"), TEXT("Das Ziel kann bis zum Ende seines naechsten Zuges keine Bewegungsboni erhalten."), {{TEXT("BlockMovementBonusesOnTarget"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Fernfuehrung"), TEXT("Erhoeht die Reichweite von PuppetHit um {Range}."), {{TEXT("Range"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Sollbruchstelle"), TEXT("Der naechste Treffer von Character 6 gegen dieses Ziel ignoriert {DefenseIgnoreForCharacterNextHit} Defense."), {{TEXT("DefenseIgnoreForCharacterNextHit"), 15.0}}),
                MakeAbilitySkillpoint(TEXT("Nadelarbeit"), TEXT("Bereits debuffte Ziele erleiden {DamageMultiplierVsDebuffed}x Schaden."), {{TEXT("DamageMultiplierVsDebuffed"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Hakenzug"), TEXT("Trifft PuppetHit ein Ziel, zieht sich die Puppe an ein freies Feld direkt neben das Ziel."), {{TEXT("PullPuppetAdjacentToTarget"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Rueckhand"), TEXT("Trifft PuppetHit ein Ziel, wird es um {KnockbackDistance} Feld von der Puppe weggestossen, falls moeglich."), {{TEXT("KnockbackDistance"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Sicherer Stich"), TEXT("Nach PuppetHit erleidet die Puppe bis zum Ende des naechsten Zuges nur {PuppetIncomingDamageMultiplierAfterUse}x Schaden."), {{TEXT("PuppetIncomingDamageMultiplierAfterUse"), 0.85}})
            }
        },
        {
            TEXT("puppetdefend"),
            TEXT("PuppetDefend"),
            TEXT("Schutz, Raumkontrolle und sichere Arbeitsfenster rund um die Puppe."),
            {
                MakeAbilitySkillpoint(TEXT("Starker Stand"), TEXT("Erhoeht die Schadensreduktion um {DamageReduction}."), {{TEXT("DamageReduction"), 0.15}}),
                MakeAbilitySkillpoint(TEXT("Langer Schutz"), TEXT("Erhoeht die Dauer um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 40.0}}),
                MakeAbilitySkillpoint(TEXT("Drohpraesenz"), TEXT("Gegner in der Naehe der Puppe verlieren {InitiativeLossOnAttackNearPuppet} Initiative, wenn sie Character 6 angreifen."), {{TEXT("InitiativeLossOnAttackNearPuppet"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Sicherer Anker"), TEXT("Solange PuppetDefend aktiv ist, erleidet Character 6 nur {SelfIncomingDamageMultiplierIfPuppetCloser}x Schaden, wenn die Puppe naeher am Angreifer steht."), {{TEXT("SelfIncomingDamageMultiplierIfPuppetCloser"), 0.85}}),
                MakeAbilitySkillpoint(TEXT("Kontrollraum"), TEXT("Gegner neben der Puppe koennen waehrend PuppetDefend keine Bewegungsboni erhalten."), {{TEXT("AuraBlockMovementBonusesNearPuppet"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Abfangen"), TEXT("Wird Character 6 angegriffen, verliert der Angreifer waehrend PuppetDefend {InitiativeLossOnAttack} Initiative."), {{TEXT("InitiativeLossOnAttack"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Taktischer Schirm"), TEXT("Character 6 ignoriert gegen Ziele in der Naehe der Puppe {AuraDefenseIgnoreAddAgainstTargetsNearPuppet} zusaetzliche Defense."), {{TEXT("AuraDefenseIgnoreAddAgainstTargetsNearPuppet"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Drahtwall"), TEXT("Gegner neben der Puppe erhalten waehrend PuppetDefend +{AuraMoveCostAddNearPuppet} MoveCost."), {{TEXT("AuraMoveCostAddNearPuppet"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Gegendruck"), TEXT("Wird die Puppe waehrend PuppetDefend getroffen, verursacht ihr naechster Treffer {NextPuppetHitDamageMultiplier}x Schaden."), {{TEXT("NextPuppetHitDamageMultiplier"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Geteilte Arbeit"), TEXT("Character 6 erhaelt waehrend PuppetDefend {InitiativeGainOnAttackInPuppetRange} Initiative, wenn er ein Ziel in Reichweite der Puppe angreift."), {{TEXT("InitiativeGainOnAttackInPuppetRange"), 10.0}}),
                MakeAbilitySkillpoint(TEXT("Sperrpunkt"), TEXT("Gegner in der Naehe der Puppe verursachen waehrend PuppetDefend nur {OutgoingDamageMultiplierEnemiesNearPuppet}x Schaden."), {{TEXT("OutgoingDamageMultiplierEnemiesNearPuppet"), 0.9}}),
                MakeAbilitySkillpoint(TEXT("Kurzer Takt"), TEXT("Verringert den Cooldown um {-Cooldown}, verringert aber die Dauer um {-DurationInitiative} Initiative."), {{TEXT("Cooldown"), -35.0}, {TEXT("DurationInitiative"), -40.0}})
            }
        },
        {
            TEXT("stringsnare"),
            TEXT("StringSnare"),
            TEXT("Spannt Faeden ueber das Feld und macht Ziele fuer Character 6 und die Puppe angreifbar."),
            {
                MakeAbilitySkillpoint(TEXT("Fester Zug"), TEXT("Erhoeht den MoveCost-Malus auf {MoveCostAddOnTarget}."), {{TEXT("MoveCostAddOnTarget"), 3.0}}),
                MakeAbilitySkillpoint(TEXT("Schneidfaden"), TEXT("Der Skill verursacht {Damage} Schaden."), {{TEXT("Damage"), 120.0}}),
                MakeAbilitySkillpoint(TEXT("Stoerfaden"), TEXT("Das Ziel verliert {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 12.0}}),
                MakeAbilitySkillpoint(TEXT("Knotenpunkt"), TEXT("Das Ziel kann bis zum Ende seines naechsten Zuges keine Bewegungsboni erhalten."), {{TEXT("BlockMovementBonusesOnTarget"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Zugfenster"), TEXT("Character 6 verursacht gegen das Ziel bis zum Ende des Zuges {IncomingDamageMultiplierFromCauser}x Schaden."), {{TEXT("IncomingDamageMultiplierFromCauser"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Puppenfenster"), TEXT("Die Puppe verursacht gegen das Ziel bis zum Ende des Zuges {IncomingDamageMultiplierFromPuppet}x Schaden."), {{TEXT("IncomingDamageMultiplierFromPuppet"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Drahtwand"), TEXT("Gegner direkt neben dem Ziel erhalten +{AoEMoveCostAddRadiusOne} MoveCost bis zum Ende ihres naechsten Zuges."), {{TEXT("AoEMoveCostAddRadiusOne"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Gespannte Leine"), TEXT("Erhoeht die Dauer um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 40.0}}),
                MakeAbilitySkillpoint(TEXT("Hakenspur"), TEXT("Der Skill verursacht {Damage} Schaden und erschwert Bewegung zusaetzlich um +{MoveCostAddOnTarget}."), {{TEXT("Damage"), 80.0}, {TEXT("MoveCostAddOnTarget"), 1.0}}),
                MakeAbilitySkillpoint(TEXT("Gemeinsamer Zug"), TEXT("Character 6 und die Puppe verursachen gegen das Ziel bis zum Ende des Zuges jeweils 1.15x Schaden."), {{TEXT("IncomingDamageMultiplierFromCauser"), 1.15}, {TEXT("IncomingDamageMultiplierFromPuppet"), 1.15}}),
                MakeAbilitySkillpoint(TEXT("Umspannen"), TEXT("Das Ziel kann bis zum Ende seines naechsten Zuges keine Bewegungsboni erhalten und erhaelt +{MoveCostAddOnTarget} MoveCost."), {{TEXT("BlockMovementBonusesOnTarget"), 1.0}, {TEXT("MoveCostAddOnTarget"), 2.0}}),
                MakeAbilitySkillpoint(TEXT("Kurzer Takt"), TEXT("Verringert den Cooldown um {-Cooldown}, verringert aber die Dauer um {-DurationInitiative} Initiative."), {{TEXT("Cooldown"), -35.0}, {TEXT("DurationInitiative"), -40.0}})
            }
        }
    };

    int32 UpdatedProgressions = 0;
    for (const FCharacter6AbilityDefaults& DefaultEntry : Defaults)
    {
        URoundSkillProgressionData* Progression = nullptr;
        if (const URoundSkillProgressionData* const* FoundProgression = ProgressionAssets.Find(DefaultEntry.SkillKey))
        {
            Progression = const_cast<URoundSkillProgressionData*>(*FoundProgression);
        }

        URoundAbilityData* AbilityData = nullptr;
        if (const URoundAbilityData* const* FoundAbility = AbilityAssets.Find(DefaultEntry.SkillKey))
        {
            AbilityData = const_cast<URoundAbilityData*>(*FoundAbility);
        }

        if (!Progression || !AbilityData)
        {
            UE_LOG(LogRoundAbilityRegistryImport, Warning, TEXT("PopulateCharacter6AbilityProgressionDefaults skipped %s - Progression=%s Ability=%s"),
                DefaultEntry.SkillKey,
                *GetNameSafe(Progression),
                *GetNameSafe(AbilityData));
            continue;
        }

        Progression->Modify();
        Progression->SkillId = AbilityData->AbilityId;
        Progression->DisplayName = FText::FromString(DefaultEntry.DisplayName);
        Progression->Description = FText::FromString(DefaultEntry.Description);
        Progression->Levels = DefaultEntry.Levels;
        Progression->MarkPackageDirty();

        AbilityData->Modify();
        AbilityData->SkillProgression = Progression;
        AbilityData->MarkPackageDirty();
        ++UpdatedProgressions;
    }

    MarkPackageDirty();
    UE_LOG(LogRoundAbilityRegistryImport, Log, TEXT("PopulateCharacter6AbilityProgressionDefaults updated %d Character 6 ability progressions"), UpdatedProgressions);
#endif
}

void URoundAbilityRegistryData::PopulateEngineerAbilityProgressionDefaults()
{
#if WITH_EDITOR
    struct FEngineerDefaults { const TCHAR* Asset; const TCHAR* Progression; const TCHAR* Name; const TCHAR* Description; TArray<FRoundSkillpointDetails> Levels; };
    const TArray<FEngineerDefaults> Defaults = {
        { TEXT("Turret"), TEXT("Turret"), TEXT("Turret"), TEXT("Ein dauerhaftes Kontrollgeraet, das automatisch den naechsten Gegner in Reichweite beschiesst."), {
            MakeAbilitySkillpoint(TEXT("Schweres Kaliber"), TEXT("Erhoeht den Schaden jedes Schusses um {Damage}."), {{TEXT("Damage"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Langlauf"), TEXT("Erhoeht die Schussreichweite um {TurretRange}."), {{TEXT("TurretRange"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Aufbau"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -50.0}}),
            MakeAbilitySkillpoint(TEXT("Panzerplatte"), TEXT("Erhoeht das Leben des Turrets um {Health}."), {{TEXT("Health"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Schnelllader"), TEXT("Der Turret feuert alle {-TriggerIntervalInitiative} Initiative frueher."), {{TEXT("TriggerIntervalInitiative"), -10.0}}),
            MakeAbilitySkillpoint(TEXT("Unterdrueckungsfeuer"), TEXT("Jeder Treffer laesst das Ziel {InitiativeLossOnHit} Initiative verlieren."), {{TEXT("InitiativeLossOnHit"), 8.0}}),
            MakeAbilitySkillpoint(TEXT("Panzerbrecher"), TEXT("Turret-Schuesse ignorieren {DefenseIgnore} Defense."), {{TEXT("DefenseIgnore"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Doppellauf"), TEXT("Nach dem Hauptziel trifft der Turret den naechsten anderen Gegner in Reichweite fuer {SecondaryTargetDamageMultiplier}x Schaden."), {{TEXT("SecondaryTargetDamageMultiplier"), 0.6}}),
            MakeAbilitySkillpoint(TEXT("Dauerbetrieb"), TEXT("Erhoeht die Lebensdauer des Turrets um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 80.0}}),
            MakeAbilitySkillpoint(TEXT("Exekutionsroutine"), TEXT("Ziele unter {LowHealthThreshold} Leben erleiden {LowHealthDamageMultiplier}x Schaden."), {{TEXT("LowHealthThreshold"), 0.35}, {TEXT("LowHealthDamageMultiplier"), 1.5}}),
            MakeAbilitySkillpoint(TEXT("Gatlingmotor"), TEXT("Der Turret feuert alle {-TriggerIntervalInitiative} Initiative frueher."), {{TEXT("TriggerIntervalInitiative"), -20.0}}),
            MakeAbilitySkillpoint(TEXT("Glaskanone"), TEXT("Erhoeht den Schaden um {Damage}, verringert aber das Leben um {-Health}."), {{TEXT("Damage"), 100.0}, {TEXT("Health"), -80.0}})
        }},
        { TEXT("Mine"), TEXT("Mine"), TEXT("Mine"), TEXT("Verdeckte Gebietskontrolle, die bei Betreten oder durch Overload als Flaechenschaden ausgeloest wird."), {
            MakeAbilitySkillpoint(TEXT("Mehr Sprengstoff"), TEXT("Erhoeht den Explosionsschaden um {Damage}."), {{TEXT("Damage"), 80.0}}),
            MakeAbilitySkillpoint(TEXT("Breite Ladung"), TEXT("Erhoeht den Explosionsradius um {AOERange}."), {{TEXT("AOERange"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Massenproduktion"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -40.0}}),
            MakeAbilitySkillpoint(TEXT("Schnellzuender"), TEXT("Verringert die Scharfstellzeit um {-ArmDelayInitiative} Initiative."), {{TEXT("ArmDelayInitiative"), -20.0}}),
            MakeAbilitySkillpoint(TEXT("Hohlladung"), TEXT("Die Explosion ignoriert {DefenseIgnore} Defense."), {{TEXT("DefenseIgnore"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Schockladung"), TEXT("Getroffene Gegner verlieren {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Haftladung"), TEXT("Getroffene Gegner koennen sich fuer {RootDurationInitiative} Initiative nicht bewegen."), {{TEXT("RootDurationInitiative"), 40.0}}),
            MakeAbilitySkillpoint(TEXT("Langlebiger Sensor"), TEXT("Erhoeht die Lebensdauer der Mine um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Fernverlegung"), TEXT("Erhoeht die Platzierungsreichweite um {Range}."), {{TEXT("Range"), 2.0}}),
            MakeAbilitySkillpoint(TEXT("Fernzuender"), TEXT("Eine durch Overload ausgeloeste Mine bleibt bestehen und kann spaeter erneut ausgeloest werden."), {{TEXT("PreserveOnManualTrigger"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Belagerungsladung"), TEXT("Erhoeht Schaden um {Damage} und Radius um {AOERange}."), {{TEXT("Damage"), 150.0}, {TEXT("AOERange"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Kontaktzuender"), TEXT("Die Mine ist sofort scharf, kostet aber {InitativeCost} zusaetzliche Initiative."), {{TEXT("ArmDelayInitiative"), -30.0}, {TEXT("InitativeCost"), 10.0}})
        }},
        { TEXT("ShieldEmitter"), TEXT("ShieldEmitter"), TEXT("Shield Emitter"), TEXT("Ein periodisches Schutzgeraet fuer Verbuendete in seinem Wirkungsbereich."), {
            MakeAbilitySkillpoint(TEXT("Starker Schild"), TEXT("Erhoeht jeden Schild um {ShieldAmount}."), {{TEXT("ShieldAmount"), 60.0}}),
            MakeAbilitySkillpoint(TEXT("Weites Feld"), TEXT("Erhoeht den Wirkungsradius um {AuraRange}."), {{TEXT("AuraRange"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Aufbau"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -60.0}}),
            MakeAbilitySkillpoint(TEXT("Nachladung"), TEXT("Erhoeht die Dauer jedes Schildes um {ShieldDurationInitiative} Initiative."), {{TEXT("ShieldDurationInitiative"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Medizinmodul"), TEXT("Jeder Impuls heilt betroffene Verbuendete um {HealAmount}."), {{TEXT("HealAmount"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Verstaerkter Kern"), TEXT("Erhoeht das Leben des Emitters um {Health}."), {{TEXT("Health"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Energieschub"), TEXT("Jeder Impuls gewaehrt betroffenen Verbuendeten {InitiativeGain} Initiative."), {{TEXT("InitiativeGain"), 8.0}}),
            MakeAbilitySkillpoint(TEXT("Reinigungsfeld"), TEXT("Jeder Impuls entfernt einen entfernbaren Debuff von jedem betroffenen Verbuendeten."), {{TEXT("CleanseOneDebuff"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Fokussierter Schild"), TEXT("Betrifft ein Impuls nur einen Verbuendeten, erhaelt dieser {SoloShieldMultiplier}x Schild."), {{TEXT("SoloShieldMultiplier"), 1.75}}),
            MakeAbilitySkillpoint(TEXT("Impulsmotor"), TEXT("Der Emitter aktiviert sich alle {-TriggerIntervalInitiative} Initiative frueher."), {{TEXT("TriggerIntervalInitiative"), -20.0}}),
            MakeAbilitySkillpoint(TEXT("Festungsfeld"), TEXT("Erhoeht jeden Schild um {ShieldAmount}."), {{TEXT("ShieldAmount"), 120.0}}),
            MakeAbilitySkillpoint(TEXT("Dauerstellung"), TEXT("Erhoeht die Lebensdauer des Emitters um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 120.0}})
        }},
        { TEXT("SpotterBeacon"), TEXT("SpotterBeacon"), TEXT("Spotter Beacon"), TEXT("Deckt ein Gebiet auf und macht Gegner fuer das gesamte Team verwundbarer."), {
            MakeAbilitySkillpoint(TEXT("Breiter Scan"), TEXT("Erhoeht den Scanradius um {AuraRange}."), {{TEXT("AuraRange"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Praezise Daten"), TEXT("Erhoeht den Schadensmultiplikator markierter Ziele um {DamageTakenMultiplier}."), {{TEXT("DamageTakenMultiplier"), 0.1}}),
            MakeAbilitySkillpoint(TEXT("Schneller Aufbau"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -50.0}}),
            MakeAbilitySkillpoint(TEXT("Langzeitmarkierung"), TEXT("Markierungen halten {DebuffDurationInitiative} Initiative laenger."), {{TEXT("DebuffDurationInitiative"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Zieldruck"), TEXT("Markierte Gegner erhalten +{MoveCostAdd} MoveCost."), {{TEXT("MoveCostAdd"), 2.0}}),
            MakeAbilitySkillpoint(TEXT("Stoersignal"), TEXT("Jeder Scan laesst markierte Gegner {InitiativeLoss} Initiative verlieren."), {{TEXT("InitiativeLoss"), 10.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Scan"), TEXT("Der Beacon scannt alle {-TriggerIntervalInitiative} Initiative frueher."), {{TEXT("TriggerIntervalInitiative"), -10.0}}),
            MakeAbilitySkillpoint(TEXT("Gepanzerter Beacon"), TEXT("Erhoeht sein Leben um {Health}."), {{TEXT("Health"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Vorposten"), TEXT("Erhoeht die Platzierungsreichweite um {Range}."), {{TEXT("Range"), 2.0}}),
            MakeAbilitySkillpoint(TEXT("Stoerfrequenz"), TEXT("Markierte Gegner verursachen fuer die Markierungsdauer nur {OutgoingDamageMultiplier}x Schaden."), {{TEXT("OutgoingDamageMultiplier"), 0.85}}),
            MakeAbilitySkillpoint(TEXT("Zielcomputer"), TEXT("Erhoeht den Schadensmultiplikator markierter Ziele um {DamageTakenMultiplier}."), {{TEXT("DamageTakenMultiplier"), 0.2}}),
            MakeAbilitySkillpoint(TEXT("Ueberwachungsgitter"), TEXT("Erhoeht Scanradius um {AuraRange} und Lebensdauer um {DurationInitiative} Initiative."), {{TEXT("AuraRange"), 2.0}, {TEXT("DurationInitiative"), 80.0}})
        }},
        { TEXT("Overload"), TEXT("Overload"), TEXT("Overload"), TEXT("Loest alle eigenen Geraete in Reichweite sofort aus und wird dadurch zum zentralen Kombowerkzeug des Engineers."), {
            MakeAbilitySkillpoint(TEXT("Groesseres Netzwerk"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 2.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Neustart"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -60.0}}),
            MakeAbilitySkillpoint(TEXT("Leichte Steuerung"), TEXT("Verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("InitativeCost"), -8.0}}),
            MakeAbilitySkillpoint(TEXT("Notfallreparatur"), TEXT("Overload repariert jedes betroffene Geraet vor der Aktivierung um {RepairDevices}."), {{TEXT("RepairDevices"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Laufzeitinjektion"), TEXT("Betroffene Geraete erhalten {LifetimeExtensionInitiative} Initiative Lebensdauer."), {{TEXT("LifetimeExtensionInitiative"), 80.0}}),
            MakeAbilitySkillpoint(TEXT("Energierueckfluss"), TEXT("Character 7 erhaelt pro erfolgreich aktiviertem Geraet {InitiativeGainPerDevice} Initiative."), {{TEXT("InitiativeGainPerDevice"), 5.0}}),
            MakeAbilitySkillpoint(TEXT("Rueckkopplung"), TEXT("Der aktuelle Cooldown sinkt pro erfolgreich aktiviertem Geraet um {CooldownReductionPerDevice} Initiative."), {{TEXT("CooldownReductionPerDevice"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Kaskade"), TEXT("Jedes Geraet, das nach der ersten Aktivierung noch besteht, wird sofort ein zweites Mal aktiviert."), {{TEXT("DoubleTrigger"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Opferschaltung"), TEXT("Betroffene Geraete werden nach der Aktivierung zerstoert und verursachen an benachbarten Gegnern {DestroyExplosionDamage} Blitzschaden."), {{TEXT("DestroyAfterOverload"), 1.0}, {TEXT("DestroyExplosionDamage"), 180.0}}),
            MakeAbilitySkillpoint(TEXT("Systemneustart"), TEXT("Overload repariert jedes betroffene Geraet vollstaendig und verlaengert seine Lebensdauer um {LifetimeExtensionInitiative} Initiative."), {{TEXT("RepairDevices"), 9999.0}, {TEXT("LifetimeExtensionInitiative"), 120.0}}),
            MakeAbilitySkillpoint(TEXT("Netzwerksturm"), TEXT("Jedes nach der ersten Aktivierung noch bestehende Geraet wird ein zweites Mal aktiviert; Overload kostet {InitativeCost} mehr Initiative."), {{TEXT("DoubleTrigger"), 1.0}, {TEXT("InitativeCost"), 10.0}}),
            MakeAbilitySkillpoint(TEXT("Kernschmelze"), TEXT("Zerstoert alle betroffenen Geraete nach der Aktivierung. Jedes verursacht an benachbarten Gegnern {DestroyExplosionDamage} Blitzschaden; der Cooldown sinkt um {-Cooldown}."), {{TEXT("DestroyAfterOverload"), 1.0}, {TEXT("DestroyExplosionDamage"), 300.0}, {TEXT("Cooldown"), -100.0}})
        }}
    };

    for (const FEngineerDefaults& Entry : Defaults)
    {
        URoundAbilityData* Ability = LoadObject<URoundAbilityData>(nullptr, *FString::Printf(TEXT("/Game/TopDown/Data/Abilities/DA_Ability_%s.DA_Ability_%s"), Entry.Asset, Entry.Asset));
        URoundSkillProgressionData* Progression = LoadObject<URoundSkillProgressionData>(nullptr, *FString::Printf(TEXT("/Game/TopDown/Data/SkillProgression/DA_SkillProgression_%s.DA_SkillProgression_%s"), Entry.Progression, Entry.Progression));
        if (!Ability || !Progression) continue;
        Progression->Modify(); Progression->SkillId = Ability->AbilityId; Progression->DisplayName = FText::FromString(Entry.Name);
        Progression->Description = FText::FromString(Entry.Description); Progression->Levels = Entry.Levels; Progression->MarkPackageDirty();
        Ability->Modify(); Ability->SkillProgression = Progression; Ability->MarkPackageDirty();
    }
    MarkPackageDirty();
#endif
}

void URoundAbilityRegistryData::PopulateChronomancerAbilityProgressionDefaults()
{
#if WITH_EDITOR
    struct FDefaults { const TCHAR* Asset; const TCHAR* Name; const TCHAR* Description; TArray<FRoundSkillpointDetails> Levels; };
    const TArray<FDefaults> Defaults = {
        { TEXT("Accelerate"), TEXT("Accelerate"), TEXT("Beschleunigt die Cooldowns eines Verbuendeten und kann zu einem breiten Tempo- oder Selbstkombowerkzeug ausgebaut werden."), {
            MakeAbilitySkillpoint(TEXT("Schneller Lauf"), TEXT("Reduziert jeden aktuellen Cooldown des Ziels um weitere {CooldownReduction} Initiative."), {{TEXT("CooldownReduction"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Weite Sekunde"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Leichter Impuls"), TEXT("Verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("InitativeCost"), -5.0}}),
            MakeAbilitySkillpoint(TEXT("Vorsprung"), TEXT("Das Ziel erhaelt zusaetzlich {TargetInitiativeGain} Initiative."), {{TEXT("TargetInitiativeGain"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Prioritaet"), TEXT("Der Skill mit dem hoechsten aktuellen Cooldown wird um weitere {ExtraReductionToHighestCooldown} Initiative reduziert."), {{TEXT("ExtraReductionToHighestCooldown"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Eigenzeit"), TEXT("Wird ein anderer Verbuendeter beschleunigt, werden auch die Cooldowns des Chronomancers reduziert."), {{TEXT("AccelerateSelf"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Zeitwelle"), TEXT("Accelerate wirkt zusaetzlich auf Verbuendete direkt neben dem Ziel."), {{TEXT("AffectAdjacentAllies"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Paradoxe Effizienz"), TEXT("Verringert die Fragmentkosten um {-FragmentCost}."), {{TEXT("FragmentCost"), -1.0}}),
            MakeAbilitySkillpoint(TEXT("Kurzer Zyklus"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -55.0}}),
            MakeAbilitySkillpoint(TEXT("Fixpunkt"), TEXT("Der Skill mit dem hoechsten aktuellen Cooldown wird um weitere {ExtraReductionToHighestCooldown} Initiative reduziert."), {{TEXT("ExtraReductionToHighestCooldown"), 70.0}}),
            MakeAbilitySkillpoint(TEXT("Synchronisation"), TEXT("Accelerate wirkt auch auf den Chronomancer und gibt dem gewaehlten Ziel {TargetInitiativeGain} Initiative."), {{TEXT("AccelerateSelf"), 1.0}, {TEXT("TargetInitiativeGain"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Gemeinsamer Takt"), TEXT("Accelerate wirkt auf alle Verbuendeten direkt neben dem Ziel; der Cooldown steigt um {Cooldown}."), {{TEXT("AffectAdjacentAllies"), 1.0}, {TEXT("Cooldown"), 45.0}})
        }},
        { TEXT("BorrowedTime"), TEXT("Borrowed Time"), TEXT("Verschiebt einen Teil eingehenden Schadens und laesst daraus spaeter Heilung, Tempo oder echte Schadensvermeidung entstehen."), {
            MakeAbilitySkillpoint(TEXT("Mehr geliehene Zeit"), TEXT("Verschiebt weitere {DeferredDamageFraction} des eingehenden Schadens."), {{TEXT("DeferredDamageFraction"), 0.10}}),
            MakeAbilitySkillpoint(TEXT("Langes Darlehen"), TEXT("Erhoeht die Dauer um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Fernleihe"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Atempause"), TEXT("Wenn der aufgeschobene Schaden faellig wird und das Ziel ueberlebt, heilt es sich um {HealOnResolve}."), {{TEXT("HealOnResolve"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Aufholjagd"), TEXT("Wenn der aufgeschobene Schaden faellig wird und das Ziel ueberlebt, erhaelt es {InitiativeGainOnResolve} Initiative."), {{TEXT("InitiativeGainOnResolve"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Fruehe Rueckzahlung"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -55.0}}),
            MakeAbilitySkillpoint(TEXT("Schuldenerlass"), TEXT("Beim Faelligwerden werden {DeferredDamageForgivenFraction} des gespeicherten Schadens erlassen."), {{TEXT("DeferredDamageForgivenFraction"), 0.25}}),
            MakeAbilitySkillpoint(TEXT("Zeitkredit"), TEXT("Verringert die Fragmentkosten um {-FragmentCost}."), {{TEXT("FragmentCost"), -1.0}}),
            MakeAbilitySkillpoint(TEXT("Lange Schonfrist"), TEXT("Erhoeht die Dauer um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 65.0}}),
            MakeAbilitySkillpoint(TEXT("Paradoxe Tilgung"), TEXT("Verschiebt weitere 15% Schaden und erlaesst beim Faelligwerden 20% des gespeicherten Schadens."), {{TEXT("DeferredDamageFraction"), 0.15}, {TEXT("DeferredDamageForgivenFraction"), 0.20}}),
            MakeAbilitySkillpoint(TEXT("Zweite Chance"), TEXT("Beim Faelligwerden heilt das ueberlebende Ziel um {HealOnResolve} und erhaelt {InitiativeGainOnResolve} Initiative."), {{TEXT("HealOnResolve"), 240.0}, {TEXT("InitiativeGainOnResolve"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Dauerstundung"), TEXT("Erhoeht die Dauer stark, aber auch den Cooldown."), {{TEXT("DurationInitiative"), 110.0}, {TEXT("Cooldown"), 50.0}})
        }},
        { TEXT("Stasis"), TEXT("Stasis"), TEXT("Nimmt einen Verbuendeten vollstaendig aus dem Kampf; der Tree bestimmt Dauer, Verfuegbarkeit und den Moment seiner Rueckkehr."), {
            MakeAbilitySkillpoint(TEXT("Stabile Blase"), TEXT("Erhoeht die Dauer um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Fernstasis"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Bereitschaft"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -55.0}}),
            MakeAbilitySkillpoint(TEXT("Regeneration"), TEXT("Beim Ende der Stasis heilt das Ziel {HealOnRelease} Leben."), {{TEXT("HealOnRelease"), 130.0}}),
            MakeAbilitySkillpoint(TEXT("Zeitsprung"), TEXT("Beim Ende der Stasis erhaelt das Ziel {InitiativeGainOnRelease} Initiative."), {{TEXT("InitiativeGainOnRelease"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Sparsames Siegel"), TEXT("Verringert die Fragmentkosten um {-FragmentCost}."), {{TEXT("FragmentCost"), -1.0}}),
            MakeAbilitySkillpoint(TEXT("Notfallstasis"), TEXT("Die Stasis endet frueher und der Skill ist schneller erneut bereit."), {{TEXT("DurationInitiative"), -30.0}, {TEXT("Cooldown"), -85.0}}),
            MakeAbilitySkillpoint(TEXT("Heilschlaf"), TEXT("Beim Ende der Stasis heilt das Ziel {HealOnRelease} Leben."), {{TEXT("HealOnRelease"), 260.0}}),
            MakeAbilitySkillpoint(TEXT("Sicherer Abstand"), TEXT("Erhoeht die Reichweite um {Range} und verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("Range"), 2.0}, {TEXT("InitativeCost"), -5.0}}),
            MakeAbilitySkillpoint(TEXT("Angehaltene Ewigkeit"), TEXT("Erhoeht die Dauer um {DurationInitiative} Initiative und verringert die Fragmentkosten um {-FragmentCost}."), {{TEXT("DurationInitiative"), 70.0}, {TEXT("FragmentCost"), -1.0}}),
            MakeAbilitySkillpoint(TEXT("Perfekte Rueckkehr"), TEXT("Beim Ende heilt das Ziel {HealOnRelease} und erhaelt {InitiativeGainOnRelease} Initiative."), {{TEXT("HealOnRelease"), 300.0}, {TEXT("InitiativeGainOnRelease"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Panikknopf"), TEXT("Stasis hat einen stark reduzierten Cooldown, kostet aber mehr Initiative."), {{TEXT("Cooldown"), -120.0}, {TEXT("InitativeCost"), 10.0}})
        }},
        { TEXT("TemporalAnchor"), TEXT("Temporal Anchor"), TEXT("Speichert die Position eines Verbuendeten und belohnt riskante Vorstoesse durch eine kontrollierte Rueckkehr."), {
            MakeAbilitySkillpoint(TEXT("Langer Anker"), TEXT("Erhoeht die Dauer bis zur Rueckkehr um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Fernanker"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Anker"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -55.0}}),
            MakeAbilitySkillpoint(TEXT("Gesicherte Rueckkehr"), TEXT("Bei der Rueckkehr heilt das Ziel {HealOnReturn} Leben."), {{TEXT("HealOnReturn"), 130.0}}),
            MakeAbilitySkillpoint(TEXT("Schwung erhalten"), TEXT("Bei der Rueckkehr erhaelt das Ziel {InitiativeGainOnReturn} Initiative."), {{TEXT("InitiativeGainOnReturn"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Zeitdruck"), TEXT("Gegner neben dem Rueckkehrfeld verlieren {NearbyEnemyInitiativeLossOnReturn} Initiative."), {{TEXT("NearbyEnemyInitiativeLossOnReturn"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Kurze Leine"), TEXT("Das Ziel kehrt deutlich frueher zurueck; der Cooldown sinkt."), {{TEXT("DurationInitiative"), -45.0}, {TEXT("Cooldown"), -45.0}}),
            MakeAbilitySkillpoint(TEXT("Freier Anker"), TEXT("Verringert die Fragmentkosten um {-FragmentCost}."), {{TEXT("FragmentCost"), -1.0}}),
            MakeAbilitySkillpoint(TEXT("Expeditionsfenster"), TEXT("Erhoeht die Dauer und Reichweite."), {{TEXT("DurationInitiative"), 70.0}, {TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Rueckholimpuls"), TEXT("Bei der Rueckkehr heilt das Ziel und erhaelt Initiative."), {{TEXT("HealOnReturn"), 240.0}, {TEXT("InitiativeGainOnReturn"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Zeitbombe"), TEXT("Gegner neben dem Rueckkehrfeld verlieren {NearbyEnemyInitiativeLossOnReturn} Initiative."), {{TEXT("NearbyEnemyInitiativeLossOnReturn"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Fixierter Moment"), TEXT("Erhoeht die Dauer stark und verringert die Fragmentkosten, erhoeht aber den Cooldown."), {{TEXT("DurationInitiative"), 110.0}, {TEXT("FragmentCost"), -1.0}, {TEXT("Cooldown"), 45.0}})
        }},
        { TEXT("TimeSiphon"), TEXT("Time Siphon"), TEXT("Stiehlt einem Gegner Initiative und erzeugt die Fragmente fuer die uebrigen Zeitzauber."), {
            MakeAbilitySkillpoint(TEXT("Staerkerer Entzug"), TEXT("Entzieht zusaetzlich {InitiativeDrain} Initiative."), {{TEXT("InitiativeDrain"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Weite Verbindung"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Zugriff"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -35.0}}),
            MakeAbilitySkillpoint(TEXT("Fragmentierung"), TEXT("Erzeugt {FragmentsGained} zusaetzliches Fragment."), {{TEXT("FragmentsGained"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Gestohlener Moment"), TEXT("Der Chronomancer erhaelt {SelfInitiativeGain} Initiative."), {{TEXT("SelfInitiativeGain"), 12.0}}),
            MakeAbilitySkillpoint(TEXT("Ueberladung"), TEXT("Bei maximalen Fragmenten entzieht Time Siphon zusaetzlich {BonusDrainAtFullFragments} Initiative."), {{TEXT("BonusDrainAtFullFragments"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Zeitriss"), TEXT("Gegner direkt neben dem Ziel verlieren ebenfalls {AdjacentInitiativeDrain} Initiative."), {{TEXT("AdjacentInitiativeDrain"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Mueheloser Raub"), TEXT("Verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("InitativeCost"), -5.0}}),
            MakeAbilitySkillpoint(TEXT("Kurzer Kreislauf"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -65.0}}),
            MakeAbilitySkillpoint(TEXT("Paradoxernte"), TEXT("Erzeugt {FragmentsGained} zusaetzliche Fragmente, hat aber einen hoeheren Cooldown."), {{TEXT("FragmentsGained"), 2.0}, {TEXT("Cooldown"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Zeitdieb"), TEXT("Entzieht zusaetzlich Initiative und gibt dem Chronomancer Initiative."), {{TEXT("InitiativeDrain"), 25.0}, {TEXT("SelfInitiativeGain"), 22.0}}),
            MakeAbilitySkillpoint(TEXT("Kollabierende Sekunde"), TEXT("Entzieht dem Ziel und benachbarten Gegnern mehr Initiative."), {{TEXT("InitiativeDrain"), 20.0}, {TEXT("AdjacentInitiativeDrain"), 30.0}})
        }}
    };
    for (const FDefaults& Entry : Defaults)
    {
        const FString AbilityPath = FString::Printf(TEXT("/Game/TopDown/Data/Abilities/DA_Ability_%s.DA_Ability_%s"), Entry.Asset, Entry.Asset);
        const FString ProgressionPath = FString::Printf(TEXT("/Game/TopDown/Data/SkillProgression/DA_SkillProgression_%s.DA_SkillProgression_%s"), Entry.Asset, Entry.Asset);
        URoundAbilityData* Ability = LoadObject<URoundAbilityData>(nullptr, *AbilityPath);
        URoundSkillProgressionData* Progression = LoadObject<URoundSkillProgressionData>(nullptr, *ProgressionPath);
        if (!Ability || !Progression) continue;
        Progression->Modify(); Progression->SkillId = Ability->AbilityId; Progression->DisplayName = FText::FromString(Entry.Name); Progression->Description = FText::FromString(Entry.Description); Progression->Levels = Entry.Levels; Progression->MarkPackageDirty();
        Ability->Modify(); Ability->SkillProgression = Progression;
        if (FString(Entry.Asset).Equals(TEXT("TimeSiphon"))) Ability->FieldState = ERoundFieldState::Enemy;
        Ability->MarkPackageDirty();
    }
    MarkPackageDirty();
#endif
}

void URoundAbilityRegistryData::PopulateDruidAbilityProgressionDefaults()
{
#if WITH_EDITOR
    struct FDefaults { const TCHAR* Asset; const TCHAR* Name; const TCHAR* Description; TArray<FRoundSkillpointDetails> Levels; };
    const TArray<FDefaults> Defaults = {
        { TEXT("ThornPatch"), TEXT("Thorn Patch"), TEXT("Eroeffnet das Natur-Netz offensiv: Das Zielfeld wird zu Dornen und ein Gegner darauf erleidet sofort Schaden."), {
            MakeAbilitySkillpoint(TEXT("Scharfe Dornen"), TEXT("Erhoeht den Sofortschaden um {Damage}."), {{TEXT("Damage"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Wurf"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Schnelles Wachstum"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -35.0}}),
            MakeAbilitySkillpoint(TEXT("Festhaltende Ranken"), TEXT("Ein getroffener Gegner verliert {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Verbindungswuchs"), TEXT("Grenzt das Zielfeld bereits an Naturboden, erhaelt der Druide {InitiativeRefundIfConnected} Initiative zurueck."), {{TEXT("InitiativeRefundIfConnected"), 12.0}}),
            MakeAbilitySkillpoint(TEXT("Harte Spitzen"), TEXT("Der Sofortschaden ignoriert {DefenseIgnore} Verteidigung."), {{TEXT("DefenseIgnore"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Wucherung"), TEXT("Auch alle direkt angrenzenden Felder werden zu Dornen; Gegner darauf erleiden ebenfalls den Sofortschaden."), {{TEXT("SpreadToAdjacentFields"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Blitzkeim"), TEXT("Verringert Initiativekosten und Cooldown."), {{TEXT("InitativeCost"), -6.0}, {TEXT("Cooldown"), -30.0}}),
            MakeAbilitySkillpoint(TEXT("Blutdornen"), TEXT("Erhoeht Schaden und Initiativeverlust."), {{TEXT("Damage"), 70.0}, {TEXT("InitiativeLossOnHit"), 10.0}}),
            MakeAbilitySkillpoint(TEXT("Dornenfeld"), TEXT("Auch angrenzende Felder werden zu Dornen; der Cooldown steigt um {Cooldown}."), {{TEXT("SpreadToAdjacentFields"), 1.0}, {TEXT("Cooldown"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Belagerungsranken"), TEXT("Erhoeht Schaden und Verteidigungsignorierung."), {{TEXT("Damage"), 110.0}, {TEXT("DefenseIgnore"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Endloses Gestruepp"), TEXT("Grenzt das Feld an Naturboden, wird viel Initiative erstattet und die Reichweite steigt."), {{TEXT("InitiativeRefundIfConnected"), 22.0}, {TEXT("Range"), 2.0}})
        }},
        { TEXT("Grove"), TEXT("Grove"), TEXT("Legt einen schützenden Hain an, der Fernschaden mindert und als heilender Knoten fuer Wild Bloom dient."), {
            MakeAbilitySkillpoint(TEXT("Pflegender Hain"), TEXT("Ein Verbuendeter auf dem Zielfeld wird sofort um {HealOnPlacement} geheilt."), {{TEXT("HealOnPlacement"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Samenflug"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Austrieb"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -35.0}}),
            MakeAbilitySkillpoint(TEXT("Belebender Boden"), TEXT("Ein Verbuendeter auf dem Zielfeld erhaelt {InitiativeGainOnPlacement} Initiative."), {{TEXT("InitiativeGainOnPlacement"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Reinigende Kraeuter"), TEXT("Entfernt einen entfernbaren Debuff vom Verbuendeten auf dem Zielfeld."), {{TEXT("CleanseOneDebuff"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Leichter Samen"), TEXT("Verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("InitativeCost"), -6.0}}),
            MakeAbilitySkillpoint(TEXT("Hainkreis"), TEXT("Das Zielfeld und alle direkt angrenzenden Felder werden zu Hainen."), {{TEXT("SpreadToAdjacentFields"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Rettungsort"), TEXT("Ein Verbuendeter auf dem Zielfeld wird geheilt und erhaelt Initiative."), {{TEXT("HealOnPlacement"), 170.0}, {TEXT("InitiativeGainOnPlacement"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Dauerpflege"), TEXT("Verringert den Cooldown und heilt einen Verbuendeten auf dem Zielfeld."), {{TEXT("Cooldown"), -55.0}, {TEXT("HealOnPlacement"), 90.0}}),
            MakeAbilitySkillpoint(TEXT("Urwald"), TEXT("Das Zielfeld und angrenzende Felder werden zu Hainen; der Cooldown steigt um {Cooldown}."), {{TEXT("SpreadToAdjacentFields"), 1.0}, {TEXT("Cooldown"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Lebensquelle"), TEXT("Heilt den Verbuendeten auf dem Zielfeld stark und entfernt einen Debuff."), {{TEXT("HealOnPlacement"), 280.0}, {TEXT("CleanseOneDebuff"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Gruene Zuflucht"), TEXT("Erhoeht die Reichweite und gibt dem Verbuendeten auf dem Zielfeld Initiative."), {{TEXT("Range"), 2.0}, {TEXT("InitiativeGainOnPlacement"), 25.0}})
        }},
        { TEXT("Rootsnare"), TEXT("Rootsnare"), TEXT("Bindet einen Gegner an seinem Feld; Naturboden verlaengert die Kontrolle und schafft ein klares Fokusziel."), {
            MakeAbilitySkillpoint(TEXT("Feste Wurzeln"), TEXT("Erhoeht die Basisdauer um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Rankenpeitsche"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Weite Ranke"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Naturgriff"), TEXT("Auf Naturboden haelt Rootsnare weitere {BonusDurationOnNatureInitiative} Initiative."), {{TEXT("BonusDurationOnNatureInitiative"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Offene Deckung"), TEXT("Solange das Ziel verwurzelt ist, erleidet es vom Druiden 15% mehr Schaden."), {{TEXT("IncomingDamageMultiplierFromCauserAdd"), 0.15}}),
            MakeAbilitySkillpoint(TEXT("Schneller Knoten"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -50.0}}),
            MakeAbilitySkillpoint(TEXT("Nachziehende Wurzeln"), TEXT("Wenn die Wurzel endet, verliert das Ziel {InitiativeLossOnExpire} Initiative."), {{TEXT("InitiativeLossOnExpire"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Leichter Griff"), TEXT("Verringert die Initiativekosten um {-InitativeCost} und erhoeht die Reichweite um {Range}."), {{TEXT("InitativeCost"), -7.0}, {TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Reissende Ranken"), TEXT("Erhoeht Schaden und Verteidigungsignorierung."), {{TEXT("Damage"), 75.0}, {TEXT("DefenseIgnore"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Uralte Fessel"), TEXT("Erhoeht Basisdauer und Naturbonus deutlich."), {{TEXT("DurationInitiative"), 35.0}, {TEXT("BonusDurationOnNatureInitiative"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Jagdmarke"), TEXT("Solange das Ziel verwurzelt ist, erleidet es vom Druiden 25% mehr Schaden."), {{TEXT("IncomingDamageMultiplierFromCauserAdd"), 0.25}}),
            MakeAbilitySkillpoint(TEXT("Zeitwurzel"), TEXT("Beim Ende verliert das Ziel Initiative; der Skill ist schneller erneut bereit."), {{TEXT("InitiativeLossOnExpire"), 30.0}, {TEXT("Cooldown"), -65.0}})
        }},
        { TEXT("WildBloom"), TEXT("Wild Bloom"), TEXT("Loest alle verbundenen Naturfelder gleichzeitig aus: Dornen schaden Gegnern, Haine heilen und Wald gibt Verbuendeten Initiative."), {
            MakeAbilitySkillpoint(TEXT("Giftige Bluete"), TEXT("Erhoeht den Schaden ausgelöster Dornen um {ThornDamage}."), {{TEXT("ThornDamage"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Heilende Bluete"), TEXT("Erhoeht die Heilung ausgelöster Haine um {GroveHeal}."), {{TEXT("GroveHeal"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Rascher Fruehling"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -55.0}}),
            MakeAbilitySkillpoint(TEXT("Waldatem"), TEXT("Erhoeht den Initiativegewinn ausgelöster Waldfelder um {ForestInitiativeGain}."), {{TEXT("ForestInitiativeGain"), 10.0}}),
            MakeAbilitySkillpoint(TEXT("Rueckkehr der Kraft"), TEXT("Pro tatsaechlich ausgeloestem Feld heilt sich der Druide um {SelfHealPerTriggeredField}."), {{TEXT("SelfHealPerTriggeredField"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Kreislauf"), TEXT("Pro tatsaechlich ausgeloestem Feld sinkt der aktuelle Cooldown um {CooldownReductionPerTriggeredField}, maximal 100 Initiative."), {{TEXT("CooldownReductionPerTriggeredField"), 12.0}}),
            MakeAbilitySkillpoint(TEXT("Ueppige Bluete"), TEXT("Verstaerkt Dornen, Haine und Waldfelder gleichzeitig."), {{TEXT("ThornDamage"), 35.0}, {TEXT("GroveHeal"), 35.0}, {TEXT("ForestInitiativeGain"), 7.0}}),
            MakeAbilitySkillpoint(TEXT("Leichte Ernte"), TEXT("Verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("InitativeCost"), -12.0}}),
            MakeAbilitySkillpoint(TEXT("Einmalige Ernte"), TEXT("Wild Bloom ist deutlich staerker, aber alle verbundenen Naturfelder werden danach zu normalem Boden."), {{TEXT("ThornDamage"), 90.0}, {TEXT("GroveHeal"), 90.0}, {TEXT("ForestInitiativeGain"), 15.0}, {TEXT("WitherAfterBloom"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Ueberwucherung"), TEXT("Verstaerkt alle drei Feldtypen stark."), {{TEXT("ThornDamage"), 80.0}, {TEXT("GroveHeal"), 80.0}, {TEXT("ForestInitiativeGain"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Ewiger Kreislauf"), TEXT("Pro ausgeloestem Feld sinkt der Cooldown und der Druide heilt sich."), {{TEXT("CooldownReductionPerTriggeredField"), 18.0}, {TEXT("SelfHealPerTriggeredField"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Naturkatastrophe"), TEXT("Dornen verursachen massiv mehr Schaden; Hainheilung und Waldinitiative bleiben unveraendert."), {{TEXT("ThornDamage"), 170.0}})
        }},
        { TEXT("BearForm"), TEXT("Bear Form"), TEXT("Tauscht das Natur-Kit voruebergehend gegen eine separate robuste Nahkampfform mit eigener Lebensleiste."), {
            MakeAbilitySkillpoint(TEXT("Massiger Baer"), TEXT("Erhoeht die maximale Baerengesundheit um {BearMaxHealth}."), {{TEXT("BearMaxHealth"), 250.0}}),
            MakeAbilitySkillpoint(TEXT("Kraeftige Pranke"), TEXT("Erhoeht den Angriff des Baeren um {BearAttack}."), {{TEXT("BearAttack"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Wandel"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -90.0}}),
            MakeAbilitySkillpoint(TEXT("Dicke Haut"), TEXT("Erhoeht die Verteidigung des Baeren um {BearDefense}."), {{TEXT("BearDefense"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Stuermischer Eintritt"), TEXT("Beim Wechsel in die Baerenform erhaelt der Baer {InitiativeGainOnEnter} Initiative."), {{TEXT("InitiativeGainOnEnter"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Leichte Tatzen"), TEXT("Verringert die Bewegungskosten des Baeren um {-BearMoveCost}."), {{TEXT("BearMoveCost"), -2.0}}),
            MakeAbilitySkillpoint(TEXT("Fressrausch"), TEXT("Der Baer heilt sich um 20% des von ihm verursachten Schadens."), {{TEXT("BearLifestealFraction"), 0.20}}),
            MakeAbilitySkillpoint(TEXT("Winterschlaf"), TEXT("Zu Beginn jedes Zuges in Baerenform heilt sich der Baer um {BearHealOnTurnStart}."), {{TEXT("BearHealOnTurnStart"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Erholte Rueckkehr"), TEXT("Beim freiwilligen oder erzwungenen Ende der Baerenform wird die gespeicherte Druidenform um {DruidHealOnExit} geheilt."), {{TEXT("DruidHealOnExit"), 180.0}}),
            MakeAbilitySkillpoint(TEXT("Uralter Baer"), TEXT("Erhoeht Gesundheit und Verteidigung des Baeren stark."), {{TEXT("BearMaxHealth"), 450.0}, {TEXT("BearDefense"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Raubtier"), TEXT("Erhoeht den Angriff des Baeren und gewaehrt Lebensraub."), {{TEXT("BearAttack"), 80.0}, {TEXT("BearLifestealFraction"), 0.25}}),
            MakeAbilitySkillpoint(TEXT("Formkreislauf"), TEXT("Verringert den Cooldown stark und heilt die Druidenform bei der Rueckkehr."), {{TEXT("Cooldown"), -150.0}, {TEXT("DruidHealOnExit"), 250.0}})
        }}
    };
    for (const FDefaults& Entry : Defaults)
    {
        const FString AbilityPath = FString::Printf(TEXT("/Game/TopDown/Data/Abilities/DA_Ability_%s.DA_Ability_%s"), Entry.Asset, Entry.Asset);
        const FString ProgressionPath = FString::Printf(TEXT("/Game/TopDown/Data/SkillProgression/DA_SkillProgression_%s.DA_SkillProgression_%s"), Entry.Asset, Entry.Asset);
        URoundAbilityData* Ability = LoadObject<URoundAbilityData>(nullptr, *AbilityPath);
        URoundSkillProgressionData* Progression = LoadObject<URoundSkillProgressionData>(nullptr, *ProgressionPath);
        if (!Ability || !Progression) continue;
        Progression->Modify(); Progression->SkillId = Ability->AbilityId; Progression->DisplayName = FText::FromString(Entry.Name); Progression->Description = FText::FromString(Entry.Description); Progression->Levels = Entry.Levels; Progression->MarkPackageDirty();
        Ability->Modify(); Ability->SkillProgression = Progression;
        if (FString(Entry.Asset).Equals(TEXT("Rootsnare"))) { Ability->FieldState = ERoundFieldState::Enemy; Ability->bRequiresTargetCharacter = true; Ability->bRequiresTargetTile = true; }
        if (FString(Entry.Asset).Equals(TEXT("ThornPatch"))) { Ability->FieldState = ERoundFieldState::Any; Ability->bRequiresTargetTile = true; }
        Ability->MarkPackageDirty();
    }
    MarkPackageDirty();
#endif
}

void URoundAbilityRegistryData::PopulateDragoonAbilityProgressionDefaults()
{
#if WITH_EDITOR
    struct FDefaults { const TCHAR* Asset; const TCHAR* Name; const TCHAR* Description; TArray<FRoundSkillpointDetails> Levels; };
    const TArray<FDefaults> Defaults = {
        { TEXT("Lunge"), TEXT("Lunge"), TEXT("Stuermt geradlinig vor, stoppt am ersten Gegner und verbindet Bewegung mit einem Lanzenstoss."), {
            MakeAbilitySkillpoint(TEXT("Weite Lanze"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Kraeftiger Stoss"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Anlauf"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -45.0}}),
            MakeAbilitySkillpoint(TEXT("Beschleunigung"), TEXT("Jedes vor dem Treffer durchquerte Feld erhoeht den Schaden um 6%."), {{TEXT("DamageMultiplierAddPerTile"), 0.06}}),
            MakeAbilitySkillpoint(TEXT("Aufgespiesst"), TEXT("Der getroffene Gegner kann sich fuer {RootDurationInitiative} Initiative nicht bewegen."), {{TEXT("RootDurationInitiative"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Schwunggewinn"), TEXT("Ein Treffer erzeugt {BonusMomentumOnHit} zusaetzliches Momentum."), {{TEXT("BonusMomentumOnHit"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Volle Distanz"), TEXT("Nach einem Treffer am Ende eines fast vollstaendigen Anlaufs erhaelt der Dragoon {InitiativeRefundAtLongRange} Initiative."), {{TEXT("InitiativeRefundAtLongRange"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Panzerstecher"), TEXT("Der Treffer ignoriert {DefenseIgnore} Verteidigung."), {{TEXT("DefenseIgnore"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Trefferfolge"), TEXT("Ein Treffer reduziert den aktuellen Cooldown um {CooldownReductionOnHit} Initiative."), {{TEXT("CooldownReductionOnHit"), 55.0}}),
            MakeAbilitySkillpoint(TEXT("Meteorlanze"), TEXT("Jedes durchquerte Feld erhoeht den Schaden um 10%; die Initiativekosten steigen."), {{TEXT("DamageMultiplierAddPerTile"), 0.10}, {TEXT("InitativeCost"), 8.0}}),
            MakeAbilitySkillpoint(TEXT("Fixierender Sturm"), TEXT("Der Treffer verwurzelt lange und erzeugt zusaetzliches Momentum."), {{TEXT("RootDurationInitiative"), 60.0}, {TEXT("BonusMomentumOnHit"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Endloser Anlauf"), TEXT("Erhoeht Reichweite und reduziert den Cooldown nach einem Treffer."), {{TEXT("Range"), 2.0}, {TEXT("CooldownReductionOnHit"), 70.0}})
        }},
        { TEXT("Overshoot"), TEXT("Overshoot"), TEXT("Springt ueber das anvisierte Feld und landet dahinter; Position und freie Landeflaeche bestimmen den Nutzen."), {
            MakeAbilitySkillpoint(TEXT("Harter Ueberflug"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 55.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Sprung"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Kurzer Anlauf"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -50.0}}),
            MakeAbilitySkillpoint(TEXT("Tiefe Landung"), TEXT("Erhoeht die moegliche Landedistanz hinter dem Ziel um {BehindDistance}."), {{TEXT("BehindDistance"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Erschuetterung"), TEXT("Ein getroffener Gegner verliert {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Weicher Fall"), TEXT("Nach der Landung erhaelt der Dragoon {InitiativeGainOnLanding} Initiative."), {{TEXT("InitiativeGainOnLanding"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Fallhoehe"), TEXT("Jedes Feld hinter dem Ziel erhoeht den Schaden um 12%."), {{TEXT("DamageMultiplierAddPerBehindTile"), 0.12}}),
            MakeAbilitySkillpoint(TEXT("Luftkampf"), TEXT("Ein Treffer erzeugt {BonusMomentumOnHit} zusaetzliches Momentum."), {{TEXT("BonusMomentumOnHit"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Praezisionssprung"), TEXT("Ignoriert Verteidigung und kostet weniger Initiative."), {{TEXT("DefenseIgnore"), 30.0}, {TEXT("InitativeCost"), -8.0}}),
            MakeAbilitySkillpoint(TEXT("Himmelssturz"), TEXT("Jedes Feld hinter dem Ziel erhoeht den Schaden um 20%; die Landedistanz steigt."), {{TEXT("DamageMultiplierAddPerBehindTile"), 0.20}, {TEXT("BehindDistance"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Fliegender Rhythmus"), TEXT("Die Landung gibt Initiative und ein Treffer erzeugt Momentum."), {{TEXT("InitiativeGainOnLanding"), 25.0}, {TEXT("BonusMomentumOnHit"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Grenzenlos"), TEXT("Erhoeht Reichweite und Landedistanz deutlich."), {{TEXT("Range"), 2.0}, {TEXT("BehindDistance"), 2.0}})
        }},
        { TEXT("ExecuteDive"), TEXT("Execute Dive"), TEXT("Springt neben einen verwundeten Gegner und verursacht gegen Ziele unter der Lebensschwelle stark erhoehten Schaden."), {
            MakeAbilitySkillpoint(TEXT("Tiefer Sturz"), TEXT("Erhoeht den Grundschaden um {Damage}."), {{TEXT("Damage"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Fall"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Jagdtempo"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -35.0}}),
            MakeAbilitySkillpoint(TEXT("Fruehe Beute"), TEXT("Erhoeht die Lebensschwelle fuer den Schadensbonus um 10 Prozentpunkte."), {{TEXT("LowHealthThresholdPercent"), 0.10}}),
            MakeAbilitySkillpoint(TEXT("Durchbohren"), TEXT("Der Treffer ignoriert {DefenseIgnore} Verteidigung."), {{TEXT("DefenseIgnore"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Todesschwung"), TEXT("Eine Toetung gibt {InitiativeGainOnKill} Initiative zurueck."), {{TEXT("InitiativeGainOnKill"), 22.0}}),
            MakeAbilitySkillpoint(TEXT("Fallende Klinge"), TEXT("Erhoeht den Multiplikator gegen Ziele unter der Schwelle um 0,5."), {{TEXT("DamageMultiplierVsLowHealth"), 0.5}}),
            MakeAbilitySkillpoint(TEXT("Wiederaufstieg"), TEXT("Eine Toetung reduziert den aktuellen Cooldown um {CooldownReductionOnKill} Initiative."), {{TEXT("CooldownReductionOnKill"), 80.0}}),
            MakeAbilitySkillpoint(TEXT("Momentumopfer"), TEXT("Jeder vorhandene Momentum-Stack gibt {BonusDamagePerMomentumConsumed} Bonusschaden; nach dem Treffer wird alles Momentum verbraucht."), {{TEXT("BonusDamagePerMomentumConsumed"), 35.0}, {TEXT("ConsumeMomentum"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Hinrichtung"), TEXT("Erhoeht Schwelle und Schadensmultiplikator gegen verwundete Ziele."), {{TEXT("LowHealthThresholdPercent"), 0.15}, {TEXT("DamageMultiplierVsLowHealth"), 0.65}}),
            MakeAbilitySkillpoint(TEXT("Aufstieg des Siegers"), TEXT("Eine Toetung gibt Initiative zurueck und reduziert den Cooldown."), {{TEXT("InitiativeGainOnKill"), 30.0}, {TEXT("CooldownReductionOnKill"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Alles oder nichts"), TEXT("Verbraucht alles Momentum fuer hohen Bonusschaden pro Stack."), {{TEXT("BonusDamagePerMomentumConsumed"), 55.0}, {TEXT("ConsumeMomentum"), 1.0}})
        }},
        { TEXT("HoldtheLine"), TEXT("Hold the Line"), TEXT("Beendet den Zug und bedroht Gegner, die bis zum naechsten eigenen Zug die Lanzenreichweite verlassen."), {
            MakeAbilitySkillpoint(TEXT("Scharfe Reaktion"), TEXT("Erhoeht den Reaktionsschaden um {Damage}."), {{TEXT("Damage"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Lange Wacht"), TEXT("Erhoeht die Bedrohungsreichweite um {ThreatRangeAdd}."), {{TEXT("ThreatRangeAdd"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Bereitstellung"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -55.0}}),
            MakeAbilitySkillpoint(TEXT("Abfangstoss"), TEXT("Ein getroffener Gegner verliert {InitiativeLossOnTrigger} Initiative."), {{TEXT("InitiativeLossOnTrigger"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Geschlossene Formation"), TEXT("Solange Hold the Line aktiv ist, erleidet der Dragoon 15% weniger Schaden."), {{TEXT("IncomingDamageMultiplierAdd"), -0.15}}),
            MakeAbilitySkillpoint(TEXT("Panzerbrechende Wacht"), TEXT("Reaktionstreffer ignorieren {DefenseIgnore} Verteidigung."), {{TEXT("DefenseIgnore"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Weites Sperrgebiet"), TEXT("Erhoeht die Bedrohungsreichweite um {ThreatRangeAdd} und den Reaktionsschaden."), {{TEXT("ThreatRangeAdd"), 2.0}, {TEXT("Damage"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Unnachgiebig"), TEXT("Solange Hold the Line aktiv ist, erleidet der Dragoon 25% weniger Schaden."), {{TEXT("IncomingDamageMultiplierAdd"), -0.25}}),
            MakeAbilitySkillpoint(TEXT("Kontrollierter Gegenstoss"), TEXT("Reaktionstreffer verursachen Initiativeverlust und ignorieren Verteidigung."), {{TEXT("InitiativeLossOnTrigger"), 20.0}, {TEXT("DefenseIgnore"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Niemand entkommt"), TEXT("Erhoeht Reichweite und Schaden aller Reaktionstreffer stark."), {{TEXT("ThreatRangeAdd"), 2.0}, {TEXT("Damage"), 90.0}}),
            MakeAbilitySkillpoint(TEXT("Eiserne Linie"), TEXT("Der Dragoon erleidet 30% weniger Schaden und Reaktionstreffer entziehen Initiative."), {{TEXT("IncomingDamageMultiplierAdd"), -0.30}, {TEXT("InitiativeLossOnTrigger"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Gezielte Wacht"), TEXT("Hold the Line endet nach {MaxTriggers} Reaktionen; diese verursachen deutlich mehr Schaden und der Cooldown sinkt."), {{TEXT("MaxTriggers"), 2.0}, {TEXT("Damage"), 120.0}, {TEXT("Cooldown"), -80.0}})
        }},
        { TEXT("Repel"), TEXT("Repel"), TEXT("Stoesst einen nahen Gegner zurueck und schafft Abstand oder erzwingt eine unguenstige Position."), {
            MakeAbilitySkillpoint(TEXT("Harter Stoss"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Langer Schaft"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Gegenstoss"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -40.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Rueckstoss"), TEXT("Erhoeht die Stossdistanz um {PushDistance}."), {{TEXT("PushDistance"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Taumeln"), TEXT("Der Gegner verliert {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Abstossimpuls"), TEXT("Der Dragoon erhaelt {SelfInitiativeGain} Initiative."), {{TEXT("SelfInitiativeGain"), 12.0}}),
            MakeAbilitySkillpoint(TEXT("Wandbrecher"), TEXT("Kann der Gegner nicht die volle Distanz bewegt werden, erleidet er {CollisionDamage} zusaetzlichen Schaden."), {{TEXT("CollisionDamage"), 120.0}}),
            MakeAbilitySkillpoint(TEXT("Schwungstoss"), TEXT("Ein Treffer erzeugt {BonusMomentumOnHit} zusaetzliches Momentum."), {{TEXT("BonusMomentumOnHit"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Ruestungsbrecher"), TEXT("Ignoriert Verteidigung und kostet weniger Initiative."), {{TEXT("DefenseIgnore"), 30.0}, {TEXT("InitativeCost"), -5.0}}),
            MakeAbilitySkillpoint(TEXT("Katapult"), TEXT("Erhoeht die Stossdistanz um {PushDistance}; eine blockierte Bewegung verursacht hohen Kollisionsschaden."), {{TEXT("PushDistance"), 2.0}, {TEXT("CollisionDamage"), 170.0}}),
            MakeAbilitySkillpoint(TEXT("Taktischer Abstand"), TEXT("Entzieht dem Gegner Initiative und gibt dem Dragoon Initiative."), {{TEXT("InitiativeLossOnHit"), 28.0}, {TEXT("SelfInitiativeGain"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Unaufhaltsamer Stoss"), TEXT("Erhoeht Schaden, Verteidigungsignorierung und erzeugt Momentum."), {{TEXT("Damage"), 100.0}, {TEXT("DefenseIgnore"), 35.0}, {TEXT("BonusMomentumOnHit"), 1.0}})
        }}
    };
    for (const FDefaults& Entry : Defaults)
    {
        const FString AbilityPath = FString::Printf(TEXT("/Game/TopDown/Data/Abilities/DA_Ability_%s.DA_Ability_%s"), Entry.Asset, Entry.Asset);
        const FString ProgressionPath = FString::Printf(TEXT("/Game/TopDown/Data/SkillProgression/DA_SkillProgression_%s.DA_SkillProgression_%s"), Entry.Asset, Entry.Asset);
        URoundAbilityData* Ability = LoadObject<URoundAbilityData>(nullptr, *AbilityPath);
        URoundSkillProgressionData* Progression = LoadObject<URoundSkillProgressionData>(nullptr, *ProgressionPath);
        if (!Ability || !Progression) continue;
        Progression->Modify(); Progression->SkillId = Ability->AbilityId; Progression->DisplayName = FText::FromString(Entry.Name); Progression->Description = FText::FromString(Entry.Description); Progression->Levels = Entry.Levels; Progression->MarkPackageDirty();
        Ability->Modify(); Ability->SkillProgression = Progression;
        if (FString(Entry.Asset).Equals(TEXT("Repel"))) { Ability->FieldState = ERoundFieldState::Enemy; Ability->bRequiresTargetCharacter = true; Ability->bRequiresTargetTile = true; }
        if (FString(Entry.Asset).Equals(TEXT("ExecuteDive"))) { Ability->bRequiresTargetCharacter = true; Ability->bRequiresTargetTile = true; }
        Ability->MarkPackageDirty();
    }
    MarkPackageDirty();
#endif
}

void URoundAbilityRegistryData::PopulateGeomancerAbilityProgressionDefaults()
{
#if WITH_EDITOR
    struct FDefaults { const TCHAR* Asset; const TCHAR* Name; const TCHAR* Description; TArray<FRoundSkillpointDetails> Levels; };
    const TArray<FDefaults> Defaults = {
        { TEXT("StoneWall"), TEXT("Stone Wall"), TEXT("Errichtet eine gerade Felswand; der Baum entscheidet zwischen Ausdehnung, Tempo und unmittelbarer Kontrolle."), {
            MakeAbilitySkillpoint(TEXT("Laengere Mauer"), TEXT("Erhoeht die Mauerlaenge um {WallLength}."), {{TEXT("WallLength"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Aufbau"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -35.0}}),
            MakeAbilitySkillpoint(TEXT("Ferngestein"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Schutzwall"), TEXT("Jeder erfolgreich erzeugte Felsen heilt {HealPerRockCreated} Leben."), {{TEXT("HealPerRockCreated"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Splitterwurf"), TEXT("Neue Felsen verursachen einmalig {AdjacentDamage} Schaden an angrenzenden Gegnern."), {{TEXT("AdjacentDamage"), 70.0}}),
            MakeAbilitySkillpoint(TEXT("Guenstiger Bruch"), TEXT("Verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("InitativeCost"), -6.0}}),
            MakeAbilitySkillpoint(TEXT("Lange Barrikade"), TEXT("Erhoeht Mauerlaenge und Reichweite."), {{TEXT("WallLength"), 2.0}, {TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Aufbauimpuls"), TEXT("Jeder erzeugte Felsen gibt {InitiativePerRockCreated} Initiative, maximal 30."), {{TEXT("InitiativePerRockCreated"), 6.0}}),
            MakeAbilitySkillpoint(TEXT("Scharfe Kante"), TEXT("Neue Felsen verursachen hohen Schaden an angrenzenden Gegnern."), {{TEXT("AdjacentDamage"), 130.0}}),
            MakeAbilitySkillpoint(TEXT("Festungsbau"), TEXT("Errichtet drei weitere Felsen und kostet weniger Initiative."), {{TEXT("WallLength"), 3.0}, {TEXT("InitativeCost"), -5.0}}),
            MakeAbilitySkillpoint(TEXT("Lebender Wall"), TEXT("Jeder erzeugte Felsen heilt und gibt Initiative."), {{TEXT("HealPerRockCreated"), 45.0}, {TEXT("InitiativePerRockCreated"), 7.0}}),
            MakeAbilitySkillpoint(TEXT("Bergrutsch"), TEXT("Erhoeht Reichweite; neue Felsen beschaedigen angrenzende Gegner stark."), {{TEXT("Range"), 2.0}, {TEXT("AdjacentDamage"), 170.0}})
        }},
        { TEXT("Faultline"), TEXT("Faultline"), TEXT("Zieht eine Schadenslinie, deren Reichweite durch Felsen verlaengert und deren Schaden nach einem Felskontakt verstaerkt wird."), {
            MakeAbilitySkillpoint(TEXT("Tiefer Riss"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 40.0}}),
            MakeAbilitySkillpoint(TEXT("Weite Spalte"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Fruehes Beben"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -40.0}}),
            MakeAbilitySkillpoint(TEXT("Felsresonanz"), TEXT("Erhoeht den Bonusschaden nach einem Felskontakt um {RockBonusDamage}."), {{TEXT("RockBonusDamage"), 55.0}}),
            MakeAbilitySkillpoint(TEXT("Erschuetterung"), TEXT("Getroffene Gegner verlieren {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 16.0}}),
            MakeAbilitySkillpoint(TEXT("Offene Naht"), TEXT("Ignoriert {DefenseIgnore} Verteidigung."), {{TEXT("DefenseIgnore"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Nachhall"), TEXT("Jeder durchquerte Felsen reduziert den aktuellen Cooldown um {CooldownReductionPerRock}, maximal 120."), {{TEXT("CooldownReductionPerRock"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Endlose Kluft"), TEXT("Erhoeht Reichweite und Felsbonusschaden."), {{TEXT("Range"), 2.0}, {TEXT("RockBonusDamage"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Schockader"), TEXT("Entzieht Initiative und ignoriert Verteidigung."), {{TEXT("InitiativeLossOnHit"), 22.0}, {TEXT("DefenseIgnore"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Kontinentalnaht"), TEXT("Erhoeht Schaden und Felsbonusschaden stark."), {{TEXT("Damage"), 80.0}, {TEXT("RockBonusDamage"), 90.0}}),
            MakeAbilitySkillpoint(TEXT("Resonanzkette"), TEXT("Felsen reduzieren den Cooldown stark und verlaengern die Linie."), {{TEXT("CooldownReductionPerRock"), 28.0}, {TEXT("Range"), 2.0}}),
            MakeAbilitySkillpoint(TEXT("Tektonischer Schlag"), TEXT("Hoher Schaden, Verteidigungsignorierung und Initiativeverlust."), {{TEXT("Damage"), 90.0}, {TEXT("DefenseIgnore"), 35.0}, {TEXT("InitiativeLossOnHit"), 25.0}})
        }},
        { TEXT("Detonate"), TEXT("Detonate"), TEXT("Zuendet mehrere Felsen im Zielbereich; ueberlappende Explosionen belohnen vorbereitete Felsgruppen."), {
            MakeAbilitySkillpoint(TEXT("Mehr Sprengkraft"), TEXT("Erhoeht den Schaden jeder Explosion um {Damage}."), {{TEXT("Damage"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Zuender"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Kurze Lunte"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -40.0}}),
            MakeAbilitySkillpoint(TEXT("Suchladung"), TEXT("Erhoeht den Bereich, in dem Felsen gezuendet werden, um {AOERange}."), {{TEXT("AOERange"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Druckwelle"), TEXT("Getroffene Gegner verlieren {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Hohlladung"), TEXT("Explosionen ignorieren {DefenseIgnore} Verteidigung."), {{TEXT("DefenseIgnore"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Kettenreaktion"), TEXT("Jeder gezuendete Felsen reduziert den aktuellen Cooldown um {CooldownReductionPerRock}, maximal 120."), {{TEXT("CooldownReductionPerRock"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Grossladung"), TEXT("Erhoeht Explosionsradius und Schaden."), {{TEXT("ExplosionRadius"), 1.0}, {TEXT("Damage"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Stabile Ladung"), TEXT("Gezuendete Felsen bleiben bestehen."), {{TEXT("PreserveRocks"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Bebenzentrum"), TEXT("Erhoeht Suchbereich und Explosionsradius."), {{TEXT("AOERange"), 2.0}, {TEXT("ExplosionRadius"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Unerschoepfliches Feld"), TEXT("Felsen bleiben bestehen und jede Zuendung reduziert den Cooldown."), {{TEXT("PreserveRocks"), 1.0}, {TEXT("CooldownReductionPerRock"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Gebirgssprengung"), TEXT("Explosionen verursachen hohen Schaden und ignorieren Verteidigung."), {{TEXT("Damage"), 100.0}, {TEXT("DefenseIgnore"), 40.0}})
        }},
        { TEXT("Tremor"), TEXT("Tremor"), TEXT("Erschuettert den Nahbereich um den Geomancer, markiert Angreifer und stabilisiert seine Position gegen Druck."), {
            MakeAbilitySkillpoint(TEXT("Staerkeres Beben"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 40.0}}),
            MakeAbilitySkillpoint(TEXT("Schnelles Beben"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -45.0}}),
            MakeAbilitySkillpoint(TEXT("Leichte Erschuetterung"), TEXT("Verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("InitativeCost"), -7.0}}),
            MakeAbilitySkillpoint(TEXT("Gegenimpuls"), TEXT("Jeder getroffene Gegner gibt {InitiativePerTargetHit} Initiative, maximal 30."), {{TEXT("InitiativePerTargetHit"), 8.0}}),
            MakeAbilitySkillpoint(TEXT("Taumelwelle"), TEXT("Getroffene Gegner verlieren {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Gebrochener Stand"), TEXT("Verursacht {BonusDamageVsMarked} Bonusschaden gegen bereits markierte Gegner."), {{TEXT("BonusDamageVsMarked"), 65.0}}),
            MakeAbilitySkillpoint(TEXT("Weitbeben"), TEXT("Erhoeht den Wirkungsradius um {AOERange}."), {{TEXT("AOERange"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Standhafte Welle"), TEXT("Jeder Treffer gibt Initiative und der Cooldown sinkt."), {{TEXT("InitiativePerTargetHit"), 10.0}, {TEXT("Cooldown"), -35.0}}),
            MakeAbilitySkillpoint(TEXT("Bruchbrecher"), TEXT("Hoher Bonusschaden gegen markierte Ziele und Verteidigungsignorierung."), {{TEXT("BonusDamageVsMarked"), 100.0}, {TEXT("DefenseIgnore"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Erdbeben"), TEXT("Erhoeht Radius und Schaden stark."), {{TEXT("AOERange"), 2.0}, {TEXT("Damage"), 75.0}}),
            MakeAbilitySkillpoint(TEXT("Stillstand"), TEXT("Entzieht viel Initiative und gibt pro Treffer Initiative zurueck."), {{TEXT("InitiativeLossOnHit"), 30.0}, {TEXT("InitiativePerTargetHit"), 12.0}}),
            MakeAbilitySkillpoint(TEXT("Epizentrum"), TEXT("Markierte Ziele erleiden massiven Bonusschaden; ihre Verteidigung wird teilweise ignoriert."), {{TEXT("BonusDamageVsMarked"), 150.0}, {TEXT("DefenseIgnore"), 35.0}})
        }},
        { TEXT("Landsplitter"), TEXT("Landsplitter"), TEXT("Spaltet einen breiten Korridor, markiert Gegner und verschiebt die gesamte Front vom Geomancer weg."), {
            MakeAbilitySkillpoint(TEXT("Schwerer Spalt"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Laengerer Spalt"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Bruch"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -50.0}}),
            MakeAbilitySkillpoint(TEXT("Weitwurf"), TEXT("Erhoeht die Stossdistanz um {PushDistance}."), {{TEXT("PushDistance"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Nachzittern"), TEXT("Getroffene Gegner verlieren {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Frakturbruch"), TEXT("Verursacht {BonusDamageVsMarked} Bonusschaden gegen bereits markierte Gegner."), {{TEXT("BonusDamageVsMarked"), 70.0}}),
            MakeAbilitySkillpoint(TEXT("Tiefe Furche"), TEXT("Ignoriert Verteidigung und erhoeht den Schaden."), {{TEXT("DefenseIgnore"), 30.0}, {TEXT("Damage"), 40.0}}),
            MakeAbilitySkillpoint(TEXT("Frontverschiebung"), TEXT("Stoesst Gegner weiter zurueck und entzieht Initiative."), {{TEXT("PushDistance"), 1.0}, {TEXT("InitiativeLossOnHit"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Markenbrecher"), TEXT("Verursacht hohen Bonusschaden gegen bereits markierte Gegner."), {{TEXT("BonusDamageVsMarked"), 130.0}}),
            MakeAbilitySkillpoint(TEXT("Kontinentaldrift"), TEXT("Erhoeht Reichweite und Stossdistanz stark."), {{TEXT("Range"), 2.0}, {TEXT("PushDistance"), 2.0}}),
            MakeAbilitySkillpoint(TEXT("Totale Erschuetterung"), TEXT("Entzieht viel Initiative und ignoriert Verteidigung."), {{TEXT("InitiativeLossOnHit"), 30.0}, {TEXT("DefenseIgnore"), 40.0}}),
            MakeAbilitySkillpoint(TEXT("Weltenbruch"), TEXT("Massiver Schaden gegen markierte Ziele und stark erhoehter Grundschaden."), {{TEXT("Damage"), 100.0}, {TEXT("BonusDamageVsMarked"), 160.0}})
        }}
    };
    for (const FDefaults& Entry : Defaults)
    {
        const FString AbilityPath = FString::Printf(TEXT("/Game/TopDown/Data/Abilities/DA_Ability_%s.DA_Ability_%s"), Entry.Asset, Entry.Asset);
        const FString ProgressionPath = FString::Printf(TEXT("/Game/TopDown/Data/SkillProgression/DA_SkillProgression_%s.DA_SkillProgression_%s"), Entry.Asset, Entry.Asset);
        URoundAbilityData* Ability = LoadObject<URoundAbilityData>(nullptr, *AbilityPath);
        URoundSkillProgressionData* Progression = LoadObject<URoundSkillProgressionData>(nullptr, *ProgressionPath);
        if (!Ability || !Progression) continue;
        Progression->Modify(); Progression->SkillId = Ability->AbilityId; Progression->DisplayName = FText::FromString(Entry.Name); Progression->Description = FText::FromString(Entry.Description); Progression->Levels = Entry.Levels; Progression->MarkPackageDirty();
        Ability->Modify(); Ability->SkillProgression = Progression; Ability->MarkPackageDirty();
    }
    MarkPackageDirty();
#endif
}

void URoundAbilityRegistryData::PopulateBladecallerAbilityProgressionDefaults()
{
#if WITH_EDITOR
    struct FDefaults { const TCHAR* Asset; const TCHAR* Name; const TCHAR* Description; TArray<FRoundSkillpointDetails> Levels; };
    const TArray<FDefaults> Defaults = {
        { TEXT("LaunchBlade"), TEXT("Launch Blade"), TEXT("Schleudert eine neue Klinge geradlinig durch Gegner und laesst sie als kuenftigen Wegpunkt liegen."), {
            MakeAbilitySkillpoint(TEXT("Scharfer Wurf"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Wurf"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Feste Bindung"), TEXT("Die erzeugte Klinge bleibt {BladeLifetime} Initiative laenger bestehen."), {{TEXT("BladeLifetime"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Durchschlag"), TEXT("Ignoriert {DefenseIgnore} Verteidigung."), {{TEXT("DefenseIgnore"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Rueckstrom"), TEXT("Jeder getroffene Gegner gibt {InitiativePerHit} Initiative, maximal 25."), {{TEXT("InitiativePerHit"), 8.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Nachwurf"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -18.0}}),
            MakeAbilitySkillpoint(TEXT("Beschleunigte Klinge"), TEXT("Jedes durchquerte Feld erhoeht den Schaden um 5%."), {{TEXT("DamageMultiplierAddPerTraversedTile"), 0.05}}),
            MakeAbilitySkillpoint(TEXT("Mehrfachtreffer"), TEXT("Trifft der Wurf mindestens zwei Gegner, sinkt der aktuelle Cooldown um {CooldownReductionOnMultiHit} Initiative."), {{TEXT("CooldownReductionOnMultiHit"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Dauerwerfer"), TEXT("Erhoeht Reichweite und Lebensdauer der Klinge."), {{TEXT("Range"), 2.0}, {TEXT("BladeLifetime"), 70.0}}),
            MakeAbilitySkillpoint(TEXT("Ballistische Schneide"), TEXT("Jedes durchquerte Feld erhoeht den Schaden um 9%; der Grundschaden steigt."), {{TEXT("DamageMultiplierAddPerTraversedTile"), 0.09}, {TEXT("Damage"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Kettenwurf"), TEXT("Mehrfachtreffer reduzieren den Cooldown stark und geben Initiative pro Treffer."), {{TEXT("CooldownReductionOnMultiHit"), 45.0}, {TEXT("InitiativePerHit"), 10.0}}),
            MakeAbilitySkillpoint(TEXT("Unendliche Schneide"), TEXT("Erhoeht Reichweite, Verteidigungsignorierung und Klingenlebensdauer."), {{TEXT("Range"), 2.0}, {TEXT("DefenseIgnore"), 35.0}, {TEXT("BladeLifetime"), 90.0}})
        }},
        { TEXT("DriveBlades"), TEXT("Drive Blades"), TEXT("Treibt alle aktiven Klingen durch einen gemeinsamen Zielpunkt; Vorbereitung und Linienwahl bestimmen den Ertrag."), {
            MakeAbilitySkillpoint(TEXT("Klingenwucht"), TEXT("Erhoeht den Schaden jeder Klinge um {Damage}."), {{TEXT("Damage"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Tiefer Durchlauf"), TEXT("Klingen fliegen {BehindDistance} Feld weiter hinter das Ziel."), {{TEXT("BehindDistance"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Frueher Sturm"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -65.0}}),
            MakeAbilitySkillpoint(TEXT("Schwarmdruck"), TEXT("Jede weitere aktive Klinge erhoeht den Schaden jedes Treffers um {BonusDamagePerAdditionalBlade}."), {{TEXT("BonusDamagePerAdditionalBlade"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Trefferstrom"), TEXT("Jeder Treffer gibt {InitiativePerHit} Initiative, maximal 35."), {{TEXT("InitiativePerHit"), 6.0}}),
            MakeAbilitySkillpoint(TEXT("Panzerklingen"), TEXT("Treffer ignorieren {DefenseIgnore} Verteidigung."), {{TEXT("DefenseIgnore"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Erneuerter Schwarm"), TEXT("Jede bewegte Klinge erneuert ihre Lebensdauer."), {{TEXT("RefreshMovedBladeLifetime"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Rotationsimpuls"), TEXT("Jede bewegte Klinge reduziert den aktuellen Cooldown um {CooldownReductionPerMovedBlade}, maximal 180."), {{TEXT("CooldownReductionPerMovedBlade"), 28.0}}),
            MakeAbilitySkillpoint(TEXT("Langer Klingenregen"), TEXT("Erhoeht Reichweite und Flugstrecke hinter dem Ziel."), {{TEXT("Range"), 1.0}, {TEXT("BehindDistance"), 2.0}}),
            MakeAbilitySkillpoint(TEXT("Tausend Schnitte"), TEXT("Weitere Klingen erhoehen den Schaden stark; Treffer geben Initiative."), {{TEXT("BonusDamagePerAdditionalBlade"), 45.0}, {TEXT("InitiativePerHit"), 7.0}}),
            MakeAbilitySkillpoint(TEXT("Endlose Rotation"), TEXT("Bewegte Klingen erneuern sich und reduzieren den Cooldown."), {{TEXT("RefreshMovedBladeLifetime"), 1.0}, {TEXT("CooldownReductionPerMovedBlade"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Klingenorkan"), TEXT("Erhoeht Grundschaden, Durchschlag und Flugweite hinter dem Ziel."), {{TEXT("Damage"), 100.0}, {TEXT("DefenseIgnore"), 40.0}, {TEXT("BehindDistance"), 2.0}})
        }},
        { TEXT("BladeSwap"), TEXT("Blade Swap"), TEXT("Tauscht den Bladecaller mit einer platzierten Klinge; deren Rueckflug schneidet die Linie zur alten Position."), {
            MakeAbilitySkillpoint(TEXT("Scharfer Tausch"), TEXT("Erhoeht den Rueckflugschaden um {Damage}."), {{TEXT("Damage"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Tausch"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Fliegender Wechsel"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -40.0}}),
            MakeAbilitySkillpoint(TEXT("Distanzschnitt"), TEXT("Jedes getauschte Feld erhoeht den Rueckflugschaden um 6%."), {{TEXT("DamageMultiplierAddPerSwapTile"), 0.06}}),
            MakeAbilitySkillpoint(TEXT("Neu gebunden"), TEXT("Der Tausch erneuert die Lebensdauer der getauschten Klinge."), {{TEXT("RefreshSwappedBladeLifetime"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Raumspalter"), TEXT("Der Rueckflug ignoriert {DefenseIgnore} Verteidigung."), {{TEXT("DefenseIgnore"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Wechselimpuls"), TEXT("Ein Tausch ueber mindestens 4 Felder gibt {InitiativeGainOnLongSwap} Initiative."), {{TEXT("LongSwapMinimumDistance"), 4.0}, {TEXT("InitiativeGainOnLongSwap"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Horizontschnitt"), TEXT("Jedes getauschte Feld erhoeht den Schaden um 10%; die Reichweite steigt."), {{TEXT("DamageMultiplierAddPerSwapTile"), 0.10}, {TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Sicherer Anker"), TEXT("Erneuert die Klinge und verringert Initiativekosten."), {{TEXT("RefreshSwappedBladeLifetime"), 1.0}, {TEXT("InitativeCost"), -7.0}}),
            MakeAbilitySkillpoint(TEXT("Dimensionsschnitt"), TEXT("Hoher Distanzschaden und Verteidigungsignorierung."), {{TEXT("DamageMultiplierAddPerSwapTile"), 0.13}, {TEXT("DefenseIgnore"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Sprungkette"), TEXT("Erhoeht Reichweite; ein weiter Tausch gibt Initiative und erneuert die Klinge."), {{TEXT("Range"), 2.0}, {TEXT("LongSwapMinimumDistance"), 4.0}, {TEXT("InitiativeGainOnLongSwap"), 24.0}, {TEXT("RefreshSwappedBladeLifetime"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Vollkommener Wechsel"), TEXT("Erhoeht Schaden stark und reduziert Kosten sowie Cooldown."), {{TEXT("Damage"), 100.0}, {TEXT("InitativeCost"), -8.0}, {TEXT("Cooldown"), -55.0}})
        }},
        { TEXT("BladeCall"), TEXT("Blade Call"), TEXT("Lenkt die dem Ziel naechste Klinge dorthin und verursacht am Ziel einen Flaechentreffer."), {
            MakeAbilitySkillpoint(TEXT("Harter Ruf"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 40.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Ruf"), TEXT("Erhoeht die Zielreichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Neue Bindung"), TEXT("Die gerufene Klinge erneuert ihre Lebensdauer."), {{TEXT("RefreshBladeLifetime"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Klingenexplosion"), TEXT("Erhoeht den Zielradius um {AOERange}."), {{TEXT("AOERange"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Rufstrom"), TEXT("Jeder Treffer gibt {InitiativePerHit} Initiative, maximal 30."), {{TEXT("InitiativePerHit"), 7.0}}),
            MakeAbilitySkillpoint(TEXT("Gezielter Schnitt"), TEXT("Ignoriert {DefenseIgnore} Verteidigung."), {{TEXT("DefenseIgnore"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Doppelruf"), TEXT("Trifft Blade Call mindestens zweimal, sinkt der aktuelle Cooldown um {CooldownReductionOnMultiHit} Initiative."), {{TEXT("CooldownReductionOnMultiHit"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Gebundener Ruf"), TEXT("Erneuert die Klinge und verringert den Cooldown."), {{TEXT("RefreshBladeLifetime"), 1.0}, {TEXT("Cooldown"), -45.0}}),
            MakeAbilitySkillpoint(TEXT("Schneidender Kreis"), TEXT("Erhoeht Radius und Schaden."), {{TEXT("AOERange"), 1.0}, {TEXT("Damage"), 55.0}}),
            MakeAbilitySkillpoint(TEXT("Klingenmagnet"), TEXT("Erhoeht Reichweite und Radius stark."), {{TEXT("Range"), 2.0}, {TEXT("AOERange"), 2.0}}),
            MakeAbilitySkillpoint(TEXT("Resonanzruf"), TEXT("Mehrfachtreffer geben Initiative und reduzieren den Cooldown stark."), {{TEXT("InitiativePerHit"), 9.0}, {TEXT("CooldownReductionOnMultiHit"), 70.0}}),
            MakeAbilitySkillpoint(TEXT("Exakter Ruf"), TEXT("Hoher Schaden und Verteidigungsignorierung; die Klinge wird erneuert."), {{TEXT("Damage"), 100.0}, {TEXT("DefenseIgnore"), 40.0}, {TEXT("RefreshBladeLifetime"), 1.0}})
        }},
        { TEXT("RecallBlade"), TEXT("Recall Blade"), TEXT("Zieht eine Klinge durch ihre Rueckfluglinie, nimmt sie auf und wandelt sie in einen stapelbaren Schutz um."), {
            MakeAbilitySkillpoint(TEXT("Scharfer Rueckflug"), TEXT("Erhoeht den Rueckflugschaden um {Damage}."), {{TEXT("Damage"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Rueckruf"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Laengerer Schutz"), TEXT("Erhoeht die Dauer jedes Schutzstapels um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Verstaerkte Deckung"), TEXT("Jeder Schutzstapel reduziert Schaden um weitere 8 Prozentpunkte."), {{TEXT("DamageReduction"), 0.08}}),
            MakeAbilitySkillpoint(TEXT("Rueckkehr"), TEXT("Der Rueckruf heilt {HealOnRecall} Leben."), {{TEXT("HealOnRecall"), 90.0}}),
            MakeAbilitySkillpoint(TEXT("Schneidender Rueckruf"), TEXT("Der Rueckflug ignoriert {DefenseIgnore} Verteidigung."), {{TEXT("DefenseIgnore"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Abfangimpuls"), TEXT("Der Rueckruf gibt {InitiativeGainOnRecall} Initiative."), {{TEXT("InitiativeGainOnRecall"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Wiederkehrende Klinge"), TEXT("Der Rueckruf reduziert seinen aktuellen Cooldown um {CooldownReductionOnRecall} Initiative."), {{TEXT("CooldownReductionOnRecall"), 65.0}}),
            MakeAbilitySkillpoint(TEXT("Schutzkreis"), TEXT("Erhoeht Schutzstaerke und Schutzdauer."), {{TEXT("DamageReduction"), 0.07}, {TEXT("DurationInitiative"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Klingenpanzer"), TEXT("Jeder Stapel reduziert Schaden stark und bleibt deutlich laenger bestehen."), {{TEXT("DamageReduction"), 0.12}, {TEXT("DurationInitiative"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Geschlossener Ruecklauf"), TEXT("Rueckruf heilt, gibt Initiative und reduziert den Cooldown."), {{TEXT("HealOnRecall"), 130.0}, {TEXT("InitiativeGainOnRecall"), 20.0}, {TEXT("CooldownReductionOnRecall"), 75.0}}),
            MakeAbilitySkillpoint(TEXT("Toedlicher Rueckruf"), TEXT("Erhoeht Rueckflugschaden und Verteidigungsignorierung stark."), {{TEXT("Damage"), 110.0}, {TEXT("DefenseIgnore"), 40.0}})
        }}
    };
    for (const FDefaults& Entry : Defaults)
    {
        const FString AbilityPath = FString::Printf(TEXT("/Game/TopDown/Data/Abilities/DA_Ability_%s.DA_Ability_%s"), Entry.Asset, Entry.Asset);
        const FString ProgressionPath = FString::Printf(TEXT("/Game/TopDown/Data/SkillProgression/DA_SkillProgression_%s.DA_SkillProgression_%s"), Entry.Asset, Entry.Asset);
        URoundAbilityData* Ability = LoadObject<URoundAbilityData>(nullptr, *AbilityPath);
        URoundSkillProgressionData* Progression = LoadObject<URoundSkillProgressionData>(nullptr, *ProgressionPath);
        if (!Ability || !Progression) continue;
        Progression->Modify(); Progression->SkillId = Ability->AbilityId; Progression->DisplayName = FText::FromString(Entry.Name); Progression->Description = FText::FromString(Entry.Description); Progression->Levels = Entry.Levels; Progression->MarkPackageDirty();
        Ability->Modify(); Ability->SkillProgression = Progression; Ability->MarkPackageDirty();
    }
    MarkPackageDirty();
#endif
}

void URoundAbilityRegistryData::PopulateClericAbilityProgressionDefaults()
{
#if WITH_EDITOR
    struct FDefaults { const TCHAR* Asset; const TCHAR* Name; const TCHAR* Description; TArray<FRoundSkillpointDetails> Levels; };
    const TArray<FDefaults> Defaults = {
        { TEXT("DelayHeal"), TEXT("Delayed Heal"), TEXT("Bereitet eine starke Heilung vor, die nach kurzer Initiativeverzoegerung auf dem Verbündeten ausgeloest wird."), {
            MakeAbilitySkillpoint(TEXT("Staerkere Heilung"), TEXT("Erhoeht die Heilung um {Heal}."), {{TEXT("Heal"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Zuspruch"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Fruehe Antwort"), TEXT("Verringert die Ausloeseverzoegerung um 10 Initiative."), {{TEXT("DelayedExecutionInitiative"), -10.0}}),
            MakeAbilitySkillpoint(TEXT("Notfallgebet"), TEXT("Unter 50% Leben heilt das Ziel zusaetzlich {BonusHealBelowHalf}."), {{TEXT("BonusHealBelowHalf"), 150.0}}),
            MakeAbilitySkillpoint(TEXT("Ermutigung"), TEXT("Bei der Heilung erhaelt das Ziel {TargetInitiativeGain} Initiative."), {{TEXT("TargetInitiativeGain"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Sparsame Liturgie"), TEXT("Verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("InitativeCost"), -10.0}}),
            MakeAbilitySkillpoint(TEXT("Beharrliches Gebet"), TEXT("Erhoeht Heilung und verringert den Cooldown."), {{TEXT("Heal"), 125.0}, {TEXT("Cooldown"), -70.0}}),
            MakeAbilitySkillpoint(TEXT("Rettender Glaube"), TEXT("Unter 50% Leben steigt die Heilung stark; die Ausloesung erfolgt frueher."), {{TEXT("BonusHealBelowHalf"), 225.0}, {TEXT("DelayedExecutionInitiative"), -8.0}}),
            MakeAbilitySkillpoint(TEXT("Aufrichtende Worte"), TEXT("Das Ziel erhaelt Initiative und die Reichweite steigt."), {{TEXT("TargetInitiativeGain"), 22.0}, {TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Grosses Gebet"), TEXT("Erhoeht Grund- und Notfallheilung stark."), {{TEXT("Heal"), 225.0}, {TEXT("BonusHealBelowHalf"), 200.0}}),
            MakeAbilitySkillpoint(TEXT("Sofortige Antwort"), TEXT("Entfernt fast die gesamte Verzoegerung und gibt dem Ziel Initiative."), {{TEXT("DelayedExecutionInitiative"), -20.0}, {TEXT("TargetInitiativeGain"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Unermuedliche Fuersorge"), TEXT("Verringert Kosten und Cooldown deutlich."), {{TEXT("InitativeCost"), -15.0}, {TEXT("Cooldown"), -120.0}})
        }},
        { TEXT("Pulse"), TEXT("Pulse"), TEXT("Schuetzt einen Verbündeten, indem angrenzende Gegner beschaedigt und von ihm fortgestossen werden."), {
            MakeAbilitySkillpoint(TEXT("Staerkere Welle"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 55.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Schutz"), TEXT("Erhoeht die Zielreichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Heilender Puls"), TEXT("Heilt den geschuetzten Verbündeten um {HealCenterAlly}."), {{TEXT("HealCenterAlly"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Weite Welle"), TEXT("Erhoeht die Rueckstossdistanz um {KnockbackDistance}."), {{TEXT("KnockbackDistance"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Erschuetterung"), TEXT("Getroffene Gegner verlieren {InitiativeLossOnHit} Initiative."), {{TEXT("InitiativeLossOnHit"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Schutzimpuls"), TEXT("Der Verbündete erhaelt einen Schild ueber {ShieldCenterAlly} fuer 50 Initiative."), {{TEXT("ShieldCenterAlly"), 160.0}}),
            MakeAbilitySkillpoint(TEXT("Heilige Druckwelle"), TEXT("Erhoeht Schaden und ignoriert Verteidigung."), {{TEXT("Damage"), 70.0}, {TEXT("DefenseIgnore"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Atempause"), TEXT("Heilt den Verbündeten und verringert den Cooldown."), {{TEXT("HealCenterAlly"), 150.0}, {TEXT("Cooldown"), -55.0}}),
            MakeAbilitySkillpoint(TEXT("Abstand halten"), TEXT("Stoesst weiter und entzieht Initiative."), {{TEXT("KnockbackDistance"), 1.0}, {TEXT("InitiativeLossOnHit"), 22.0}}),
            MakeAbilitySkillpoint(TEXT("Unantastbar"), TEXT("Stoesst Gegner sehr weit weg und gibt dem Verbündeten einen Schild."), {{TEXT("KnockbackDistance"), 2.0}, {TEXT("ShieldCenterAlly"), 250.0}}),
            MakeAbilitySkillpoint(TEXT("Lebenswelle"), TEXT("Heilt stark und gibt einen Schild."), {{TEXT("HealCenterAlly"), 220.0}, {TEXT("ShieldCenterAlly"), 220.0}}),
            MakeAbilitySkillpoint(TEXT("Goettlicher Impuls"), TEXT("Hoher Schaden, Initiativeverlust und Verteidigungsignorierung."), {{TEXT("Damage"), 120.0}, {TEXT("InitiativeLossOnHit"), 28.0}, {TEXT("DefenseIgnore"), 35.0}})
        }},
        { TEXT("Rescue"), TEXT("Rescue"), TEXT("Versetzt einen Verbündeten auf ein freies Feld nahe dem Cleric und beendet dadurch gefaehrliche Positionen."), {
            MakeAbilitySkillpoint(TEXT("Weiter Ruf"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Erste Hilfe"), TEXT("Der gerettete Verbündete wird um {Heal} geheilt."), {{TEXT("Heal"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Schnelle Rettung"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -45.0}}),
            MakeAbilitySkillpoint(TEXT("Neuer Schwung"), TEXT("Der gerettete Verbündete erhaelt {TargetInitiativeGain} Initiative."), {{TEXT("TargetInitiativeGain"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Sichere Ankunft"), TEXT("Entfernt bei der Rettung einen entfernbaren Debuff."), {{TEXT("RemoveDebuff"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Kostenlose Hand"), TEXT("Verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("InitativeCost"), -5.0}}),
            MakeAbilitySkillpoint(TEXT("Feldsanitaeter"), TEXT("Heilt stark und entfernt einen Debuff."), {{TEXT("Heal"), 180.0}, {TEXT("RemoveDebuff"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Eilige Evakuierung"), TEXT("Erhoeht Reichweite und gibt Initiative."), {{TEXT("Range"), 2.0}, {TEXT("TargetInitiativeGain"), 22.0}}),
            MakeAbilitySkillpoint(TEXT("Bereitschaft"), TEXT("Verringert Kosten und Cooldown."), {{TEXT("InitativeCost"), -5.0}, {TEXT("Cooldown"), -70.0}}),
            MakeAbilitySkillpoint(TEXT("Vollstaendige Bergung"), TEXT("Heilt, entfernt einen Debuff und gibt Initiative."), {{TEXT("Heal"), 250.0}, {TEXT("RemoveDebuff"), 1.0}, {TEXT("TargetInitiativeGain"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Ueberall zugleich"), TEXT("Erhoeht Reichweite stark und reduziert den Cooldown."), {{TEXT("Range"), 3.0}, {TEXT("Cooldown"), -80.0}}),
            MakeAbilitySkillpoint(TEXT("Keine Zeit verlieren"), TEXT("Die Rettung kostet keine Initiative und gibt dem Ziel Initiative."), {{TEXT("InitativeCost"), -10.0}, {TEXT("TargetInitiativeGain"), 28.0}})
        }},
        { TEXT("Shield"), TEXT("Shield"), TEXT("Legt einen zeitlich begrenzten Schadenspuffer auf einen Verbündeten und bereitet Blessing vor."), {
            MakeAbilitySkillpoint(TEXT("Staerkerer Schild"), TEXT("Erhoeht den Schild um {ShieldAmount}."), {{TEXT("ShieldAmount"), 120.0}}),
            MakeAbilitySkillpoint(TEXT("Laengerer Schild"), TEXT("Erhoeht die Dauer um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Schutz"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Aufbauende Barriere"), TEXT("Das Ziel erhaelt {TargetInitiativeGain} Initiative."), {{TEXT("TargetInitiativeGain"), 15.0}}),
            MakeAbilitySkillpoint(TEXT("Heilige Salbe"), TEXT("Heilt das Ziel sofort um {Heal}."), {{TEXT("Heal"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Rasche Barriere"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -50.0}}),
            MakeAbilitySkillpoint(TEXT("Dicke Barriere"), TEXT("Erhoeht Schild und Dauer."), {{TEXT("ShieldAmount"), 180.0}, {TEXT("DurationInitiative"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Lebensschild"), TEXT("Heilt und erhoeht den Schild."), {{TEXT("Heal"), 150.0}, {TEXT("ShieldAmount"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Taktische Barriere"), TEXT("Gibt Initiative und verringert den Cooldown."), {{TEXT("TargetInitiativeGain"), 20.0}, {TEXT("Cooldown"), -55.0}}),
            MakeAbilitySkillpoint(TEXT("Unbrechbarer Glaube"), TEXT("Erhoeht Schild und Dauer massiv."), {{TEXT("ShieldAmount"), 300.0}, {TEXT("DurationInitiative"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Zweite Kraft"), TEXT("Heilt stark und gibt Initiative."), {{TEXT("Heal"), 220.0}, {TEXT("TargetInitiativeGain"), 24.0}}),
            MakeAbilitySkillpoint(TEXT("Staendige Wacht"), TEXT("Verringert Kosten und Cooldown deutlich."), {{TEXT("InitativeCost"), -10.0}, {TEXT("Cooldown"), -90.0}})
        }},
        { TEXT("Cleanse"), TEXT("Cleanse"), TEXT("Entfernt einen zufaelligen entfernbaren Debuff und baut Blessing auf; ohne Debuff bleibt der Segen erhalten."), {
            MakeAbilitySkillpoint(TEXT("Weiter Beistand"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Lindernde Reinigung"), TEXT("Jeder entfernte Debuff heilt {HealPerDebuffRemoved}."), {{TEXT("HealPerDebuffRemoved"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Rasche Reinigung"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -55.0}}),
            MakeAbilitySkillpoint(TEXT("Gruendliche Reinigung"), TEXT("Entfernt einen zusaetzlichen zufaelligen Debuff."), {{TEXT("AdditionalDebuffsRemoved"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Befreiung"), TEXT("Wird ein Debuff entfernt, erhaelt das Ziel {InitiativeGainOnCleanse} Initiative."), {{TEXT("InitiativeGainOnCleanse"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Leichte Liturgie"), TEXT("Verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("InitativeCost"), -7.0}}),
            MakeAbilitySkillpoint(TEXT("Reinigende Heilung"), TEXT("Entfernt einen weiteren Debuff und heilt pro entferntem Effekt."), {{TEXT("AdditionalDebuffsRemoved"), 1.0}, {TEXT("HealPerDebuffRemoved"), 130.0}}),
            MakeAbilitySkillpoint(TEXT("Neuer Anfang"), TEXT("Heilt pro Debuff und gibt Initiative."), {{TEXT("HealPerDebuffRemoved"), 150.0}, {TEXT("InitiativeGainOnCleanse"), 22.0}}),
            MakeAbilitySkillpoint(TEXT("Bereite Hand"), TEXT("Verringert Kosten und Cooldown."), {{TEXT("InitativeCost"), -5.0}, {TEXT("Cooldown"), -75.0}}),
            MakeAbilitySkillpoint(TEXT("Vollkommene Reinheit"), TEXT("Entfernt bis zu drei zusaetzliche Debuffs."), {{TEXT("AdditionalDebuffsRemoved"), 3.0}}),
            MakeAbilitySkillpoint(TEXT("Wiederherstellung"), TEXT("Jeder entfernte Debuff heilt stark und gibt Initiative."), {{TEXT("HealPerDebuffRemoved"), 220.0}, {TEXT("InitiativeGainOnCleanse"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Unermuedliche Reinigung"), TEXT("Verringert Kosten und Cooldown massiv; entfernt einen weiteren Debuff."), {{TEXT("InitativeCost"), -8.0}, {TEXT("Cooldown"), -110.0}, {TEXT("AdditionalDebuffsRemoved"), 1.0}})
        }}
    };
    for (const FDefaults& Entry : Defaults)
    {
        const FString AbilityPath = FString::Printf(TEXT("/Game/TopDown/Data/Abilities/DA_Ability_%s.DA_Ability_%s"), Entry.Asset, Entry.Asset);
        const FString ProgressionPath = FString::Printf(TEXT("/Game/TopDown/Data/SkillProgression/DA_SkillProgression_%s.DA_SkillProgression_%s"), Entry.Asset, Entry.Asset);
        URoundAbilityData* Ability = LoadObject<URoundAbilityData>(nullptr, *AbilityPath);
        URoundSkillProgressionData* Progression = LoadObject<URoundSkillProgressionData>(nullptr, *ProgressionPath);
        if (!Ability || !Progression) continue;
        Progression->Modify(); Progression->SkillId = Ability->AbilityId; Progression->DisplayName = FText::FromString(Entry.Name); Progression->Description = FText::FromString(Entry.Description); Progression->Levels = Entry.Levels; Progression->MarkPackageDirty();
        Ability->Modify(); Ability->SkillProgression = Progression; Ability->MarkPackageDirty();
    }
    MarkPackageDirty();
#endif
}

void URoundAbilityRegistryData::PopulateGamblerAbilityProgressionDefaults()
{
#if WITH_EDITOR
    struct FDefaults { const TCHAR* Asset; const TCHAR* Name; const TCHAR* Description; TArray<FRoundSkillpointDetails> Levels; };
    const TArray<FDefaults> Defaults = {
        { TEXT("QuickDraw"), TEXT("Quick Draw"), TEXT("Spielt die aelteste Handkarte als Schuss; Rang skaliert alle Werte und die Farbe bestimmt den Nebeneffekt."), {
            MakeAbilitySkillpoint(TEXT("Scharfer Schuss"), TEXT("Erhoeht den Grundschaden um {Damage}."), {{TEXT("Damage"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Weite Karte"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Zug"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -25.0}}),
            MakeAbilitySkillpoint(TEXT("Herzbube"), TEXT("Erhoeht die Herz-Heilung um {HeartsHeal} vor Rangskalierung."), {{TEXT("HeartsHeal"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Kreuzbube"), TEXT("Erhoeht den Kreuz-Initiativeverlust um {ClubsInitiativeLoss} vor Rangskalierung."), {{TEXT("ClubsInitiativeLoss"), 10.0}}),
            MakeAbilitySkillpoint(TEXT("Karobube"), TEXT("Erhoeht den Karo-Initiativegewinn um {DiamondsInitiativeGain} vor Rangskalierung."), {{TEXT("DiamondsInitiativeGain"), 10.0}}),
            MakeAbilitySkillpoint(TEXT("Pikass"), TEXT("Erhoeht den Pik-Bonusschaden um {SpadesBonusDamage} vor Rangskalierung."), {{TEXT("SpadesBonusDamage"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Neue Karte"), TEXT("Nach einem Treffer wird {DrawOnHit} Karte gezogen."), {{TEXT("DrawOnHit"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Bildkartenrhythmus"), TEXT("Eine Bildkarte reduziert den aktuellen Cooldown um {CooldownReductionOnFaceCard} Initiative."), {{TEXT("CooldownReductionOnFaceCard"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Vier Asse"), TEXT("Verstaerkt alle vier Farbeffekte."), {{TEXT("HeartsHeal"), 35.0}, {TEXT("ClubsInitiativeLoss"), 13.0}, {TEXT("DiamondsInitiativeGain"), 13.0}, {TEXT("SpadesBonusDamage"), 55.0}}),
            MakeAbilitySkillpoint(TEXT("Kartentrick"), TEXT("Nach jedem Treffer ziehen; Bildkarten reduzieren den Cooldown."), {{TEXT("DrawOnHit"), 1.0}, {TEXT("CooldownReductionOnFaceCard"), 55.0}}),
            MakeAbilitySkillpoint(TEXT("Todesschuss"), TEXT("Erhoeht Schaden und Durchschlag; eine Toetung zieht zwei Karten."), {{TEXT("Damage"), 70.0}, {TEXT("DefenseIgnore"), 35.0}, {TEXT("DrawOnKill"), 2.0}})
        }},
        { TEXT("DoubleDraw"), TEXT("Double Draw"), TEXT("Zieht sofort zwei Karten; bei voller Hand werden die aeltesten Karten automatisch abgeworfen."), {
            MakeAbilitySkillpoint(TEXT("Dritte Karte"), TEXT("Zieht {DrawCount} zusaetzliche Karte."), {{TEXT("DrawCount"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Billiger Zug"), TEXT("Verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("InitativeCost"), -5.0}}),
            MakeAbilitySkillpoint(TEXT("Schnelles Mischen"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -40.0}}),
            MakeAbilitySkillpoint(TEXT("Kartenenergie"), TEXT("Jede gezogene Karte gibt {InitiativePerCardDrawn} Initiative, maximal 25."), {{TEXT("InitiativePerCardDrawn"), 6.0}}),
            MakeAbilitySkillpoint(TEXT("Glueckspause"), TEXT("Jede gezogene Karte heilt {HealPerCardDrawn} Leben."), {{TEXT("HealPerCardDrawn"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Sauberer Zug"), TEXT("Werden alle vorgesehenen Karten gezogen, sinkt der aktuelle Cooldown um {CooldownReductionOnFullDraw}."), {{TEXT("CooldownReductionOnFullDraw"), 55.0}}),
            MakeAbilitySkillpoint(TEXT("Volle Hand"), TEXT("Zieht zwei weitere Karten; die Kosten steigen leicht."), {{TEXT("DrawCount"), 2.0}, {TEXT("InitativeCost"), 5.0}}),
            MakeAbilitySkillpoint(TEXT("Aufwind"), TEXT("Gezogene Karten geben Initiative und heilen."), {{TEXT("InitiativePerCardDrawn"), 7.0}, {TEXT("HealPerCardDrawn"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Perfekter Deal"), TEXT("Ein vollstaendiger Zug reduziert den Cooldown; die Kosten sinken."), {{TEXT("CooldownReductionOnFullDraw"), 70.0}, {TEXT("InitativeCost"), -5.0}}),
            MakeAbilitySkillpoint(TEXT("Fuenf Karten"), TEXT("Zieht drei zusaetzliche Karten."), {{TEXT("DrawCount"), 3.0}}),
            MakeAbilitySkillpoint(TEXT("Lebende Hand"), TEXT("Jede Karte heilt stark und gibt Initiative."), {{TEXT("HealPerCardDrawn"), 55.0}, {TEXT("InitiativePerCardDrawn"), 8.0}}),
            MakeAbilitySkillpoint(TEXT("Endloser Deal"), TEXT("Ein voller Zug reduziert den Cooldown massiv und die Faehigkeit kostet weniger."), {{TEXT("CooldownReductionOnFullDraw"), 100.0}, {TEXT("Cooldown"), -45.0}, {TEXT("InitativeCost"), -5.0}})
        }},
        { TEXT("LowStakes"), TEXT("Low Stakes"), TEXT("Wirft alle Handkarten unter Rang 10 ab und ersetzt sie; die Hand wird gezielt auf hohe Karten verdichtet."), {
            MakeAbilitySkillpoint(TEXT("Niedrigere Grenze"), TEXT("Behaelt bereits Karten ab Rang 9."), {{TEXT("MinimumKeptRank"), -1.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Tausch"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -45.0}}),
            MakeAbilitySkillpoint(TEXT("Abwurftempo"), TEXT("Jede abgeworfene Karte gibt {InitiativePerDiscard} Initiative, maximal 25."), {{TEXT("InitiativePerDiscard"), 5.0}}),
            MakeAbilitySkillpoint(TEXT("Versicherung"), TEXT("Jede abgeworfene Karte heilt {HealPerDiscard} Leben."), {{TEXT("HealPerDiscard"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Grosser Wechsel"), TEXT("Bei mindestens drei Abwuerfen wird {ExtraDrawCount} zusaetzliche Karte gezogen."), {{TEXT("ExtraDrawThreshold"), 3.0}, {TEXT("ExtraDrawCount"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Rotierender Einsatz"), TEXT("Jeder Abwurf reduziert den aktuellen Cooldown um {CooldownReductionPerDiscard}, maximal 100."), {{TEXT("CooldownReductionPerDiscard"), 14.0}}),
            MakeAbilitySkillpoint(TEXT("Hohe Einsaetze"), TEXT("Wirft auch Neuner ab und gibt Initiative pro Abwurf."), {{TEXT("MinimumKeptRank"), 1.0}, {TEXT("InitiativePerDiscard"), 6.0}}),
            MakeAbilitySkillpoint(TEXT("Sicheres Spiel"), TEXT("Behaelt Karten ab Rang 8 und heilt pro Abwurf."), {{TEXT("MinimumKeptRank"), -2.0}, {TEXT("HealPerDiscard"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Wechsel"), TEXT("Abwuerfe reduzieren den Cooldown und drei Abwuerfe ziehen extra."), {{TEXT("CooldownReductionPerDiscard"), 16.0}, {TEXT("ExtraDrawThreshold"), 3.0}, {TEXT("ExtraDrawCount"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Alles auf Bilder"), TEXT("Behaelt nur Bildkarten und Asse; Abwuerfe geben Initiative."), {{TEXT("MinimumKeptRank"), 1.0}, {TEXT("InitiativePerDiscard"), 8.0}}),
            MakeAbilitySkillpoint(TEXT("Neues Blatt"), TEXT("Ab drei Abwuerfen werden zwei Extrakarten gezogen und jeder Abwurf heilt."), {{TEXT("ExtraDrawThreshold"), 3.0}, {TEXT("ExtraDrawCount"), 2.0}, {TEXT("HealPerDiscard"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Dauerndes Mischen"), TEXT("Jeder Abwurf reduziert den Cooldown stark; die Grundabklingzeit sinkt."), {{TEXT("CooldownReductionPerDiscard"), 22.0}, {TEXT("Cooldown"), -60.0}})
        }},
        { TEXT("Flush"), TEXT("Flush"), TEXT("Verbraucht alle Karten der dominanten Farbe und skaliert deren farbspezifischen Effekt mit der gesamten Rangsumme."), {
            MakeAbilitySkillpoint(TEXT("Pikstaerke"), TEXT("Erhoeht den Grundschaden von Pik und Kreuz um {Damage}."), {{TEXT("Damage"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Herzkraft"), TEXT("Erhoeht die Herz-Heilung um {HeartsHeal} vor Rangskalierung."), {{TEXT("HeartsHeal"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Weiter Einsatz"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Kreuzdruck"), TEXT("Erhoeht den Kreuz-Initiativeverlust um {ClubsInitiativeLoss}."), {{TEXT("ClubsInitiativeLoss"), 8.0}}),
            MakeAbilitySkillpoint(TEXT("Karogewinn"), TEXT("Erhoeht den Karo-Initiativegewinn um {DiamondsInitiativeGain}."), {{TEXT("DiamondsInitiativeGain"), 9.0}}),
            MakeAbilitySkillpoint(TEXT("Neue Hand"), TEXT("Nach Flush wird {DrawAfterFlush} Karte gezogen."), {{TEXT("DrawAfterFlush"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Tiefer Einsatz"), TEXT("Jede verbrauchte Karte reduziert den aktuellen Cooldown um {CooldownReductionPerConsumedCard}, maximal 120."), {{TEXT("CooldownReductionPerConsumedCard"), 18.0}}),
            MakeAbilitySkillpoint(TEXT("Farbmeister"), TEXT("Verstaerkt alle vier Farbeffekte."), {{TEXT("Damage"), 30.0}, {TEXT("HeartsHeal"), 25.0}, {TEXT("ClubsInitiativeLoss"), 8.0}, {TEXT("DiamondsInitiativeGain"), 9.0}}),
            MakeAbilitySkillpoint(TEXT("Neu Austeilen"), TEXT("Zieht nach Flush zwei Karten."), {{TEXT("DrawAfterFlush"), 2.0}}),
            MakeAbilitySkillpoint(TEXT("Royal Flush"), TEXT("Verstaerkt alle Farbeffekte stark und ignoriert Verteidigung."), {{TEXT("Damage"), 55.0}, {TEXT("HeartsHeal"), 45.0}, {TEXT("ClubsInitiativeLoss"), 12.0}, {TEXT("DiamondsInitiativeGain"), 13.0}, {TEXT("DefenseIgnore"), 22.0}}),
            MakeAbilitySkillpoint(TEXT("Endloser Flush"), TEXT("Jede Karte reduziert den Cooldown; danach werden zwei Karten gezogen."), {{TEXT("CooldownReductionPerConsumedCard"), 25.0}, {TEXT("DrawAfterFlush"), 2.0}}),
            MakeAbilitySkillpoint(TEXT("All In"), TEXT("Erhoeht die Rangskalierung des Flush indirekt durch hohen Basiseffekt und kostet weniger."), {{TEXT("Damage"), 80.0}, {TEXT("HeartsHeal"), 60.0}, {TEXT("InitativeCost"), -10.0}})
        }},
        { TEXT("RigTheDeck"), TEXT("Rig the Deck"), TEXT("Ersetzt Rang und Farbe einer ausgewaehlten Handkarte durch das Profil einer zufaelligen Karte aus dem verbleibenden Deck."), {
            MakeAbilitySkillpoint(TEXT("Aufgewertete Karte"), TEXT("Erhoeht den neuen Rang um {RankAdd}."), {{TEXT("RankAdd"), 2.0}}),
            MakeAbilitySkillpoint(TEXT("Billiger Trick"), TEXT("Verringert die Initiativekosten um {-InitativeCost}."), {{TEXT("InitativeCost"), -6.0}}),
            MakeAbilitySkillpoint(TEXT("Schneller Trick"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -50.0}}),
            MakeAbilitySkillpoint(TEXT("Sicherer Rang"), TEXT("Das Ergebnis hat mindestens Rang {MinimumResultRank}."), {{TEXT("MinimumResultRank"), 9.0}}),
            MakeAbilitySkillpoint(TEXT("Nebenkarte"), TEXT("Nach dem Manipulieren wird {DrawAfterRig} Karte gezogen."), {{TEXT("DrawAfterRig"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Fingerfertigkeit"), TEXT("Der Einsatz reduziert seinen aktuellen Cooldown um {CooldownReductionOnRig}."), {{TEXT("CooldownReductionOnRig"), 55.0}}),
            MakeAbilitySkillpoint(TEXT("Hochstapler"), TEXT("Erhoeht den Rang deutlich und garantiert mindestens Rang 8."), {{TEXT("RankAdd"), 3.0}, {TEXT("MinimumResultRank"), 8.0}}),
            MakeAbilitySkillpoint(TEXT("Doppelter Trick"), TEXT("Zieht danach eine Karte und reduziert den Cooldown."), {{TEXT("DrawAfterRig"), 1.0}, {TEXT("CooldownReductionOnRig"), 65.0}}),
            MakeAbilitySkillpoint(TEXT("Vorbereitung"), TEXT("Garantiert mindestens Rang 10 und verringert die Kosten."), {{TEXT("MinimumResultRank"), 10.0}, {TEXT("InitativeCost"), -7.0}}),
            MakeAbilitySkillpoint(TEXT("Gezinktes Ass"), TEXT("Erhoeht den Rang um 5 und garantiert mindestens Rang 10."), {{TEXT("RankAdd"), 5.0}, {TEXT("MinimumResultRank"), 10.0}}),
            MakeAbilitySkillpoint(TEXT("Kartentausch"), TEXT("Zieht zwei Karten nach dem Manipulieren."), {{TEXT("DrawAfterRig"), 2.0}}),
            MakeAbilitySkillpoint(TEXT("Meisterbetrug"), TEXT("Verringert Kosten und Cooldown stark; das Ergebnis hat mindestens Rang 9."), {{TEXT("InitativeCost"), -10.0}, {TEXT("Cooldown"), -80.0}, {TEXT("MinimumResultRank"), 9.0}})
        }}
    };
    for (const FDefaults& Entry : Defaults)
    {
        const FString AbilityPath = FString::Printf(TEXT("/Game/TopDown/Data/Abilities/DA_Ability_%s.DA_Ability_%s"), Entry.Asset, Entry.Asset);
        const FString ProgressionPath = FString::Printf(TEXT("/Game/TopDown/Data/SkillProgression/DA_SkillProgression_%s.DA_SkillProgression_%s"), Entry.Asset, Entry.Asset);
        URoundAbilityData* Ability = LoadObject<URoundAbilityData>(nullptr, *AbilityPath); URoundSkillProgressionData* Progression = LoadObject<URoundSkillProgressionData>(nullptr, *ProgressionPath);
        if (!Ability || !Progression) continue;
        Progression->Modify(); Progression->SkillId = Ability->AbilityId; Progression->DisplayName = FText::FromString(Entry.Name); Progression->Description = FText::FromString(Entry.Description); Progression->Levels = Entry.Levels; Progression->MarkPackageDirty();
        Ability->Modify(); Ability->SkillProgression = Progression; Ability->MarkPackageDirty();
    }
    MarkPackageDirty();
#endif
}

void URoundAbilityRegistryData::PopulatePaladinAbilityProgressionDefaults()
{
#if WITH_EDITOR
    struct FDefaults { const TCHAR* Asset; const TCHAR* Name; const TCHAR* Description; TArray<FRoundSkillpointDetails> Levels; };
    const TArray<FDefaults> Defaults = {
        { TEXT("Smite"), TEXT("Smite"), TEXT("Ein direkter heiliger Nahkampfschlag, der den Paladin zwischen verlaesslichem Druck, Kontrolle und Selbsterhalt waehlen laesst."), {
            MakeAbilitySkillpoint(TEXT("Schwerer Schlag"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 70.0}}),
            MakeAbilitySkillpoint(TEXT("Rueckkehrender Glaube"), TEXT("Smite heilt den Paladin bei einem Treffer um {SelfHealOnHit}."), {{TEXT("SelfHealOnHit"), 80.0}}),
            MakeAbilitySkillpoint(TEXT("Eifer"), TEXT("Ein Treffer gibt {InitiativeGainOnHit} Initiative zurueck."), {{TEXT("InitiativeGainOnHit"), 12.0}}),
            MakeAbilitySkillpoint(TEXT("Richtender Hammer"), TEXT("Der Treffer entzieht dem Ziel {ImmediateInitiativeLoss} Initiative."), {{TEXT("ImmediateInitiativeLoss"), 16.0}}),
            MakeAbilitySkillpoint(TEXT("Ruestungsbrecher"), TEXT("Ignoriert {DefenseIgnore} Verteidigung."), {{TEXT("DefenseIgnore"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Rasche Litanei"), TEXT("Verringert den Cooldown um {-Cooldown}."), {{TEXT("Cooldown"), -55.0}}),
            MakeAbilitySkillpoint(TEXT("Kreuzritterschlag"), TEXT("Hoher Schaden und Verteidigungsignorierung."), {{TEXT("Damage"), 90.0}, {TEXT("DefenseIgnore"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Lebensfunke"), TEXT("Heilt und gibt Initiative bei einem Treffer."), {{TEXT("SelfHealOnHit"), 120.0}, {TEXT("InitiativeGainOnHit"), 14.0}}),
            MakeAbilitySkillpoint(TEXT("Niederschlag"), TEXT("Entzieht Initiative und wird schneller wieder bereit."), {{TEXT("ImmediateInitiativeLoss"), 22.0}, {TEXT("Cooldown"), -45.0}}),
            MakeAbilitySkillpoint(TEXT("Gottesurteil"), TEXT("Massiv erhoehter Schaden mit starkem Durchschlag."), {{TEXT("Damage"), 180.0}, {TEXT("DefenseIgnore"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Unermuedlicher Eifer"), TEXT("Gibt viel Initiative zurueck und verringert Kosten sowie Cooldown."), {{TEXT("InitiativeGainOnHit"), 20.0}, {TEXT("InitativeCost"), -8.0}, {TEXT("Cooldown"), -50.0}}),
            MakeAbilitySkillpoint(TEXT("Heiliger Volltreffer"), TEXT("Verbindet Schaden, Heilung und Verzoegerung des Ziels."), {{TEXT("Damage"), 110.0}, {TEXT("SelfHealOnHit"), 150.0}, {TEXT("ImmediateInitiativeLoss"), 18.0}})
        }},
        { TEXT("Condem"), TEXT("Condemn"), TEXT("Kann nur gegen einen Gegner eingesetzt werden, dessen Schaden die Paladinaura kuerzlich reduziert hat, und wandelt Schutz in Vergeltung und Schweigen um."), {
            MakeAbilitySkillpoint(TEXT("Laengeres Gedaechtnis"), TEXT("Erhoeht das Reaktionsfenster um {RecentAllyDamageWindowInitiative} Initiative."), {{TEXT("RecentAllyDamageWindowInitiative"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Scharfes Urteil"), TEXT("Erhoeht den Schaden um {Damage}."), {{TEXT("Damage"), 65.0}}),
            MakeAbilitySkillpoint(TEXT("Ferne Anklage"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Mahnendes Schweigen"), TEXT("Verlaengert das Schweigen um {SilenceDurationInitiative} Initiative."), {{TEXT("SilenceDurationInitiative"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Gerechte Erholung"), TEXT("Condemn heilt den Paladin um {SelfHealOnCondemn}."), {{TEXT("SelfHealOnCondemn"), 100.0}}),
            MakeAbilitySkillpoint(TEXT("Schnelles Urteil"), TEXT("Condemn gibt {InitiativeGainOnCondemn} Initiative zurueck."), {{TEXT("InitiativeGainOnCondemn"), 16.0}}),
            MakeAbilitySkillpoint(TEXT("Unentrinnbare Schuld"), TEXT("Erhoeht Reaktionsfenster und Reichweite."), {{TEXT("RecentAllyDamageWindowInitiative"), 30.0}, {TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Gebrochenes Siegel"), TEXT("Erhoeht Schaden und Verteidigungsignorierung."), {{TEXT("Damage"), 90.0}, {TEXT("DefenseIgnore"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Kurzes Verhoer"), TEXT("Verringert Kosten und Cooldown."), {{TEXT("InitativeCost"), -9.0}, {TEXT("Cooldown"), -55.0}}),
            MakeAbilitySkillpoint(TEXT("Endgueltiges Urteil"), TEXT("Fuegt hohen Schaden zu und schweigt deutlich laenger."), {{TEXT("Damage"), 150.0}, {TEXT("SilenceDurationInitiative"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Vergeltender Glaube"), TEXT("Heilt den Paladin und gibt Initiative zurueck."), {{TEXT("SelfHealOnCondemn"), 170.0}, {TEXT("InitiativeGainOnCondemn"), 20.0}}),
            MakeAbilitySkillpoint(TEXT("Allgegenwaertige Anklage"), TEXT("Grosses Reaktionsfenster, mehr Reichweite und stark reduzierter Cooldown."), {{TEXT("RecentAllyDamageWindowInitiative"), 50.0}, {TEXT("Range"), 2.0}, {TEXT("Cooldown"), -70.0}})
        }},
        { TEXT("SanctifyGround"), TEXT("Sanctify Ground"), TEXT("Erschafft eine zeitlich begrenzte heilige Schadenszone; ihre Position, Flaeche und Kontrollwirkung bestimmen, ob sie Raum haelt oder mit dem Paladin vorrueckt."), {
            MakeAbilitySkillpoint(TEXT("Geweihte Weite"), TEXT("Erhoeht den Zonenradius um {AuraRadius}."), {{TEXT("AuraRadius"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Brennender Boden"), TEXT("Erhoeht den Schaden jedes Initiative-Pulses um {Damage}."), {{TEXT("Damage"), 3.0}}),
            MakeAbilitySkillpoint(TEXT("Laengere Weihe"), TEXT("Erhoeht die Dauer um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 35.0}}),
            MakeAbilitySkillpoint(TEXT("Prozession"), TEXT("Die Zone bleibt nicht am Zielfeld, sondern folgt dem Paladin."), {{TEXT("FollowCaster"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Drueckende Heiligkeit"), TEXT("Jeder Puls entzieht getroffenen Gegnern {InitiativeLossPerPulse} zusaetzliche Initiative."), {{TEXT("InitiativeLossPerPulse"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Rasche Weihe"), TEXT("Verringert Initiativekosten und Cooldown."), {{TEXT("InitativeCost"), -6.0}, {TEXT("Cooldown"), -25.0}}),
            MakeAbilitySkillpoint(TEXT("Kathedralenplatz"), TEXT("Erhoeht Radius und Dauer."), {{TEXT("AuraRadius"), 1.0}, {TEXT("DurationInitiative"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Laeuterndes Feuer"), TEXT("Erhoeht Pulsschaden und Initiativeverlust."), {{TEXT("Damage"), 4.0}, {TEXT("InitiativeLossPerPulse"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Wandernde Kapelle"), TEXT("Die Zone folgt dem Paladin, haelt laenger und wird schneller bereit."), {{TEXT("FollowCaster"), 1.0}, {TEXT("DurationInitiative"), 30.0}, {TEXT("Cooldown"), -25.0}}),
            MakeAbilitySkillpoint(TEXT("Heiliges Gebiet"), TEXT("Massiv vergroesserte und verlaengerte Zone."), {{TEXT("AuraRadius"), 2.0}, {TEXT("DurationInitiative"), 60.0}}),
            MakeAbilitySkillpoint(TEXT("Unausweichliche Weihe"), TEXT("Jeder Puls verursacht mehr Schaden und entzieht mehr Initiative."), {{TEXT("Damage"), 6.0}, {TEXT("InitiativeLossPerPulse"), 2.0}}),
            MakeAbilitySkillpoint(TEXT("Pilgerzug"), TEXT("Eine grosse mobile Zone mit verringerten Kosten."), {{TEXT("FollowCaster"), 1.0}, {TEXT("AuraRadius"), 1.0}, {TEXT("InitativeCost"), -8.0}})
        }},
        { TEXT("Bulwark"), TEXT("Bulwark"), TEXT("Ein langer Selbstbuff, der eingehenden Schaden reduziert und den eigenen Schaden erhoeht; der Tree entscheidet zwischen Tankform, Angriff und nachhaltigem Kampf."), {
            MakeAbilitySkillpoint(TEXT("Dicker Panzer"), TEXT("Reduziert den eingehenden Schadensmultiplikator um 10 Prozentpunkte."), {{TEXT("SelfIncomingDamageMultiplier"), -0.10}}),
            MakeAbilitySkillpoint(TEXT("Heiliger Zorn"), TEXT("Erhoeht den ausgehenden Schadensmultiplikator um 15 Prozentpunkte."), {{TEXT("OutgoingDamageMultiplier"), 0.15}}),
            MakeAbilitySkillpoint(TEXT("Leichte Bastion"), TEXT("Verringert Bewegungskosten waehrend Bulwark um {-MoveCostAdd}."), {{TEXT("MoveCostAdd"), -3.0}}),
            MakeAbilitySkillpoint(TEXT("Lebensraubender Schwur"), TEXT("Heilt waehrend Bulwark um 10% des verursachten Schadens."), {{TEXT("HealFractionFromDamageDealt"), 0.10}}),
            MakeAbilitySkillpoint(TEXT("Unnachgiebig"), TEXT("Erhoeht die Dauer um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 50.0}}),
            MakeAbilitySkillpoint(TEXT("Brechende Gewalt"), TEXT("Angriffe waehrend Bulwark ignorieren {DefenseIgnore} Verteidigung."), {{TEXT("DefenseIgnore"), 30.0}}),
            MakeAbilitySkillpoint(TEXT("Festungsform"), TEXT("Reduziert eingehenden Schaden weiter und verlaengert die Dauer."), {{TEXT("SelfIncomingDamageMultiplier"), -0.10}, {TEXT("DurationInitiative"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Kreuzzugsform"), TEXT("Erhoeht ausgehenden Schaden und Durchschlag."), {{TEXT("OutgoingDamageMultiplier"), 0.20}, {TEXT("DefenseIgnore"), 25.0}}),
            MakeAbilitySkillpoint(TEXT("Ausdauernder Vormarsch"), TEXT("Heilt durch verursachten Schaden und verringert Bewegungskosten."), {{TEXT("HealFractionFromDamageDealt"), 0.12}, {TEXT("MoveCostAdd"), -3.0}}),
            MakeAbilitySkillpoint(TEXT("Uneinnehmbare Bastion"), TEXT("Stark verringerter eingehender Schaden und laengere Dauer."), {{TEXT("SelfIncomingDamageMultiplier"), -0.15}, {TEXT("DurationInitiative"), 70.0}}),
            MakeAbilitySkillpoint(TEXT("Zornige Bastion"), TEXT("Massiver Schadensbonus und hoher Durchschlag."), {{TEXT("OutgoingDamageMultiplier"), 0.30}, {TEXT("DefenseIgnore"), 40.0}}),
            MakeAbilitySkillpoint(TEXT("Lebende Festung"), TEXT("Vereint Schadensreduktion, Lebensraub und Beweglichkeit."), {{TEXT("SelfIncomingDamageMultiplier"), -0.10}, {TEXT("HealFractionFromDamageDealt"), 0.15}, {TEXT("MoveCostAdd"), -4.0}})
        }},
        { TEXT("ShieldOath"), TEXT("Shield Oath"), TEXT("Leitet einen Anteil des vor Mitigation berechneten Schadens eines Verbündeten auf den Paladin um und macht dessen Lebenspool zur taktischen Teamressource."), {
            MakeAbilitySkillpoint(TEXT("Staerkerer Eid"), TEXT("Leitet 10 Prozentpunkte mehr Schaden um."), {{TEXT("RedirectPreMitigationFraction"), 0.10}}),
            MakeAbilitySkillpoint(TEXT("Weiter Schwur"), TEXT("Erhoeht die Reichweite um {Range}."), {{TEXT("Range"), 1.0}}),
            MakeAbilitySkillpoint(TEXT("Anhaltender Eid"), TEXT("Erhoeht die Dauer um {DurationInitiative} Initiative."), {{TEXT("DurationInitiative"), 40.0}}),
            MakeAbilitySkillpoint(TEXT("Gepolsterter Schlag"), TEXT("Umgeleiteter Schaden trifft den Paladin 20% schwaecher."), {{TEXT("RedirectedDamageMultiplierAdd"), -0.20}}),
            MakeAbilitySkillpoint(TEXT("Mut des Beschuetzten"), TEXT("Der Verbündete erhaelt bei jedem umgeleiteten Treffer {ProtectedAllyInitiativePerHit} Initiative."), {{TEXT("ProtectedAllyInitiativePerHit"), 6.0}}),
            MakeAbilitySkillpoint(TEXT("Opfergabe"), TEXT("Der Paladin heilt um 20% des umgeleiteten Schadens, nachdem der Treffer verarbeitet wurde."), {{TEXT("PaladinHealFractionFromRedirect"), 0.20}}),
            MakeAbilitySkillpoint(TEXT("Fester Bund"), TEXT("Leitet mehr Schaden um und haelt laenger."), {{TEXT("RedirectPreMitigationFraction"), 0.10}, {TEXT("DurationInitiative"), 45.0}}),
            MakeAbilitySkillpoint(TEXT("Sanfter Bund"), TEXT("Verringert den Schaden am Paladin und heilt einen Teil zurueck."), {{TEXT("RedirectedDamageMultiplierAdd"), -0.20}, {TEXT("PaladinHealFractionFromRedirect"), 0.15}}),
            MakeAbilitySkillpoint(TEXT("Ermutigender Bund"), TEXT("Der Verbündete gewinnt Initiative; Reichweite und Cooldown verbessern sich."), {{TEXT("ProtectedAllyInitiativePerHit"), 8.0}, {TEXT("Range"), 1.0}, {TEXT("Cooldown"), -45.0}}),
            MakeAbilitySkillpoint(TEXT("Absoluter Eid"), TEXT("Leitet deutlich mehr Schaden um und bleibt laenger bestehen."), {{TEXT("RedirectPreMitigationFraction"), 0.20}, {TEXT("DurationInitiative"), 60.0}}),
            MakeAbilitySkillpoint(TEXT("Selbstloser Schutz"), TEXT("Umgeleiteter Schaden wird stark reduziert und teilweise geheilt."), {{TEXT("RedirectedDamageMultiplierAdd"), -0.35}, {TEXT("PaladinHealFractionFromRedirect"), 0.20}}),
            MakeAbilitySkillpoint(TEXT("Ungebrochene Gemeinschaft"), TEXT("Der Verbündete gewinnt Initiative; Eid kostet weniger und wird schneller bereit."), {{TEXT("ProtectedAllyInitiativePerHit"), 12.0}, {TEXT("InitativeCost"), -10.0}, {TEXT("Cooldown"), -65.0}})
        }}
    };
    for (const FDefaults& Entry : Defaults)
    {
        const FString AbilityPath = FString::Printf(TEXT("/Game/TopDown/Data/Abilities/DA_Ability_%s.DA_Ability_%s"), Entry.Asset, Entry.Asset);
        const FString ProgressionPath = FString::Printf(TEXT("/Game/TopDown/Data/SkillProgression/DA_SkillProgression_%s.DA_SkillProgression_%s"), Entry.Asset, Entry.Asset);
        URoundAbilityData* Ability = LoadObject<URoundAbilityData>(nullptr, *AbilityPath);
        URoundSkillProgressionData* Progression = LoadObject<URoundSkillProgressionData>(nullptr, *ProgressionPath);
        if (!Ability || !Progression) continue;
        Progression->Modify(); Progression->SkillId = Ability->AbilityId; Progression->DisplayName = FText::FromString(Entry.Name); Progression->Description = FText::FromString(Entry.Description); Progression->Levels = Entry.Levels; Progression->MarkPackageDirty();
        Ability->Modify(); Ability->SkillProgression = Progression; Ability->MarkPackageDirty();
    }
    MarkPackageDirty();
#endif
}

URoundAbilityData* URoundAbilityRegistryData::FindAbilityDataById(const FName AbilityId) const
{
    if (const TObjectPtr<URoundAbilityData>* Entry = Entries.FindByPredicate([AbilityId](const TObjectPtr<URoundAbilityData>& Candidate)
        {
            return Candidate && Candidate->AbilityId == AbilityId;
        }))
    {
        return Entry->Get();
    }

    return nullptr;
}

TSubclassOf<ARoundAbilityActor> URoundAbilityRegistryData::FindAbilityClassById(const FName AbilityId) const
{
    if (const TObjectPtr<URoundAbilityData>* Entry = Entries.FindByPredicate([AbilityId](const TObjectPtr<URoundAbilityData>& Candidate)
        {
            return Candidate && Candidate->AbilityId == AbilityId;
        }))
    {
        if (Entry->Get())
        {
            return Entry->Get()->AbilityClass;
        }
    }

    return nullptr;
}
