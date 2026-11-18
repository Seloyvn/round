#include "Data/RoundCharacterRegistryData.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Actors/RoundCharacter.h"
#include "Data/RoundAbilityData.h"
#include "Data/RoundCharacterData.h"
#include "Data/RoundCharacterProgressionData.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Blueprint/UserWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundCharacterRegistryImport, Log, All);

namespace
{
struct FLegacyCharacterCsvRow
{
    FString RowName;
    bool bDummy = false;
    TArray<FString> Abilities;
    FString SkillMenuPath;
    FString InitiativeIconPath;
    FString NameText;
    FString CharacterClassPath;
    FString CustomUIPath;
    FString SkillpointsPath;
    TMap<FString, double> Details;
};

FString CharacterCsvNormalizeCharacterKey(FString Value)
{
    Value = Value.TrimStartAndEnd();
    Value.ReplaceInline(TEXT("DA_Character_"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("DA_Character"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("DA_"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("BP_"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("_C"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("Details"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("Progression"), TEXT(""), ESearchCase::IgnoreCase);
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

FString CharacterCsvNormalizeAbilityKey(FString Value)
{
    Value = Value.TrimStartAndEnd();
    Value.ReplaceInline(TEXT("DA_Ability_"), TEXT(""), ESearchCase::IgnoreCase);
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

TArray<FString> ParseCharacterCsvLine(const FString& Line)
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

bool LoadCharacterCsvRows(const FString& FilePath, TArray<TMap<FString, FString>>& OutRows)
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

    const TArray<FString> Headers = ParseCharacterCsvLine(Lines[0]);
    for (int32 LineIndex = 1; LineIndex < Lines.Num(); ++LineIndex)
    {
        if (Lines[LineIndex].TrimStartAndEnd().IsEmpty())
        {
            continue;
        }

        const TArray<FString> Fields = ParseCharacterCsvLine(Lines[LineIndex]);
        TMap<FString, FString> Row;
        for (int32 HeaderIndex = 0; HeaderIndex < Headers.Num(); ++HeaderIndex)
        {
            Row.Add(Headers[HeaderIndex], Fields.IsValidIndex(HeaderIndex) ? Fields[HeaderIndex] : FString());
        }

        OutRows.Add(MoveTemp(Row));
    }

    return OutRows.Num() > 0;
}

TArray<FString> ParseLegacyAbilityArray(FString Value)
{
    TArray<FString> Result;
    Value = Value.TrimStartAndEnd();
    if (Value.IsEmpty() || Value == TEXT("()"))
    {
        return Result;
    }

    Value.RemoveFromStart(TEXT("("));
    Value.RemoveFromEnd(TEXT(")"));

    bool bInQuotes = false;
    FString Current;
    for (int32 Index = 0; Index < Value.Len(); ++Index)
    {
        const TCHAR Character = Value[Index];
        if (Character == TEXT('"'))
        {
            const bool bEscapedQuote = bInQuotes && Index + 1 < Value.Len() && Value[Index + 1] == TEXT('"');
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
            const FString Token = Current.TrimStartAndEnd();
            if (!Token.IsEmpty())
            {
                Result.Add(Token);
            }
            Current.Reset();
        }
        else
        {
            Current.AppendChar(Character);
        }
    }

    const FString LastToken = Current.TrimStartAndEnd();
    if (!LastToken.IsEmpty())
    {
        Result.Add(LastToken);
    }

    return Result;
}

TMap<FString, double> ParseCharacterDoubleMap(FString ValueString)
{
    TMap<FString, double> Result;
    ValueString = ValueString.TrimStartAndEnd();
    if (ValueString.IsEmpty() || ValueString == TEXT("()"))
    {
        return Result;
    }

    int32 Cursor = 0;
    while (Cursor < ValueString.Len())
    {
        const int32 KeyStart = ValueString.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, Cursor);
        if (KeyStart == INDEX_NONE)
        {
            break;
        }

        const int32 KeyEnd = ValueString.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromStart, KeyStart + 1);
        if (KeyEnd == INDEX_NONE)
        {
            break;
        }

        const FString Key = ValueString.Mid(KeyStart + 1, KeyEnd - KeyStart - 1);
        const int32 NumberStart = ValueString.Find(TEXT(","), ESearchCase::IgnoreCase, ESearchDir::FromStart, KeyEnd);
        const int32 PairEnd = ValueString.Find(TEXT(")"), ESearchCase::IgnoreCase, ESearchDir::FromStart, NumberStart);
        if (NumberStart == INDEX_NONE || PairEnd == INDEX_NONE)
        {
            break;
        }

        const FString NumberString = ValueString.Mid(NumberStart + 1, PairEnd - NumberStart - 1).TrimStartAndEnd();
        Result.Add(Key, FCString::Atod(*NumberString));
        Cursor = PairEnd + 1;
    }

    return Result;
}

FText ParseLegacyText(const FString& Value)
{
    FString Trimmed = Value.TrimStartAndEnd();
    if (Trimmed.IsEmpty())
    {
        return FText::GetEmpty();
    }

    const int32 LastQuote = Trimmed.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
    if (LastQuote != INDEX_NONE)
    {
        const int32 PrevQuote = Trimmed.Find(TEXT("\""), ESearchCase::IgnoreCase, ESearchDir::FromEnd, LastQuote - 1);
        if (PrevQuote != INDEX_NONE)
        {
            return FText::FromString(Trimmed.Mid(PrevQuote + 1, LastQuote - PrevQuote - 1));
        }
    }

    return FText::FromString(Trimmed);
}

template <typename TObjectType>
TObjectType* LoadObjectFromLegacyPath(const FString& LegacyPath)
{
    if (LegacyPath.IsEmpty())
    {
        return nullptr;
    }

    return LoadObject<TObjectType>(nullptr, *LegacyPath);
}

template <typename TObjectType>
TSubclassOf<TObjectType> LoadClassFromLegacyPath(const FString& LegacyPath)
{
    if (LegacyPath.IsEmpty())
    {
        return nullptr;
    }

    return LoadClass<TObjectType>(nullptr, *LegacyPath);
}

bool AppendAbilityToCharacterIfMissing(const TCHAR* CharacterDataPath, const TCHAR* AbilityDataPath)
{
    URoundCharacterData* CharacterData = LoadObject<URoundCharacterData>(nullptr, CharacterDataPath);
    URoundAbilityData* AbilityData = LoadObject<URoundAbilityData>(nullptr, AbilityDataPath);
    if (!CharacterData || !AbilityData)
    {
        UE_LOG(LogRoundCharacterRegistryImport, Warning, TEXT("AppendAbilityToCharacterIfMissing failed - Character=%s Ability=%s"),
            CharacterDataPath,
            AbilityDataPath);
        return false;
    }

    if (CharacterData->Abilities.Contains(AbilityData))
    {
        return false;
    }

    CharacterData->Modify();
    CharacterData->Abilities.Add(AbilityData);
    CharacterData->MarkPackageDirty();
    return true;
}

int32 GetDetailInt(const TMap<FString, double>& Details, const TCHAR* PrimaryKey, const int32 FallbackValue, std::initializer_list<const TCHAR*> Aliases = {})
{
    auto FindValue = [&Details](const TCHAR* Key, double& OutValue) -> bool
    {
        if (const double* ExactValue = Details.Find(Key))
        {
            OutValue = *ExactValue;
            return true;
        }

        for (const TPair<FString, double>& Pair : Details)
        {
            if (Pair.Key.Equals(Key, ESearchCase::IgnoreCase))
            {
                OutValue = Pair.Value;
                return true;
            }
        }

        return false;
    };

    double FoundValue = 0.0;
    if (FindValue(PrimaryKey, FoundValue))
    {
        return FMath::RoundToInt(FoundValue);
    }

    for (const TCHAR* Alias : Aliases)
    {
        if (FindValue(Alias, FoundValue))
        {
            return FMath::RoundToInt(FoundValue);
        }
    }

    return FallbackValue;
}

void ApplyBaseStatsFromDetails(URoundCharacterData* CharacterData)
{
    if (!CharacterData)
    {
        return;
    }

    CharacterData->BaseStats.MaxHealth = GetDetailInt(CharacterData->Details, TEXT("Health"), CharacterData->BaseStats.MaxHealth, {TEXT("MaxHealth")});
    CharacterData->BaseStats.Attack = GetDetailInt(CharacterData->Details, TEXT("Attack"), CharacterData->BaseStats.Attack);
    CharacterData->BaseStats.Defense = GetDetailInt(CharacterData->Details, TEXT("Defense"), CharacterData->BaseStats.Defense);
    CharacterData->BaseStats.MaxInitiative = GetDetailInt(CharacterData->Details, TEXT("MaxInitiative"), CharacterData->BaseStats.MaxInitiative, {TEXT("MaxInitative"), TEXT("Initiative")});
    CharacterData->BaseStats.AttackRange = GetDetailInt(CharacterData->Details, TEXT("AttackRange"), CharacterData->BaseStats.AttackRange, {TEXT("Range")});
    CharacterData->BaseStats.VisionRange = GetDetailInt(CharacterData->Details, TEXT("VisionRange"), CharacterData->BaseStats.VisionRange);
    CharacterData->BaseStats.MoveCost = GetDetailInt(CharacterData->Details, TEXT("MoveCost"), CharacterData->BaseStats.MoveCost, {TEXT("Movecost")});
    CharacterData->BaseStats.AttackCost = GetDetailInt(CharacterData->Details, TEXT("AttackCost"), CharacterData->BaseStats.AttackCost);
    CharacterData->BaseStats.TurnDelay = GetDetailInt(CharacterData->Details, TEXT("TurnDelay"), CharacterData->BaseStats.TurnDelay);
}

TMap<FString, TObjectPtr<URoundAbilityData>> BuildAbilityAssetMap()
{
    TMap<FString, TObjectPtr<URoundAbilityData>> Result;

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.ClassPaths.Add(URoundAbilityData::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(TEXT("/Game/TopDown/Data/Abilities"));
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;

    TArray<FAssetData> Assets;
    AssetRegistryModule.Get().GetAssets(Filter, Assets);
    for (const FAssetData& AssetData : Assets)
    {
        if (URoundAbilityData* AbilityData = Cast<URoundAbilityData>(AssetData.GetAsset()))
        {
            Result.Add(CharacterCsvNormalizeAbilityKey(AbilityData->GetName()), AbilityData);
            if (!AbilityData->AbilityId.IsNone())
            {
                Result.Add(CharacterCsvNormalizeAbilityKey(AbilityData->AbilityId.ToString()), AbilityData);
            }
        }
    }

    return Result;
}

bool BuildLegacyRow(const TMap<FString, FString>& CsvRow, FLegacyCharacterCsvRow& OutRow)
{
    const FString* RowName = CsvRow.Find(TEXT("---"));
    if (!RowName || RowName->IsEmpty())
    {
        return false;
    }

    OutRow.RowName = *RowName;
    OutRow.bDummy = CsvRow.FindRef(TEXT("Dummy")).Equals(TEXT("True"), ESearchCase::IgnoreCase);
    OutRow.Abilities = ParseLegacyAbilityArray(CsvRow.FindRef(TEXT("Abilities")));
    OutRow.SkillMenuPath = CsvRow.FindRef(TEXT("SkillMenu"));
    OutRow.InitiativeIconPath = CsvRow.FindRef(TEXT("InitativeIcon"));
    OutRow.NameText = CsvRow.FindRef(TEXT("Name"));
    OutRow.CharacterClassPath = CsvRow.FindRef(TEXT("CharacterClass"));
    OutRow.CustomUIPath = CsvRow.FindRef(TEXT("CustomUI"));
    OutRow.SkillpointsPath = CsvRow.FindRef(TEXT("Skillpoints"));
    OutRow.Details = ParseCharacterDoubleMap(CsvRow.FindRef(TEXT("Details")));
    return true;
}

TMap<FString, TObjectPtr<URoundCharacterProgressionData>> BuildCharacterProgressionAssetMap(const FString& AssetDirectory)
{
    TMap<FString, TObjectPtr<URoundCharacterProgressionData>> Result;

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.ClassPaths.Add(URoundCharacterProgressionData::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(*AssetDirectory);
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;

    TArray<FAssetData> Assets;
    AssetRegistryModule.Get().GetAssets(Filter, Assets);
    for (const FAssetData& AssetData : Assets)
    {
        if (URoundCharacterProgressionData* ProgressionData = Cast<URoundCharacterProgressionData>(AssetData.GetAsset()))
        {
            Result.Add(CharacterCsvNormalizeCharacterKey(ProgressionData->GetName()), ProgressionData);
            if (!ProgressionData->CharacterId.IsNone())
            {
                Result.Add(CharacterCsvNormalizeCharacterKey(ProgressionData->CharacterId.ToString()), ProgressionData);
            }
        }
    }

    return Result;
}

bool ConvertLegacySkillpointTable(UDataTable* SkillpointTable, TArray<FRoundSkillpointTier>& OutTiers)
{
    OutTiers.Reset();
    if (!SkillpointTable)
    {
        return false;
    }

    if (SkillpointTable->GetRowStruct() == FRoundSkillpointsDetails::StaticStruct())
    {
        TArray<FRoundSkillpointsDetails*> Rows;
        SkillpointTable->GetAllRows(TEXT("ConvertLegacySkillpointTable"), Rows);
        int32 TierIndex = 0;
        for (const FRoundSkillpointsDetails* Row : Rows)
        {
            if (!Row)
            {
                continue;
            }

            FRoundSkillpointTier& NewTier = OutTiers.AddDefaulted_GetRef();
            NewTier.Title = FText::Format(FText::FromString(TEXT("Tier {0}")), FText::AsNumber(++TierIndex));
            NewTier.Choices = Row->Skillpoints;
        }

        return OutTiers.Num() > 0;
    }

    if (SkillpointTable->GetRowStruct() == FRoundSkillpointDetails::StaticStruct())
    {
        TArray<FRoundSkillpointDetails*> Rows;
        SkillpointTable->GetAllRows(TEXT("ConvertLegacySkillpointTable"), Rows);

        for (int32 Index = 0; Index < Rows.Num(); Index += 3)
        {
            FRoundSkillpointTier& NewTier = OutTiers.AddDefaulted_GetRef();
            NewTier.Title = FText::Format(FText::FromString(TEXT("Tier {0}")), FText::AsNumber((Index / 3) + 1));

            for (int32 ChoiceOffset = 0; ChoiceOffset < 3; ++ChoiceOffset)
            {
                if (Rows.IsValidIndex(Index + ChoiceOffset) && Rows[Index + ChoiceOffset])
                {
                    NewTier.Choices.Add(*Rows[Index + ChoiceOffset]);
                }
            }
        }

        return OutTiers.Num() > 0;
    }

    return false;
}

FRoundSkillpointDetails MakeCharacterSkillpoint(
    const TCHAR* Title,
    const TCHAR* Description,
    std::initializer_list<TPair<FString, double>> Values)
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

FRoundSkillpointTier MakeCharacterTier(const TCHAR* Title, std::initializer_list<FRoundSkillpointDetails> Choices)
{
    FRoundSkillpointTier Result;
    Result.Title = FText::FromString(Title);
    Result.Choices.Append(Choices);
    return Result;
}
}

void URoundCharacterRegistryData::ImportFromCharacterClassDetails()
{
    const FString CsvPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / ImportRelativeCsvPath);

    TArray<TMap<FString, FString>> CsvRows;
    if (!LoadCharacterCsvRows(CsvPath, CsvRows))
    {
        UE_LOG(LogRoundCharacterRegistryImport, Warning, TEXT("Failed to load character csv from %s"), *CsvPath);
        return;
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.ClassPaths.Add(URoundCharacterData::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(*CharacterDataDirectory);
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;

    TArray<FAssetData> CharacterAssets;
    AssetRegistryModule.Get().GetAssets(Filter, CharacterAssets);

    TMap<FString, TObjectPtr<URoundCharacterData>> CharacterDataByKey;
    for (const FAssetData& AssetData : CharacterAssets)
    {
        if (URoundCharacterData* CharacterData = Cast<URoundCharacterData>(AssetData.GetAsset()))
        {
            CharacterDataByKey.Add(CharacterCsvNormalizeCharacterKey(CharacterData->GetName()), CharacterData);
            if (!CharacterData->CharacterId.IsNone())
            {
                CharacterDataByKey.Add(CharacterCsvNormalizeCharacterKey(CharacterData->CharacterId.ToString()), CharacterData);
            }
        }
    }

    const TMap<FString, TObjectPtr<URoundAbilityData>> AbilityDataByKey = BuildAbilityAssetMap();

    int32 UpdatedCount = 0;
    for (const TMap<FString, FString>& CsvRow : CsvRows)
    {
        FLegacyCharacterCsvRow LegacyRow;
        if (!BuildLegacyRow(CsvRow, LegacyRow))
        {
            continue;
        }

        URoundCharacterData* CharacterData = nullptr;
        if (TObjectPtr<URoundCharacterData>* FoundCharacterData = CharacterDataByKey.Find(CharacterCsvNormalizeCharacterKey(LegacyRow.RowName)))
        {
            CharacterData = FoundCharacterData->Get();
        }

        if (!CharacterData)
        {
            UE_LOG(LogRoundCharacterRegistryImport, Warning, TEXT("No RoundCharacterData asset found for row %s"), *LegacyRow.RowName);
            continue;
        }

        CharacterData->CharacterId = FName(*LegacyRow.RowName);
        CharacterData->DisplayName = ParseLegacyText(LegacyRow.NameText);
        if (CharacterData->Description.IsEmpty())
        {
            CharacterData->Description = CharacterData->DisplayName;
        }

        CharacterData->CharacterClass = LoadClassFromLegacyPath<ARoundCharacter>(LegacyRow.CharacterClassPath);
        CharacterData->CustomUIClass = LoadClassFromLegacyPath<UUserWidget>(LegacyRow.CustomUIPath);
        CharacterData->SkillMenuClass = LoadClassFromLegacyPath<UObject>(LegacyRow.SkillMenuPath);
        CharacterData->InitiativeIcon = LoadObjectFromLegacyPath<UTexture2D>(LegacyRow.InitiativeIconPath);
        CharacterData->SkillpointsTable = LoadObjectFromLegacyPath<UDataTable>(LegacyRow.SkillpointsPath);
        CharacterData->Details = LegacyRow.Details;

        ApplyBaseStatsFromDetails(CharacterData);

        CharacterData->Abilities.Reset();
        for (const FString& AbilityName : LegacyRow.Abilities)
        {
            if (const TObjectPtr<URoundAbilityData>* AbilityData = AbilityDataByKey.Find(CharacterCsvNormalizeAbilityKey(AbilityName)))
            {
                CharacterData->Abilities.Add(AbilityData->Get());
            }
            else
            {
                UE_LOG(LogRoundCharacterRegistryImport, Warning, TEXT("No RoundAbilityData asset found for %s -> %s"), *LegacyRow.RowName, *AbilityName);
            }
        }

        CharacterData->MarkPackageDirty();
        ++UpdatedCount;
    }

    MarkPackageDirty();
    UE_LOG(LogRoundCharacterRegistryImport, Log, TEXT("Imported %d character data assets from %s"), UpdatedCount, *CsvPath);
}

void URoundCharacterRegistryData::SyncBaseStatsFromDetails()
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.ClassPaths.Add(URoundCharacterData::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(*CharacterDataDirectory);
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;

    TArray<FAssetData> CharacterAssets;
    AssetRegistryModule.Get().GetAssets(Filter, CharacterAssets);

    int32 UpdatedCount = 0;
    for (const FAssetData& AssetData : CharacterAssets)
    {
        URoundCharacterData* CharacterData = Cast<URoundCharacterData>(AssetData.GetAsset());
        if (!CharacterData)
        {
            continue;
        }

        ApplyBaseStatsFromDetails(CharacterData);
        CharacterData->MarkPackageDirty();
        ++UpdatedCount;
    }

    MarkPackageDirty();
    UE_LOG(LogRoundCharacterRegistryImport, Log, TEXT("Synchronized BaseStats from Details for %d character data assets"), UpdatedCount);
}

void URoundCharacterRegistryData::ImportCharacterProgressionFromLegacySkillpoints()
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

    FARFilter CharacterFilter;
    CharacterFilter.ClassPaths.Add(URoundCharacterData::StaticClass()->GetClassPathName());
    CharacterFilter.PackagePaths.Add(*CharacterDataDirectory);
    CharacterFilter.bRecursivePaths = true;
    CharacterFilter.bRecursiveClasses = true;

    TArray<FAssetData> CharacterAssets;
    AssetRegistryModule.Get().GetAssets(CharacterFilter, CharacterAssets);
    const TMap<FString, TObjectPtr<URoundCharacterProgressionData>> ProgressionAssets = BuildCharacterProgressionAssetMap(CharacterProgressionDirectory);

    int32 UpdatedCount = 0;
    int32 ErrorCount = 0;
    for (const FAssetData& AssetData : CharacterAssets)
    {
        URoundCharacterData* CharacterData = Cast<URoundCharacterData>(AssetData.GetAsset());
        if (!CharacterData)
        {
            continue;
        }

        URoundCharacterProgressionData* ProgressionData = CharacterData->CharacterProgression;
        if (!ProgressionData)
        {
            const FString NormalizedKey = CharacterCsvNormalizeCharacterKey(
                !CharacterData->CharacterId.IsNone() ? CharacterData->CharacterId.ToString() : CharacterData->GetName());
            if (const TObjectPtr<URoundCharacterProgressionData>* FoundProgression = ProgressionAssets.Find(NormalizedKey))
            {
                ProgressionData = FoundProgression->Get();
                CharacterData->CharacterProgression = ProgressionData;
            }
        }

        if (!ProgressionData)
        {
            UE_LOG(LogRoundCharacterRegistryImport, Error, TEXT("No RoundCharacterProgressionData asset found for CharacterData=%s"), *GetNameSafe(CharacterData));
            ++ErrorCount;
            continue;
        }

        TArray<FRoundSkillpointTier> ImportedTiers;
        if (!ConvertLegacySkillpointTable(CharacterData->SkillpointsTable, ImportedTiers))
        {
            UE_LOG(LogRoundCharacterRegistryImport, Error, TEXT("Could not import legacy skillpoints for CharacterData=%s SkillpointsTable=%s"),
                *GetNameSafe(CharacterData),
                *GetNameSafe(CharacterData->SkillpointsTable));
            ++ErrorCount;
            continue;
        }

        ProgressionData->CharacterId = CharacterData->CharacterId;
        ProgressionData->DisplayName = !CharacterData->DisplayName.IsEmpty() ? CharacterData->DisplayName : FText::FromString(CharacterData->GetName());
        ProgressionData->Description = CharacterData->Description;
        ProgressionData->Tiers = ImportedTiers;
        ProgressionData->MarkPackageDirty();

        CharacterData->CharacterProgression = ProgressionData;
        CharacterData->MarkPackageDirty();
        ++UpdatedCount;
    }

    MarkPackageDirty();
    UE_LOG(LogRoundCharacterRegistryImport, Log, TEXT("Imported character progression for %d assets. Errors=%d"), UpdatedCount, ErrorCount);
}

void URoundCharacterRegistryData::PopulateCharacter1ProgressionDefaults()
{
    const TMap<FString, TObjectPtr<URoundCharacterProgressionData>> ProgressionAssets = BuildCharacterProgressionAssetMap(CharacterProgressionDirectory);
    const FString Character1Key = CharacterCsvNormalizeCharacterKey(TEXT("Character1"));

    URoundCharacterProgressionData* Character1Progression = nullptr;
    if (const TObjectPtr<URoundCharacterProgressionData>* FoundProgression = ProgressionAssets.Find(Character1Key))
    {
        Character1Progression = FoundProgression->Get();
    }
    else if (const TObjectPtr<URoundCharacterProgressionData>* FoundByAssetName = ProgressionAssets.Find(CharacterCsvNormalizeCharacterKey(TEXT("Character1Progression"))))
    {
        Character1Progression = FoundByAssetName->Get();
    }
    else
    {
        for (const TPair<FString, TObjectPtr<URoundCharacterProgressionData>>& Pair : ProgressionAssets)
        {
            if (Pair.Key.Contains(Character1Key))
            {
                Character1Progression = Pair.Value.Get();
                break;
            }
        }
    }

    if (!Character1Progression)
    {
        Character1Progression = LoadObject<URoundCharacterProgressionData>(
            nullptr,
            TEXT("/Game/TopDown/Characters/Character_1/DA_Character1Progression.DA_Character1Progression"));
    }

    if (!Character1Progression)
    {
        UE_LOG(LogRoundCharacterRegistryImport, Error, TEXT("PopulateCharacter1ProgressionDefaults failed - no RoundCharacterProgressionData asset found for Character1 in %s"), *CharacterProgressionDirectory);
        return;
    }

    Character1Progression->CharacterId = TEXT("Character1");
    Character1Progression->DisplayName = FText::FromString(TEXT("Tank"));
    Character1Progression->Description = FText::FromString(TEXT("Character-only upgrades for Character 1."));
    Character1Progression->Tiers = {
        MakeCharacterTier(TEXT("Kettenfundament"), {
            MakeCharacterSkillpoint(TEXT("Zusatzkette"), TEXT("Erhoeht MaxChains um {ChainLimitAdd}."), {{TEXT("ChainLimitAdd"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Ruhiger Zug"), TEXT("Das erste Mal pro Zug, wenn Character 1 eine Kette verbraucht, erhaelt er {FirstChainSpendInitiativeRefund} Initiative zurueck."), {{TEXT("FirstChainSpendInitiativeRefund"), 10.0}}),
            MakeCharacterSkillpoint(TEXT("Schwerer Stand"), TEXT("Hat Character 1 seinen letzten Zug ohne Bewegung beendet, erleidet er bis zu seinem naechsten Zug nur {StationaryIncomingDamageMultiplier}x Schaden."), {{TEXT("StationaryIncomingDamageMultiplier"), 0.9}})
        }),
        MakeCharacterTier(TEXT("Druckaufbau"), {
            MakeCharacterSkillpoint(TEXT("Kurzer Zyklus"), TEXT("Verringert ChainRechargeTime um {-ChainRechargeTimeAdd}."), {{TEXT("ChainRechargeTimeAdd"), -20.0}}),
            MakeCharacterSkillpoint(TEXT("Druckfeld"), TEXT("Gegner, die einen Debuff von Character 1 haben, verursachen gegen ihn nur {IncomingDamageMultiplierFromDebuffedEnemies}x Schaden."), {{TEXT("IncomingDamageMultiplierFromDebuffedEnemies"), 0.9}}),
            MakeCharacterSkillpoint(TEXT("Notglied"), TEXT("Beginnt Character 1 einen Zug mit 0 Ketten, erhaelt er {ZeroChainTurnStartGainChain} Kette, verliert aber {ZeroChainTurnStartInitiativePenalty} Initiative."), {{TEXT("ZeroChainTurnStartGainChain"), 1.0}, {TEXT("ZeroChainTurnStartInitiativePenalty"), 20.0}})
        }),
        MakeCharacterTier(TEXT("Kettenkern"), {
            MakeCharacterSkillpoint(TEXT("Kettenpolster"), TEXT("Solange Character 1 mindestens 1 Kette hat, erleidet er nur {DamageMultiplierWhileHasChain}x Schaden."), {{TEXT("DamageMultiplierWhileHasChain"), 0.85}}),
            MakeCharacterSkillpoint(TEXT("Ueberlauf"), TEXT("Wuerde Character 1 eine Kette regenerieren, waehrend er bereits am Limit ist, erhaelt er stattdessen {ChainOverflowInitiativeGain} Initiative."), {{TEXT("ChainOverflowInitiativeGain"), 12.0}}),
            MakeCharacterSkillpoint(TEXT("Laengerer Druck"), TEXT("Debuffs von Character 1 halten 25% laenger."), {{TEXT("DebuffDurationMultiplier"), 1.25}})
        }),
        MakeCharacterTier(TEXT("Ankerpunkt"), {
            MakeCharacterSkillpoint(TEXT("Standhalten"), TEXT("Wuerde Character 1 sterben, ueberlebt er stattdessen, verbraucht alle Ketten und heilt sich pro verbrauchter Kette um {CheatDeathHealPerConsumedChain}. Cooldown: {CheatDeathCooldownInitiative} Initiative."), {{TEXT("CheatDeathHealPerConsumedChain"), 250.0}, {TEXT("CheatDeathCooldownInitiative"), 160.0}}),
            MakeCharacterSkillpoint(TEXT("Eiserner Anker"), TEXT("Delay auf Character 1 ist nur 70% so effektiv. Wird er von einem Gegner bewegt, ignoriert er diese Bewegung mit 50% Chance."), {{TEXT("DelayReceivedMultiplier"), 0.7}, {TEXT("IgnoreEnemyForcedMovementChance"), 0.5}}),
            MakeCharacterSkillpoint(TEXT("Letzter Widerstand"), TEXT("Solange Character 1 0 Ketten hat, erhaelt er {DefenseWhileNoChain} Defense."), {{TEXT("DefenseWhileNoChain"), 40.0}})
        })
    };
    Character1Progression->MarkPackageDirty();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter CharacterFilter;
    CharacterFilter.ClassPaths.Add(URoundCharacterData::StaticClass()->GetClassPathName());
    CharacterFilter.PackagePaths.Add(*CharacterDataDirectory);
    CharacterFilter.bRecursivePaths = true;
    CharacterFilter.bRecursiveClasses = true;

    TArray<FAssetData> CharacterAssets;
    AssetRegistryModule.Get().GetAssets(CharacterFilter, CharacterAssets);
    for (const FAssetData& AssetData : CharacterAssets)
    {
        if (URoundCharacterData* CharacterData = Cast<URoundCharacterData>(AssetData.GetAsset()))
        {
            const FString CharacterKey = CharacterCsvNormalizeCharacterKey(!CharacterData->CharacterId.IsNone() ? CharacterData->CharacterId.ToString() : CharacterData->GetName());
            if (CharacterKey == CharacterCsvNormalizeCharacterKey(TEXT("Character1")))
            {
                CharacterData->CharacterProgression = Character1Progression;
                CharacterData->MarkPackageDirty();
                break;
            }
        }
    }

    MarkPackageDirty();
    UE_LOG(LogRoundCharacterRegistryImport, Log, TEXT("PopulateCharacter1ProgressionDefaults wrote defaults into %s"), *GetNameSafe(Character1Progression));
}

void URoundCharacterRegistryData::PopulateCharacter2ProgressionDefaults()
{
    const TMap<FString, TObjectPtr<URoundCharacterProgressionData>> ProgressionAssets = BuildCharacterProgressionAssetMap(CharacterProgressionDirectory);
    const FString Character2Key = CharacterCsvNormalizeCharacterKey(TEXT("Character2"));

    URoundCharacterProgressionData* Character2Progression = nullptr;
    if (const TObjectPtr<URoundCharacterProgressionData>* FoundProgression = ProgressionAssets.Find(Character2Key))
    {
        Character2Progression = FoundProgression->Get();
    }
    else if (const TObjectPtr<URoundCharacterProgressionData>* FoundByAssetName = ProgressionAssets.Find(CharacterCsvNormalizeCharacterKey(TEXT("Character2Progression"))))
    {
        Character2Progression = FoundByAssetName->Get();
    }
    else
    {
        for (const TPair<FString, TObjectPtr<URoundCharacterProgressionData>>& Pair : ProgressionAssets)
        {
            if (Pair.Key.Contains(Character2Key))
            {
                Character2Progression = Pair.Value.Get();
                break;
            }
        }
    }

    if (!Character2Progression)
    {
        Character2Progression = LoadObject<URoundCharacterProgressionData>(
            nullptr,
            TEXT("/Game/TopDown/Characters/Character_2/DA_Character2Progression.DA_Character2Progression"));
    }

    if (!Character2Progression)
    {
        UE_LOG(LogRoundCharacterRegistryImport, Error, TEXT("PopulateCharacter2ProgressionDefaults failed - no RoundCharacterProgressionData asset found for Character2 in %s"), *CharacterProgressionDirectory);
        return;
    }

    Character2Progression->CharacterId = TEXT("Character2");
    Character2Progression->DisplayName = FText::FromString(TEXT("Sniper"));
    Character2Progression->Description = FText::FromString(TEXT("Character-only upgrades for Character 2."));
    Character2Progression->Tiers = {
        MakeCharacterTier(TEXT("Ausrichtung"), {
            MakeCharacterSkillpoint(TEXT("Hartgeschoss"), TEXT("Erhoeht Attack um {AttackAdd}, aber auch MoveCost um {MoveCostAdd}."), {{TEXT("AttackAdd"), 40.0}, {TEXT("MoveCostAdd"), 2.0}}),
            MakeCharacterSkillpoint(TEXT("Zusatztasche"), TEXT("Erhoeht MaxAmmo um {MaxAmmoAdd}."), {{TEXT("MaxAmmoAdd"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Distanzpanzerung"), TEXT("Erleidet gegen Gegner in mehr als einem Feld Entfernung nur {RangedDamageMultiplier}x Schaden."), {{TEXT("RangedDamageMultiplier"), 0.85}})
        }),
        MakeCharacterTier(TEXT("Positionierung"), {
            MakeCharacterSkillpoint(TEXT("Sparschuss"), TEXT("Der erste normale Angriff pro Zug verbraucht keine Ammo."), {{TEXT("FirstBasicAttackFreeAmmo"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Leichte Ausruestung"), TEXT("Verringert MoveCost um {-MoveCostAdd}, aber auch MaxAmmo um {-MaxAmmoAdd}."), {{TEXT("MoveCostAdd"), -3.0}, {TEXT("MaxAmmoAdd"), -1.0}}),
            MakeCharacterSkillpoint(TEXT("Sicherer Stand"), TEXT("Wenn Character 2 sich seit seinem vorherigen Zug nicht bewegt hat, erhaelt er {DefenseAddWhileHoldingPosition} Defense."), {{TEXT("DefenseAddWhileHoldingPosition"), 25.0}})
        }),
        MakeCharacterTier(TEXT("Munitionsrhythmus"), {
            MakeCharacterSkillpoint(TEXT("Schnellladen"), TEXT("Erhoeht AmmoRegen um {AmmoRegenAdd}."), {{TEXT("AmmoRegenAdd"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Vollnachladen"), TEXT("Beginnt Character 2 einen Zug mit 0 Ammo, wird Ammo auf voll gesetzt, verliert aber {RefillAmmoInitiativePenalty} Initiative."), {{TEXT("RefillAmmoOnEmptyTurnStart"), 1.0}, {TEXT("RefillAmmoInitiativePenalty"), 25.0}}),
            MakeCharacterSkillpoint(TEXT("Letzter Schuss"), TEXT("Faellt Character 2 durch einen Schuss zum ersten Mal in einem Zug auf 0 Ammo, erhaelt er {EmptyMagazineInitiativeGain} Initiative."), {{TEXT("EmptyMagazineInitiativeGain"), 10.0}})
        }),
        MakeCharacterTier(TEXT("Scharfschuetze"), {
            MakeCharacterSkillpoint(TEXT("Eingeschossen"), TEXT("Jeder Munitionsschuss ohne Bewegung gibt einen Stapel Eingeschossen. Jeder Stapel erhoeht den Schaden weiterer Schuesse um {StationaryShotDamagePerStack}x, maximal {StationaryShotMaxStacks} Stapel. Bewegung entfernt alle Stapel."), {{TEXT("StationaryShotDamagePerStack"), 0.1}, {TEXT("StationaryShotMaxStacks"), 3.0}}),
            MakeCharacterSkillpoint(TEXT("Eine im Lauf"), TEXT("Solange Character 2 genau 1 Ammo hat, verursacht er {AllDamageMultiplierWhileOneAmmo}x Schaden. Besiegt ein solcher Schuss ein Ziel, erhaelt er einmal pro Zug 1 Ammo zurueck."), {{TEXT("AllDamageMultiplierWhileOneAmmo"), 1.2}, {TEXT("LastAmmoKillRefund"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Vorbereitet"), TEXT("Beginnt Character 2 seinen Zug mit voller Ammo, werden die Cooldowns seiner munitionsverbrauchenden Faehigkeiten um {FullAmmoTurnStartAbilityCooldownReduction} Initiative reduziert."), {{TEXT("FullAmmoTurnStartAbilityCooldownReduction"), 20.0}})
        })
    };
    Character2Progression->MarkPackageDirty();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter CharacterFilter;
    CharacterFilter.ClassPaths.Add(URoundCharacterData::StaticClass()->GetClassPathName());
    CharacterFilter.PackagePaths.Add(*CharacterDataDirectory);
    CharacterFilter.bRecursivePaths = true;
    CharacterFilter.bRecursiveClasses = true;

    TArray<FAssetData> CharacterAssets;
    AssetRegistryModule.Get().GetAssets(CharacterFilter, CharacterAssets);
    for (const FAssetData& AssetData : CharacterAssets)
    {
        if (URoundCharacterData* CharacterData = Cast<URoundCharacterData>(AssetData.GetAsset()))
        {
            const FString CharacterKey = CharacterCsvNormalizeCharacterKey(!CharacterData->CharacterId.IsNone() ? CharacterData->CharacterId.ToString() : CharacterData->GetName());
            if (CharacterKey == CharacterCsvNormalizeCharacterKey(TEXT("Character2")))
            {
                CharacterData->CharacterProgression = Character2Progression;
                CharacterData->MarkPackageDirty();
                break;
            }
        }
    }

    MarkPackageDirty();
    UE_LOG(LogRoundCharacterRegistryImport, Log, TEXT("PopulateCharacter2ProgressionDefaults wrote defaults into %s"), *GetNameSafe(Character2Progression));
}

void URoundCharacterRegistryData::PopulateCharacter3ProgressionDefaults()
{
    const TMap<FString, TObjectPtr<URoundCharacterProgressionData>> ProgressionAssets = BuildCharacterProgressionAssetMap(CharacterProgressionDirectory);
    const FString Character3Key = CharacterCsvNormalizeCharacterKey(TEXT("Character3"));

    URoundCharacterProgressionData* Character3Progression = nullptr;
    if (const TObjectPtr<URoundCharacterProgressionData>* FoundProgression = ProgressionAssets.Find(Character3Key))
    {
        Character3Progression = FoundProgression->Get();
    }
    else if (const TObjectPtr<URoundCharacterProgressionData>* FoundByAssetName = ProgressionAssets.Find(CharacterCsvNormalizeCharacterKey(TEXT("Character3Progression"))))
    {
        Character3Progression = FoundByAssetName->Get();
    }
    else
    {
        for (const TPair<FString, TObjectPtr<URoundCharacterProgressionData>>& Pair : ProgressionAssets)
        {
            if (Pair.Key.Contains(Character3Key))
            {
                Character3Progression = Pair.Value.Get();
                break;
            }
        }
    }

    if (!Character3Progression)
    {
        Character3Progression = LoadObject<URoundCharacterProgressionData>(
            nullptr,
            TEXT("/Game/TopDown/Characters/Character_3/DA_Character3Progression.DA_Character3Progression"));
    }

    if (!Character3Progression)
    {
        UE_LOG(LogRoundCharacterRegistryImport, Error, TEXT("PopulateCharacter3ProgressionDefaults failed - no RoundCharacterProgressionData asset found for Character3 in %s"), *CharacterProgressionDirectory);
        return;
    }

    Character3Progression->CharacterId = TEXT("Character3");
    Character3Progression->DisplayName = FText::FromString(TEXT("Assassin"));
    Character3Progression->Description = FText::FromString(TEXT("Character-only upgrades for Character 3."));
    Character3Progression->Tiers = {
        MakeCharacterTier(TEXT("Kampfroutine"), {
            MakeCharacterSkillpoint(TEXT("Reservestapel"), TEXT("Erhoeht MaxCombo um {MaxComboPointsAdd}."), {{TEXT("MaxComboPointsAdd"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Eroeffnungsschnitt"), TEXT("Der erste Treffer pro Zug gegen einen Gegner mit vollem Leben gewaehrt {ComboOnFullHealthHit} Combo."), {{TEXT("ComboOnFullHealthHit"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Schnelle Klinge"), TEXT("Der erste normale Angriff pro Zug kostet {FirstBasicAttackCostReduction} weniger Initiative."), {{TEXT("FirstBasicAttackCostReduction"), 10.0}})
        }),
        MakeCharacterTier(TEXT("Bewegung"), {
            MakeCharacterSkillpoint(TEXT("Schattenlauf"), TEXT("Nach {MoveDistanceForBonuses} bewegten Feldern verursacht der naechste Treffer {NextHitDamageMultiplierAfterMove}x Schaden."), {{TEXT("MoveDistanceForBonuses"), 3.0}, {TEXT("NextHitDamageMultiplierAfterMove"), 1.15}}),
            MakeCharacterSkillpoint(TEXT("Gleitender Rueckzug"), TEXT("Nach verursachtem Schaden ist einmal pro Zug eine angrenzende Bewegung kostenlos."), {{TEXT("FreeAdjacentMoveAfterDamage"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Ausweichbewegung"), TEXT("Nach {MoveDistanceForBonuses} bewegten Feldern verursacht der naechste eingehende Treffer nur {NextIncomingDamageMultiplierAfterMove}x Schaden."), {{TEXT("MoveDistanceForBonuses"), 3.0}, {TEXT("NextIncomingDamageMultiplierAfterMove"), 0.75}})
        }),
        MakeCharacterTier(TEXT("Combo"), {
            MakeCharacterSkillpoint(TEXT("Ueberlauf"), TEXT("Combo-Gewinn bei voller Combo gewaehrt einmal pro Zug {ComboOverflowInitiativeGain} Initiative."), {{TEXT("ComboOverflowInitiativeGain"), 15.0}}),
            MakeCharacterSkillpoint(TEXT("Gedraengter Rhythmus"), TEXT("Wenn Character 3 Combo verbraucht, erhaelt er {ComboSpendInitiativeGainPerPoint} Initiative pro verbrauchtem Combo."), {{TEXT("ComboSpendInitiativeGainPerPoint"), 5.0}}),
            MakeCharacterSkillpoint(TEXT("Reserveklinge"), TEXT("Beginnt Character 3 seinen Zug ohne Combo, erhaelt er {ComboOnEmptyTurnStart} Combo."), {{TEXT("ComboOnEmptyTurnStart"), 1.0}})
        }),
        MakeCharacterTier(TEXT("Assassine"), {
            MakeCharacterSkillpoint(TEXT("Spurlos"), TEXT("Nach einem Kill wird Character 3 fuer {InvisibilityOnKillDurationInitiative} Initiative unsichtbar. Verursachter Schaden beendet die Unsichtbarkeit."), {{TEXT("InvisibilityOnKillDurationInitiative"), 40.0}}),
            MakeCharacterSkillpoint(TEXT("Kein Leerlauf"), TEXT("Jeder Kill gewaehrt {ComboOnKill} Combo."), {{TEXT("ComboOnKill"), 2.0}}),
            MakeCharacterSkillpoint(TEXT("Gejagte Beute"), TEXT("Der erste getroffene ueberlebende Gegner erleidet fuer {HuntMarkDurationInitiative} Initiative {HuntMarkDamageMultiplier}x Schaden von Character 3. Danach {HuntMarkCooldownAfterExpiryInitiative} Initiative Cooldown."), {{TEXT("HuntMarkDamageMultiplier"), 1.15}, {TEXT("HuntMarkDurationInitiative"), 80.0}, {TEXT("HuntMarkCooldownAfterExpiryInitiative"), 80.0}})
        })
    };
    Character3Progression->MarkPackageDirty();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter CharacterFilter;
    CharacterFilter.ClassPaths.Add(URoundCharacterData::StaticClass()->GetClassPathName());
    CharacterFilter.PackagePaths.Add(*CharacterDataDirectory);
    CharacterFilter.bRecursivePaths = true;
    CharacterFilter.bRecursiveClasses = true;

    TArray<FAssetData> CharacterAssets;
    AssetRegistryModule.Get().GetAssets(CharacterFilter, CharacterAssets);
    for (const FAssetData& AssetData : CharacterAssets)
    {
        if (URoundCharacterData* CharacterData = Cast<URoundCharacterData>(AssetData.GetAsset()))
        {
            const FString CharacterKey = CharacterCsvNormalizeCharacterKey(!CharacterData->CharacterId.IsNone() ? CharacterData->CharacterId.ToString() : CharacterData->GetName());
            if (CharacterKey == CharacterCsvNormalizeCharacterKey(TEXT("Character3")))
            {
                CharacterData->CharacterProgression = Character3Progression;
                CharacterData->MarkPackageDirty();
                break;
            }
        }
    }

    MarkPackageDirty();
    UE_LOG(LogRoundCharacterRegistryImport, Log, TEXT("PopulateCharacter3ProgressionDefaults wrote defaults into %s"), *GetNameSafe(Character3Progression));
}

void URoundCharacterRegistryData::PopulateCharacter4ProgressionDefaults()
{
    const TMap<FString, TObjectPtr<URoundCharacterProgressionData>> ProgressionAssets = BuildCharacterProgressionAssetMap(CharacterProgressionDirectory);
    const FString Character4Key = CharacterCsvNormalizeCharacterKey(TEXT("Character4"));

    URoundCharacterProgressionData* Character4Progression = nullptr;
    if (const TObjectPtr<URoundCharacterProgressionData>* FoundProgression = ProgressionAssets.Find(Character4Key))
    {
        Character4Progression = FoundProgression->Get();
    }
    else if (const TObjectPtr<URoundCharacterProgressionData>* FoundByAssetName = ProgressionAssets.Find(CharacterCsvNormalizeCharacterKey(TEXT("Character4Progression"))))
    {
        Character4Progression = FoundByAssetName->Get();
    }
    else
    {
        for (const TPair<FString, TObjectPtr<URoundCharacterProgressionData>>& Pair : ProgressionAssets)
        {
            if (Pair.Key.Contains(Character4Key))
            {
                Character4Progression = Pair.Value.Get();
                break;
            }
        }
    }

    if (!Character4Progression)
    {
        Character4Progression = LoadObject<URoundCharacterProgressionData>(
            nullptr,
            TEXT("/Game/TopDown/Characters/Character_4/DA_Character4Progression.DA_Character4Progression"));
    }

    if (!Character4Progression)
    {
        UE_LOG(LogRoundCharacterRegistryImport, Error, TEXT("PopulateCharacter4ProgressionDefaults failed - no RoundCharacterProgressionData asset found for Character4 in %s"), *CharacterProgressionDirectory);
        return;
    }

    Character4Progression->CharacterId = TEXT("Character4");
    Character4Progression->DisplayName = FText::FromString(TEXT("Mage"));
    Character4Progression->Description = FText::FromString(TEXT("Character-only upgrades for Character 4."));
    Character4Progression->Tiers = {
        MakeCharacterTier(TEXT("Elementare Grundlage"), {
            MakeCharacterSkillpoint(TEXT("Leichter Zauberfluss"), TEXT("Der erste Skill in jedem eigenen Zug, der ein gespeichertes Element nutzt, kostet {FirstElementUseCostReduction} weniger Initiative."), {{TEXT("FirstElementUseCostReduction"), 5.0}}),
            MakeCharacterSkillpoint(TEXT("Gehaltene Energie"), TEXT("Solange ein Element gespeichert ist, erhaelt Character 4 +{DefenseWhileElementStored} Defense."), {{TEXT("DefenseWhileElementStored"), 15.0}}),
            MakeCharacterSkillpoint(TEXT("Beweglicher Magier"), TEXT("Verringert MoveCost um {-MoveCostAdd}."), {{TEXT("MoveCostAdd"), -2.0}})
        }),
        MakeCharacterTier(TEXT("Elementfuehrung"), {
            MakeCharacterSkillpoint(TEXT("Elementwechsel"), TEXT("Nutzt Character 4 einen Skill mit einem anderen Element als dem gespeicherten, erhaelt er {DifferentElementInitiativeRefund} Initiative zurueck. Einmal pro eigenem Zug."), {{TEXT("DifferentElementInitiativeRefund"), 8.0}}),
            MakeCharacterSkillpoint(TEXT("Resonanz"), TEXT("Entspricht das gespeicherte Element dem Element des genutzten Skills, ist sein Elementeffekt {SameElementEffectMultiplier}x so stark."), {{TEXT("SameElementEffectMultiplier"), 1.5}}),
            MakeCharacterSkillpoint(TEXT("Neuer Anfang"), TEXT("Wird ein Skill ohne gespeichertes Element genutzt, wird sein aktueller Cooldown danach um {CooldownReductionWithoutElement} Initiative reduziert."), {{TEXT("CooldownReductionWithoutElement"), 40.0}})
        }),
        MakeCharacterTier(TEXT("Arkane Verkettung"), {
            MakeCharacterSkillpoint(TEXT("Arkaner Leiter"), TEXT("Arcane Shift entfernt das gespeicherte Element nicht mehr. Der Elementeffekt gilt fuer Arcane Shift und bleibt fuer den folgenden Skill erhalten."), {{TEXT("ArcaneShiftPreservesElement"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Elementschutz"), TEXT("Nachdem ein gespeichertes Element genutzt wurde, verursacht der naechste eingehende Treffer innerhalb von {ElementProtectionDurationInitiative} Initiative nur {ElementProtectionIncomingDamageMultiplier}x Schaden."), {{TEXT("ElementProtectionIncomingDamageMultiplier"), 0.8}, {TEXT("ElementProtectionDurationInitiative"), 80.0}}),
            MakeCharacterSkillpoint(TEXT("Kraft sammeln"), TEXT("Trifft ein Skill mindestens 2 Gegner, ist der Effekt des neu gespeicherten Elements bei seiner naechsten Nutzung {EmpoweredStoredElementMultiplier}x so stark."), {{TEXT("EmpowerStoredElementOnMultiHit"), 1.0}, {TEXT("EmpoweredStoredElementMultiplier"), 1.25}})
        }),
        MakeCharacterTier(TEXT("Elementarmeister"), {
            MakeCharacterSkillpoint(TEXT("Elementares Echo"), TEXT("Ein genutztes Element wirkt auch auf den unmittelbar folgenden Skill. Dieser kann dadurch den alten und den neu gespeicherten Elementeffekt zugleich erhalten."), {{TEXT("ElementEcho"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Vollstaendiger Kreislauf"), TEXT("Werden Feuer, Eis, Blitz und Stein ohne Wiederholung direkt nacheinander gewirkt, erhaelt der vierte Skill alle vier Elementeffekte. Danach beginnt der Kreislauf neu."), {{TEXT("AllElementsOnCompletedCycle"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Elementarmeisterschaft"), TEXT("Alle Elementeffekte von Character 4 sind dauerhaft {ElementMasteryMultiplier}x so stark."), {{TEXT("ElementMasteryMultiplier"), 1.25}})
        })
    };
    Character4Progression->MarkPackageDirty();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter CharacterFilter;
    CharacterFilter.ClassPaths.Add(URoundCharacterData::StaticClass()->GetClassPathName());
    CharacterFilter.PackagePaths.Add(*CharacterDataDirectory);
    CharacterFilter.bRecursivePaths = true;
    CharacterFilter.bRecursiveClasses = true;

    TArray<FAssetData> CharacterAssets;
    AssetRegistryModule.Get().GetAssets(CharacterFilter, CharacterAssets);
    for (const FAssetData& AssetData : CharacterAssets)
    {
        if (URoundCharacterData* CharacterData = Cast<URoundCharacterData>(AssetData.GetAsset()))
        {
            const FString CharacterKey = CharacterCsvNormalizeCharacterKey(!CharacterData->CharacterId.IsNone() ? CharacterData->CharacterId.ToString() : CharacterData->GetName());
            if (CharacterKey == CharacterCsvNormalizeCharacterKey(TEXT("Character4")))
            {
                CharacterData->CharacterProgression = Character4Progression;
                CharacterData->MarkPackageDirty();
                break;
            }
        }
    }

    MarkPackageDirty();
    UE_LOG(LogRoundCharacterRegistryImport, Log, TEXT("PopulateCharacter4ProgressionDefaults wrote defaults into %s"), *GetNameSafe(Character4Progression));
}

void URoundCharacterRegistryData::PopulateCharacter5ProgressionDefaults()
{
    const TMap<FString, TObjectPtr<URoundCharacterProgressionData>> ProgressionAssets = BuildCharacterProgressionAssetMap(CharacterProgressionDirectory);
    const FString Character5Key = CharacterCsvNormalizeCharacterKey(TEXT("Character5"));

    URoundCharacterProgressionData* Character5Progression = nullptr;
    if (const TObjectPtr<URoundCharacterProgressionData>* FoundProgression = ProgressionAssets.Find(Character5Key))
    {
        Character5Progression = FoundProgression->Get();
    }
    else if (const TObjectPtr<URoundCharacterProgressionData>* FoundByAssetName = ProgressionAssets.Find(CharacterCsvNormalizeCharacterKey(TEXT("Character5Progression"))))
    {
        Character5Progression = FoundByAssetName->Get();
    }
    else
    {
        for (const TPair<FString, TObjectPtr<URoundCharacterProgressionData>>& Pair : ProgressionAssets)
        {
            if (Pair.Key.Contains(Character5Key))
            {
                Character5Progression = Pair.Value.Get();
                break;
            }
        }
    }

    if (!Character5Progression)
    {
        Character5Progression = LoadObject<URoundCharacterProgressionData>(
            nullptr,
            TEXT("/Game/TopDown/Characters/Character_5/DA_Character5Progression.DA_Character5Progression"));
    }

    if (!Character5Progression)
    {
        UE_LOG(LogRoundCharacterRegistryImport, Error, TEXT("PopulateCharacter5ProgressionDefaults failed - no RoundCharacterProgressionData asset found for Character5 in %s"), *CharacterProgressionDirectory);
        return;
    }

    Character5Progression->CharacterId = TEXT("Character5");
    Character5Progression->DisplayName = FText::FromString(TEXT("Fighter"));
    Character5Progression->Description = FText::FromString(TEXT("Blut erzeugen, speichern oder fuer offensiven Druck und Ueberleben einsetzen."));
    Character5Progression->Tiers = {
        MakeCharacterTier(TEXT("Blutkreislauf"), {
            MakeCharacterSkillpoint(TEXT("Frisches Blut"), TEXT("Character 5 erhaelt {BloodGainMultiplier}x so viel Blut aus verursachtem Schaden."), {{TEXT("BloodGainMultiplier"), 1.25}}),
            MakeCharacterSkillpoint(TEXT("Tiefe Reserven"), TEXT("Erhoeht das maximale Blut um {BloodCapAdd}."), {{TEXT("BloodCapAdd"), 25.0}}),
            MakeCharacterSkillpoint(TEXT("Jagdschritt"), TEXT("Verringert MoveCost um {-MoveCostAdd}."), {{TEXT("MoveCostAdd"), -2.0}})
        }),
        MakeCharacterTier(TEXT("Kampfmodus"), {
            MakeCharacterSkillpoint(TEXT("Gegenschmerz"), TEXT("Character 5 erhaelt Blut in Hoehe von {BloodGainFractionFromDamageTaken}x des tatsaechlich erlittenen Schadens."), {{TEXT("BloodGainFractionFromDamageTaken"), 0.05}}),
            MakeCharacterSkillpoint(TEXT("Geronnene Ruestung"), TEXT("Solange Character 5 mindestens {BloodDefenseThreshold} Blut besitzt, erhaelt er {DefenseWhileHighBlood} Defense."), {{TEXT("BloodDefenseThreshold"), 50.0}, {TEXT("DefenseWhileHighBlood"), 20.0}}),
            MakeCharacterSkillpoint(TEXT("Erster Schnitt"), TEXT("Der erste normale Angriff jedes eigenen Zuges, der Schaden verursacht, erzeugt zusaetzlich {FirstBasicAttackBloodGain} Blut."), {{TEXT("FirstBasicAttackBloodGain"), 8.0}})
        }),
        MakeCharacterTier(TEXT("Blutentscheidung"), {
            MakeCharacterSkillpoint(TEXT("Adrenalinstoss"), TEXT("Verbraucht Heal mindestens {BloodSpendThresholdForInitiative} Blut, erhaelt Character 5 danach {InitiativeGainOnLargeBloodSpend} Initiative."), {{TEXT("BloodSpendThresholdForInitiative"), 50.0}, {TEXT("InitiativeGainOnLargeBloodSpend"), 15.0}}),
            MakeCharacterSkillpoint(TEXT("Wundwut"), TEXT("Solange Character 5 unter {LowHealthThresholdPercent}% Leben hat, verursacht er {DamageMultiplierWhileLowHealth}x Schaden."), {{TEXT("LowHealthThresholdPercent"), 35.0}, {TEXT("DamageMultiplierWhileLowHealth"), 1.15}}),
            MakeCharacterSkillpoint(TEXT("Blutueberschuss"), TEXT("Erreicht Character 5 erstmals seit seinem letzten Zug das Blutmaximum, sinken die aktuellen Cooldowns aller Skills um {CooldownReductionOnBloodCap} Initiative."), {{TEXT("CooldownReductionOnBloodCap"), 20.0}})
        }),
        MakeCharacterTier(TEXT("Berserker-Ausprägung"), {
            MakeCharacterSkillpoint(TEXT("Blutpakt"), TEXT("Wuerde Character 5 sterben und besitzt mindestens {BloodCheatDeathCost} Blut, verbraucht er es und ueberlebt mit {BloodCheatDeathHeal} Leben. Cooldown: {BloodCheatDeathCooldownInitiative} Initiative."), {{TEXT("BloodCheatDeathCost"), 75.0}, {TEXT("BloodCheatDeathHeal"), 200.0}, {TEXT("BloodCheatDeathCooldownInitiative"), 160.0}}),
            MakeCharacterSkillpoint(TEXT("Ueberlauf"), TEXT("Blut, das wegen des Blutmaximums nicht gespeichert werden kann, heilt Character 5 zu {BloodOverflowHealFraction}x des Ueberschusses."), {{TEXT("BloodOverflowHealFraction"), 0.5}}),
            MakeCharacterSkillpoint(TEXT("Blutrausch"), TEXT("Solange Character 5 mindestens {HighBloodDamageThreshold} Blut besitzt, verursacht er {HighBloodDamageMultiplier}x Schaden."), {{TEXT("HighBloodDamageThreshold"), 75.0}, {TEXT("HighBloodDamageMultiplier"), 1.2}})
        })
    };
    Character5Progression->MarkPackageDirty();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter CharacterFilter;
    CharacterFilter.ClassPaths.Add(URoundCharacterData::StaticClass()->GetClassPathName());
    CharacterFilter.PackagePaths.Add(*CharacterDataDirectory);
    CharacterFilter.bRecursivePaths = true;
    CharacterFilter.bRecursiveClasses = true;

    TArray<FAssetData> CharacterAssets;
    AssetRegistryModule.Get().GetAssets(CharacterFilter, CharacterAssets);
    for (const FAssetData& AssetData : CharacterAssets)
    {
        if (URoundCharacterData* CharacterData = Cast<URoundCharacterData>(AssetData.GetAsset()))
        {
            const FString CharacterKey = CharacterCsvNormalizeCharacterKey(!CharacterData->CharacterId.IsNone() ? CharacterData->CharacterId.ToString() : CharacterData->GetName());
            if (CharacterKey == CharacterCsvNormalizeCharacterKey(TEXT("Character5")))
            {
                CharacterData->Details.Add(TEXT("BloodCap"), 100.0);
                CharacterData->Details.Add(TEXT("BloodGainFraction"), 0.1);
                CharacterData->CharacterProgression = Character5Progression;
                CharacterData->MarkPackageDirty();
                break;
            }
        }
    }

    MarkPackageDirty();
    UE_LOG(LogRoundCharacterRegistryImport, Log, TEXT("PopulateCharacter5ProgressionDefaults wrote defaults into %s"), *GetNameSafe(Character5Progression));
}

void URoundCharacterRegistryData::PopulateCharacter6ProgressionDefaults()
{
    const TMap<FString, TObjectPtr<URoundCharacterProgressionData>> ProgressionAssets = BuildCharacterProgressionAssetMap(CharacterProgressionDirectory);
    const FString Character6Key = CharacterCsvNormalizeCharacterKey(TEXT("Character6"));

    URoundCharacterProgressionData* Character6Progression = nullptr;
    if (const TObjectPtr<URoundCharacterProgressionData>* FoundProgression = ProgressionAssets.Find(Character6Key))
    {
        Character6Progression = FoundProgression->Get();
    }
    else if (const TObjectPtr<URoundCharacterProgressionData>* FoundByAssetName = ProgressionAssets.Find(CharacterCsvNormalizeCharacterKey(TEXT("Character6Progression"))))
    {
        Character6Progression = FoundByAssetName->Get();
    }
    else
    {
        for (const TPair<FString, TObjectPtr<URoundCharacterProgressionData>>& Pair : ProgressionAssets)
        {
            if (Pair.Key.Contains(Character6Key))
            {
                Character6Progression = Pair.Value.Get();
                break;
            }
        }
    }

    if (!Character6Progression)
    {
        Character6Progression = LoadObject<URoundCharacterProgressionData>(
            nullptr,
            TEXT("/Game/TopDown/Characters/Character_6/DA_Character6Progression.DA_Character6Progression"));
    }

    if (!Character6Progression)
    {
        UE_LOG(LogRoundCharacterRegistryImport, Error, TEXT("PopulateCharacter6ProgressionDefaults failed - no RoundCharacterProgressionData asset found for Character6 in %s"), *CharacterProgressionDirectory);
        return;
    }

    Character6Progression->CharacterId = TEXT("Character6");
    Character6Progression->DisplayName = FText::FromString(TEXT("Puppeteer"));
    Character6Progression->Description = FText::FromString(TEXT("Character-only upgrades for Character 6."));
    Character6Progression->Tiers = {
        MakeCharacterTier(TEXT("Puppenrolle"), {
            MakeCharacterSkillpoint(TEXT("Frontpuppe"), TEXT("Erhoeht MaxHealth der Puppe um {PuppetRoleMaxHealthAdd}, Defense um {PuppetRoleDefenseAdd}, verringert aber Attack um {-PuppetRoleAttackAdd}."), {{TEXT("PuppetRoleMaxHealthAdd"), 500.0}, {TEXT("PuppetRoleDefenseAdd"), 30.0}, {TEXT("PuppetRoleAttackAdd"), -20.0}}),
            MakeCharacterSkillpoint(TEXT("Kampfpuppe"), TEXT("Erhoeht Attack der Puppe um {PuppetRoleAttackAdd}, Defense um {PuppetRoleDefenseAdd} und verringert MoveCost um {-PuppetMoveCostAdd}."), {{TEXT("PuppetRoleAttackAdd"), 25.0}, {TEXT("PuppetRoleDefenseAdd"), 15.0}, {TEXT("PuppetMoveCostAdd"), -2.0}}),
            MakeCharacterSkillpoint(TEXT("Jagdpupe"), TEXT("Erhoeht Attack der Puppe um {PuppetRoleAttackAdd} und Range um {PuppetRangeAdd}, verringert aber MaxHealth um {-PuppetRoleMaxHealthAdd} und Defense um {-PuppetRoleDefenseAdd}."), {{TEXT("PuppetRoleAttackAdd"), 20.0}, {TEXT("PuppetRangeAdd"), 3.0}, {TEXT("PuppetRoleMaxHealthAdd"), -300.0}, {TEXT("PuppetRoleDefenseAdd"), -15.0}})
        }),
        MakeCharacterTier(TEXT("Kontrolle"), {
            MakeCharacterSkillpoint(TEXT("Feine Faeden"), TEXT("Erhoeht PuppetControlRange um {PuppetControlRangeAdd}."), {{TEXT("PuppetControlRangeAdd"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Leichter Zug"), TEXT("Verringert die Initiativekosten von PuppetMove um {PuppetMoveCostReduction}."), {{TEXT("PuppetMoveCostReduction"), 5.0}}),
            MakeCharacterSkillpoint(TEXT("Sicherer Abstand"), TEXT("Solange Character 6 nicht neben einem Gegner steht und die Puppe in PuppetControlRange ist, erleidet Character 6 nur {SafeDistanceDamageMultiplier}x Schaden."), {{TEXT("SafeDistanceDamageMultiplier"), 0.85}})
        }),
        MakeCharacterTier(TEXT("Koordination"), {
            MakeCharacterSkillpoint(TEXT("Doppeldruck"), TEXT("Wenn Character 6 und die Puppe dasselbe Ziel angreifen koennten, verursachen sie {DamageMultiplierWhenBothThreatenSameTarget}x Schaden gegen dieses Ziel."), {{TEXT("DamageMultiplierWhenBothThreatenSameTarget"), 1.15}}),
            MakeCharacterSkillpoint(TEXT("Wechselspiel"), TEXT("Wenn die Puppe in diesem Zug bewegt wurde, verursacht der naechste Puppet-Skill {PuppetSkillDamageMultiplierAfterMove}x Schaden."), {{TEXT("PuppetSkillDamageMultiplierAfterMove"), 1.15}}),
            MakeCharacterSkillpoint(TEXT("Abgeschnittener Weg"), TEXT("Wenn ein Gegner in der Naehe der Puppe Schaden erleidet, verliert er {InitiativeLossNearPuppetOnHit} Initiative. Cooldown: {InitiativeLossNearPuppetCooldownInitiative} Initiative pro Ziel."), {{TEXT("InitiativeLossNearPuppetOnHit"), 10.0}, {TEXT("InitiativeLossNearPuppetCooldownInitiative"), 80.0}})
        }),
        MakeCharacterTier(TEXT("Anpassung"), {
            MakeCharacterSkillpoint(TEXT("Umlenkung"), TEXT("Solange die Puppe naeher am naechsten Gegner steht als Character 6, erleidet Character 6 nur {DamageMultiplierWhilePuppetCloser}x Schaden."), {{TEXT("DamageMultiplierWhilePuppetCloser"), 0.85}}),
            MakeCharacterSkillpoint(TEXT("Gemeinsame Jagd"), TEXT("Wenn Character 6 oder die Puppe einem Ziel Schaden zufuegen, verursacht der jeweils andere diesem Ziel fuer den Rest des Zuges {SharedHuntDamageMultiplier}x Schaden. Dieser Effekt stapelt."), {{TEXT("SharedHuntDamageMultiplier"), 1.15}}),
            MakeCharacterSkillpoint(TEXT("Fadenzug"), TEXT("Wenn Character 6 oder die Puppe einem Ziel Schaden zufuegen, verliert dieses Ziel {SharedHitInitiativeLoss} Initiative. Cooldown: {SharedHitInitiativeLossCooldownInitiative} Initiative pro Ziel."), {{TEXT("SharedHitInitiativeLoss"), 10.0}, {TEXT("SharedHitInitiativeLossCooldownInitiative"), 40.0}})
        })
    };
    Character6Progression->MarkPackageDirty();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter CharacterFilter;
    CharacterFilter.ClassPaths.Add(URoundCharacterData::StaticClass()->GetClassPathName());
    CharacterFilter.PackagePaths.Add(*CharacterDataDirectory);
    CharacterFilter.bRecursivePaths = true;
    CharacterFilter.bRecursiveClasses = true;

    TArray<FAssetData> CharacterAssets;
    AssetRegistryModule.Get().GetAssets(CharacterFilter, CharacterAssets);
    for (const FAssetData& AssetData : CharacterAssets)
    {
        if (URoundCharacterData* CharacterData = Cast<URoundCharacterData>(AssetData.GetAsset()))
        {
            const FString CharacterKey = CharacterCsvNormalizeCharacterKey(!CharacterData->CharacterId.IsNone() ? CharacterData->CharacterId.ToString() : CharacterData->GetName());
            if (CharacterKey == CharacterCsvNormalizeCharacterKey(TEXT("Character6")))
            {
                CharacterData->CharacterProgression = Character6Progression;
                CharacterData->MarkPackageDirty();
                break;
            }
        }
    }

    MarkPackageDirty();
    UE_LOG(LogRoundCharacterRegistryImport, Log, TEXT("PopulateCharacter6ProgressionDefaults wrote defaults into %s"), *GetNameSafe(Character6Progression));
}

void URoundCharacterRegistryData::PopulateEngineerProgressionDefaults()
{
#if WITH_EDITOR
    URoundCharacterProgressionData* Progression = LoadObject<URoundCharacterProgressionData>(nullptr, TEXT("/Game/TopDown/Characters/Engineer/DA_EngineerProgression.DA_EngineerProgression"));
    URoundCharacterData* Character = LoadObject<URoundCharacterData>(nullptr, TEXT("/Game/TopDown/Characters/Engineer/EngineerDetails.EngineerDetails"));
    if (!Progression || !Character)
    {
        UE_LOG(LogRoundCharacterRegistryImport, Error, TEXT("PopulateEngineerProgressionDefaults failed - assets missing"));
        return;
    }

    Progression->Modify();
    Progression->CharacterId = TEXT("Engineer");
    Progression->DisplayName = FText::FromString(TEXT("Engineer"));
    Progression->Description = FText::FromString(TEXT("Spezialisiert das Geraetenetz in Reichweite, Haltbarkeit, Tempo oder maximale Feldkontrolle."));
    Progression->Tiers = {
        MakeCharacterTier(TEXT("Werkzeugkasten"), {
            MakeCharacterSkillpoint(TEXT("Zusaetzlicher Slot"), TEXT("Erhoeht das Geraetelimit um {MaxDevicesAdd}."), {{TEXT("MaxDevicesAdd"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Verstaerkte Gehaeuse"), TEXT("Alle neu gebauten Geraete besitzen 25% mehr Leben."), {{TEXT("DeviceHealthMultiplierAdd"), 0.25}}),
            MakeCharacterSkillpoint(TEXT("Fernmontage"), TEXT("Erhoeht die Platzierungsreichweite aller Geraete um {DevicePlacementRangeAdd}."), {{TEXT("DevicePlacementRangeAdd"), 1.0}})
        }),
        MakeCharacterTier(TEXT("Feldwartung"), {
            MakeCharacterSkillpoint(TEXT("Reparaturroutine"), TEXT("Zu Beginn jedes eigenen Zuges repariert Character 7 jedes aktive Geraet um {DeviceRepairOnTurnStart} Leben."), {{TEXT("DeviceRepairOnTurnStart"), 30.0}}),
            MakeCharacterSkillpoint(TEXT("Rueckgewinnung"), TEXT("Wird beim Platzieren das aelteste Geraet ersetzt, erhaelt Character 7 {InitiativeRefundOnDeviceReplace} Initiative zurueck."), {{TEXT("InitiativeRefundOnDeviceReplace"), 15.0}}),
            MakeCharacterSkillpoint(TEXT("Deckungsnetz"), TEXT("Innerhalb von {DeviceNetworkDefenseRange} Feldern um ein eigenes Geraet erhaelt Character 7 {DefenseNearDevice} Defense."), {{TEXT("DeviceNetworkDefenseRange"), 2.0}, {TEXT("DefenseNearDevice"), 20.0}})
        }),
        MakeCharacterTier(TEXT("Netzwerk"), {
            MakeCharacterSkillpoint(TEXT("Langlebige Bauteile"), TEXT("Neu gebaute Geraete bleiben 30% laenger bestehen."), {{TEXT("DeviceLifetimeMultiplierAdd"), 0.3}}),
            MakeCharacterSkillpoint(TEXT("Verteilte Panzerung"), TEXT("Character 7 erhaelt pro aktivem Geraet {DefensePerActiveDevice} Defense, maximal {MaxDefenseFromDevices}."), {{TEXT("DefensePerActiveDevice"), 5.0}, {TEXT("MaxDefenseFromDevices"), 15.0}}),
            MakeCharacterSkillpoint(TEXT("Schnelle Schaltkreise"), TEXT("Periodische Effekte neu gebauter Geraete aktivieren sich 15% schneller."), {{TEXT("DeviceTriggerIntervalReduction"), 0.15}})
        }),
        MakeCharacterTier(TEXT("Meisterkonstruktion"), {
            MakeCharacterSkillpoint(TEXT("Geraeteschwarm"), TEXT("Erhoeht das Geraetelimit um weitere {MaxDevicesAdd}."), {{TEXT("MaxDevicesAdd"), 2.0}}),
            MakeCharacterSkillpoint(TEXT("Militaerstandard"), TEXT("Neu gebaute Geraete besitzen 50% mehr Leben und bleiben 50% laenger bestehen."), {{TEXT("DeviceHealthMultiplierAdd"), 0.5}, {TEXT("DeviceLifetimeMultiplierAdd"), 0.5}}),
            MakeCharacterSkillpoint(TEXT("Uebertaktetes Netz"), TEXT("Periodische Effekte neu gebauter Geraete aktivieren sich 30% schneller."), {{TEXT("DeviceTriggerIntervalReduction"), 0.3}})
        })
    };
    Progression->MarkPackageDirty();
    Character->Modify(); Character->CharacterProgression = Progression; Character->MarkPackageDirty();
    MarkPackageDirty();
#endif
}

void URoundCharacterRegistryData::PopulateChronomancerProgressionDefaults()
{
#if WITH_EDITOR
    URoundCharacterProgressionData* Progression = LoadObject<URoundCharacterProgressionData>(nullptr, TEXT("/Game/TopDown/Characters/Chronomancer/DA_ChronomancerProgression.DA_ChronomancerProgression"));
    URoundCharacterData* Character = LoadObject<URoundCharacterData>(nullptr, TEXT("/Game/TopDown/Characters/Chronomancer/ChronomancerDetails.ChronomancerDetails"));
    if (!Progression || !Character) return;
    Progression->Modify(); Progression->CharacterId = TEXT("Chronomancer"); Progression->DisplayName = FText::FromString(TEXT("Chronomancer"));
    Progression->Description = FText::FromString(TEXT("Bestimmt, ob Fragmente als verlaessliche Ressource, als Temporeserve oder als Schutzkreislauf genutzt werden."));
    Progression->Tiers = {
        MakeCharacterTier(TEXT("Zeitvorrat"), {
            MakeCharacterSkillpoint(TEXT("Groesseres Paradox"), TEXT("Erhoeht das Fragmentmaximum um {MaxFragments}."), {{TEXT("MaxFragments"), 2.0}}),
            MakeCharacterSkillpoint(TEXT("Leichter Schritt"), TEXT("Verringert die Bewegungskosten um {-MoveCostAdd}."), {{TEXT("MoveCostAdd"), -1.0}}),
            MakeCharacterSkillpoint(TEXT("Zeitreserve"), TEXT("Erhoeht die maximale Initiative um {MaxInitiativeAdd}."), {{TEXT("MaxInitiativeAdd"), 15.0}})
        }),
        MakeCharacterTier(TEXT("Kreislauf"), {
            MakeCharacterSkillpoint(TEXT("Neuer Anfang"), TEXT("Beginnt Character 8 einen Zug ohne Fragmente, erhaelt er {FragmentGainAtTurnStartIfEmpty}."), {{TEXT("FragmentGainAtTurnStartIfEmpty"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Ueberlauf"), TEXT("Jedes Fragment oberhalb des Maximums heilt Character 8 um {HealPerOverflowFragment}."), {{TEXT("HealPerOverflowFragment"), 70.0}}),
            MakeCharacterSkillpoint(TEXT("Zeitgewinn"), TEXT("Das erste Ausgeben von Fragmenten pro Zug gibt {InitiativeGainOnFirstFragmentSpend} Initiative zurueck."), {{TEXT("InitiativeGainOnFirstFragmentSpend"), 12.0}})
        }),
        MakeCharacterTier(TEXT("Rueckkopplung"), {
            MakeCharacterSkillpoint(TEXT("Geschlossener Kreis"), TEXT("Pro ausgegebenem Fragment sinkt der aktuelle Cooldown von Time Siphon um {TimeSiphonCooldownReductionPerFragmentSpent} Initiative."), {{TEXT("TimeSiphonCooldownReductionPerFragmentSpent"), 18.0}}),
            MakeCharacterSkillpoint(TEXT("Volle Stunde"), TEXT("Beginnt Character 8 einen Zug mit maximalen Fragmenten, erhaelt er {InitiativeGainAtTurnStartIfFull} Initiative."), {{TEXT("InitiativeGainAtTurnStartIfFull"), 18.0}}),
            MakeCharacterSkillpoint(TEXT("Zeit heilt"), TEXT("Jedes ausgegebene Fragment heilt Character 8 um {HealPerFragmentSpent}."), {{TEXT("HealPerFragmentSpent"), 45.0}})
        }),
        MakeCharacterTier(TEXT("Paradoxon"), {
            MakeCharacterSkillpoint(TEXT("Freier Augenblick"), TEXT("Der erste fragmentverbrauchende Skill pro Zug kostet {FirstFragmentSpendDiscount} Fragment weniger."), {{TEXT("FirstFragmentSpendDiscount"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Kaskadenbruch"), TEXT("Jedes ueberzaehlige Fragment reduziert die aktuellen Cooldowns aller Skills um {AllCooldownReductionPerOverflowFragment} Initiative."), {{TEXT("AllCooldownReductionPerOverflowFragment"), 25.0}}),
            MakeCharacterSkillpoint(TEXT("Unendliche Reserve"), TEXT("Erhoeht das Fragmentmaximum um {MaxFragments} und die maximale Initiative um {MaxInitiativeAdd}."), {{TEXT("MaxFragments"), 2.0}, {TEXT("MaxInitiativeAdd"), 20.0}})
        })
    };
    Progression->MarkPackageDirty(); Character->Modify(); Character->CharacterProgression = Progression; Character->MarkPackageDirty(); MarkPackageDirty();
#endif
}

void URoundCharacterRegistryData::PopulateDruidProgressionDefaults()
{
#if WITH_EDITOR
    URoundCharacterProgressionData* Progression = LoadObject<URoundCharacterProgressionData>(nullptr, TEXT("/Game/TopDown/Characters/Druid/DA_DruidProgression.DA_DruidProgression"));
    URoundCharacterData* Character = LoadObject<URoundCharacterData>(nullptr, TEXT("/Game/TopDown/Characters/Druid/DruidDetails.DruidDetails"));
    if (!Progression || !Character) return;
    Progression->Modify(); Progression->CharacterId = TEXT("Druid"); Progression->DisplayName = FText::FromString(TEXT("Druid"));
    Progression->Description = FText::FromString(TEXT("Spezialisiert den Druiden auf widerstandsfaehige Naturfelder, einen schnelleren Naturkreislauf oder den Wechsel in die Baerenform."));
    Progression->Tiers = {
        MakeCharacterTier(TEXT("Wurzeln schlagen"), {
            MakeCharacterSkillpoint(TEXT("Alte Rinde"), TEXT("Erhoeht die maximale Gesundheit um {MaxHealthAdd}."), {{TEXT("MaxHealthAdd"), 220.0}}),
            MakeCharacterSkillpoint(TEXT("Naturpfad"), TEXT("Auf Naturfeldern sinken die Bewegungskosten um {MoveCostReductionOnNature}."), {{TEXT("MoveCostReductionOnNature"), 2.0}}),
            MakeCharacterSkillpoint(TEXT("Dornhaut"), TEXT("Erhoeht die Verteidigung um {DefenseAdd}."), {{TEXT("DefenseAdd"), 18.0}})
        }),
        MakeCharacterTier(TEXT("Lebendiger Boden"), {
            MakeCharacterSkillpoint(TEXT("Morgentau"), TEXT("Zu Beginn des eigenen Zuges heilt ein Naturfeld unter dem Druiden {HealOnNatureTurnStart} Leben."), {{TEXT("HealOnNatureTurnStart"), 90.0}}),
            MakeCharacterSkillpoint(TEXT("Rueckenwind"), TEXT("Zu Beginn des eigenen Zuges gibt ein Naturfeld unter dem Druiden {InitiativeOnNatureTurnStart} Initiative."), {{TEXT("InitiativeOnNatureTurnStart"), 12.0}}),
            MakeCharacterSkillpoint(TEXT("Schutzdach"), TEXT("Auf Naturfeldern erleidet der Druide 12% weniger Schaden."), {{TEXT("IncomingDamageMultiplierAddOnNature"), -0.12}})
        }),
        MakeCharacterTier(TEXT("Ausbreitung"), {
            MakeCharacterSkillpoint(TEXT("Dichtes Geflecht"), TEXT("Erhoeht die maximale Gesundheit um {MaxHealthAdd} und die Verteidigung um {DefenseAdd}."), {{TEXT("MaxHealthAdd"), 180.0}, {TEXT("DefenseAdd"), 12.0}}),
            MakeCharacterSkillpoint(TEXT("Fliessendes Wachstum"), TEXT("Auf Naturfeldern sinken die Bewegungskosten um weitere {MoveCostReductionOnNature}."), {{TEXT("MoveCostReductionOnNature"), 2.0}}),
            MakeCharacterSkillpoint(TEXT("Beschleunigte Photosynthese"), TEXT("Zu Zugbeginn auf Naturboden heilt der Druide und erhaelt Initiative."), {{TEXT("HealOnNatureTurnStart"), 60.0}, {TEXT("InitiativeOnNatureTurnStart"), 8.0}})
        }),
        MakeCharacterTier(TEXT("Urnatur"), {
            MakeCharacterSkillpoint(TEXT("Unbeweglicher Stamm"), TEXT("Auf Naturfeldern erleidet der Druide 20% weniger Schaden und erhaelt {DefenseAdd} Verteidigung."), {{TEXT("IncomingDamageMultiplierAddOnNature"), -0.20}, {TEXT("DefenseAdd"), 15.0}}),
            MakeCharacterSkillpoint(TEXT("Lebensader"), TEXT("Zu Beginn des eigenen Zuges heilt ein Naturfeld {HealOnNatureTurnStart} Leben und gibt {InitiativeOnNatureTurnStart} Initiative."), {{TEXT("HealOnNatureTurnStart"), 140.0}, {TEXT("InitiativeOnNatureTurnStart"), 15.0}}),
            MakeCharacterSkillpoint(TEXT("Wandernder Wald"), TEXT("Verringert Bewegungskosten auf Naturfeldern um {MoveCostReductionOnNature} und erhoeht die maximale Initiative um {MaxInitiativeAdd}."), {{TEXT("MoveCostReductionOnNature"), 3.0}, {TEXT("MaxInitiativeAdd"), 20.0}})
        })
    };
    Progression->MarkPackageDirty(); Character->Modify(); Character->CharacterProgression = Progression; Character->MarkPackageDirty(); MarkPackageDirty();
#endif
}

void URoundCharacterRegistryData::PopulateDragoonProgressionDefaults()
{
#if WITH_EDITOR
    URoundCharacterProgressionData* Progression = LoadObject<URoundCharacterProgressionData>(nullptr, TEXT("/Game/TopDown/Characters/Dragoon/DA_DragoonProgression.DA_DragoonProgression"));
    URoundCharacterData* Character = LoadObject<URoundCharacterData>(nullptr, TEXT("/Game/TopDown/Characters/Dragoon/DragoonDetails.DragoonDetails"));
    if (!Progression || !Character) return;
    Progression->Modify(); Progression->CharacterId = TEXT("Dragoon"); Progression->DisplayName = FText::FromString(TEXT("Dragoon"));
    Progression->Description = FText::FromString(TEXT("Formt Momentum zu verlaesslicher Ausdauer, schneller Eskalation oder maximalem Angriffsdruck."));
    Progression->Tiers = {
        MakeCharacterTier(TEXT("Anlauf"), {
            MakeCharacterSkillpoint(TEXT("Laengerer Anlauf"), TEXT("Erhoeht das Momentummaximum um {MomentumMaxStacks}."), {{TEXT("MomentumMaxStacks"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Schwere Lanze"), TEXT("Erhoeht den Bonusschaden pro Momentum um {DamagePerStack}."), {{TEXT("DamagePerStack"), 8.0}}),
            MakeCharacterSkillpoint(TEXT("Leichte Ruestung"), TEXT("Verringert die Bewegungskosten um {-MoveCostAdd}."), {{TEXT("MoveCostAdd"), -1.0}})
        }),
        MakeCharacterTier(TEXT("Rhythmus"), {
            MakeCharacterSkillpoint(TEXT("Schwung halten"), TEXT("Trifft der Dragoon in seinem Zug keinen Gegner, behaelt er bis zu {MomentumRetainedOnMiss} Momentum."), {{TEXT("MomentumRetainedOnMiss"), 2.0}}),
            MakeCharacterSkillpoint(TEXT("Geschlossene Deckung"), TEXT("Jeder Momentum-Stack gewaehrt {DefensePerMomentum} Verteidigung."), {{TEXT("DefensePerMomentum"), 4.0}}),
            MakeCharacterSkillpoint(TEXT("Explosiver Start"), TEXT("Der erste Treffer jedes eigenen Zuges erzeugt {BonusMomentumOnFirstHit} zusaetzliches Momentum."), {{TEXT("BonusMomentumOnFirstHit"), 1.0}})
        }),
        MakeCharacterTier(TEXT("Hoehenflug"), {
            MakeCharacterSkillpoint(TEXT("Voller Takt"), TEXT("Beginnt der Dragoon einen Zug mit maximalem Momentum, erhaelt er {InitiativeGainAtMaxMomentumTurnStart} Initiative."), {{TEXT("InitiativeGainAtMaxMomentumTurnStart"), 18.0}}),
            MakeCharacterSkillpoint(TEXT("Durchschlag"), TEXT("Jeder Momentum-Stack ignoriert {DefenseIgnorePerMomentum} Verteidigung."), {{TEXT("DefenseIgnorePerMomentum"), 3.0}}),
            MakeCharacterSkillpoint(TEXT("Kampfesrausch"), TEXT("Jeder Treffer bei maximalem Momentum heilt den Dragoon um {HealOnHitAtMaxMomentum}."), {{TEXT("HealOnHitAtMaxMomentum"), 55.0}})
        }),
        MakeCharacterTier(TEXT("Unaufhaltsam"), {
            MakeCharacterSkillpoint(TEXT("Perfekter Schwung"), TEXT("Momentum geht am Ende eines Zuges ohne Treffer nicht mehr verloren."), {{TEXT("KeepAllMomentumOnMiss"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Grenzenloser Anlauf"), TEXT("Erhoeht das Momentummaximum um {MomentumMaxStacks} und den Schaden pro Stack um {DamagePerStack}."), {{TEXT("MomentumMaxStacks"), 2.0}, {TEXT("DamagePerStack"), 10.0}}),
            MakeCharacterSkillpoint(TEXT("Finale Geschwindigkeit"), TEXT("Bei maximalem Momentum verursacht der Dragoon 20% mehr Gesamtschaden."), {{TEXT("DamageMultiplierAddAtMaxMomentum"), 0.20}})
        })
    };
    Progression->MarkPackageDirty(); Character->Modify(); Character->CharacterProgression = Progression; Character->MarkPackageDirty(); MarkPackageDirty();
#endif
}

void URoundCharacterRegistryData::PopulateGeomancerProgressionDefaults()
{
#if WITH_EDITOR
    URoundCharacterProgressionData* Progression = LoadObject<URoundCharacterProgressionData>(nullptr, TEXT("/Game/TopDown/Characters/Geomancer/DA_GeomancerProgression.DA_GeomancerProgression"));
    URoundCharacterData* Character = LoadObject<URoundCharacterData>(nullptr, TEXT("/Game/TopDown/Characters/Geomancer/GeomancerDetails.GeomancerDetails"));
    if (!Progression || !Character) return;
    Progression->Modify(); Progression->CharacterId = TEXT("Geomancer"); Progression->DisplayName = FText::FromString(TEXT("Geomancer"));
    Progression->Description = FText::FromString(TEXT("Verbindet Frakturmarken, Felsaufbau und Standfestigkeit zu kontrollierter Gebietsherrschaft."));
    Progression->Tiers = {
        MakeCharacterTier(TEXT("Bruchlinie"), {
            MakeCharacterSkillpoint(TEXT("Tiefe Fraktur"), TEXT("Erhoeht den Bonusschaden markierter Standardangriffe um {MarkedAttackBonusDamage}."), {{TEXT("MarkedAttackBonusDamage"), 35.0}}),
            MakeCharacterSkillpoint(TEXT("Rasche Erosion"), TEXT("Verringert die zielgebundene Abklingzeit der passiven Fraktur um 15 Initiative."), {{TEXT("PassivePerTargetCooldownInitiative"), -15.0}}),
            MakeCharacterSkillpoint(TEXT("Fester Schritt"), TEXT("Verringert die Bewegungskosten um {-MoveCostAdd}."), {{TEXT("MoveCostAdd"), -2.0}})
        }),
        MakeCharacterTier(TEXT("Nachbeben"), {
            MakeCharacterSkillpoint(TEXT("Rueckstossenergie"), TEXT("Das Ausloesen einer Fraktur gibt {InitiativeGainOnMarkTrigger} Initiative."), {{TEXT("InitiativeGainOnMarkTrigger"), 14.0}}),
            MakeCharacterSkillpoint(TEXT("Mineralpanzer"), TEXT("Das Ausloesen einer Fraktur heilt {HealOnMarkTrigger} Leben."), {{TEXT("HealOnMarkTrigger"), 75.0}}),
            MakeCharacterSkillpoint(TEXT("Offener Bruch"), TEXT("Markierte Standardangriffe ignorieren {MarkedAttackDefenseIgnore} Verteidigung."), {{TEXT("MarkedAttackDefenseIgnore"), 25.0}})
        }),
        MakeCharacterTier(TEXT("Tektonik"), {
            MakeCharacterSkillpoint(TEXT("Resonanz"), TEXT("Das Ausloesen einer Fraktur reduziert alle aktuellen Faehigkeitscooldowns um {AbilityCooldownReductionOnMarkTrigger} Initiative."), {{TEXT("AbilityCooldownReductionOnMarkTrigger"), 22.0}}),
            MakeCharacterSkillpoint(TEXT("Felsstreuung"), TEXT("Das Ausloesen einer Fraktur erzeugt {AdditionalRocksOnMarkTrigger} zusaetzlichen Felsen neben dem Ziel."), {{TEXT("AdditionalRocksOnMarkTrigger"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Scherspannung"), TEXT("Markierte Standardangriffe verursachen 20% mehr Gesamtschaden."), {{TEXT("MarkedAttackDamageMultiplierAdd"), 0.20}})
        }),
        MakeCharacterTier(TEXT("Gebirgsdruck"), {
            MakeCharacterSkillpoint(TEXT("Bleibende Fraktur"), TEXT("Ein verstaerkter Standardangriff verbraucht die Frakturmarke nicht; die zielgebundene Abklingzeit bleibt bestehen."), {{TEXT("KeepMarkAfterEmpoweredAttack"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Felskranz"), TEXT("Das Ausloesen einer Fraktur erzeugt zwei zusaetzliche Felsen neben dem Ziel."), {{TEXT("AdditionalRocksOnMarkTrigger"), 2.0}}),
            MakeCharacterSkillpoint(TEXT("Kontinentalbruch"), TEXT("Erhoeht Bonusschaden und Verteidigungsignorierung markierter Standardangriffe."), {{TEXT("MarkedAttackBonusDamage"), 70.0}, {TEXT("MarkedAttackDefenseIgnore"), 35.0}})
        })
    };
    Progression->MarkPackageDirty(); Character->Modify(); Character->CharacterProgression = Progression; Character->MarkPackageDirty(); MarkPackageDirty();
#endif
}

void URoundCharacterRegistryData::PopulateBladecallerProgressionDefaults()
{
#if WITH_EDITOR
    URoundCharacterProgressionData* Progression = LoadObject<URoundCharacterProgressionData>(nullptr, TEXT("/Game/TopDown/Characters/Bladecaller/DA_BladecallerProgression.DA_BladecallerProgression"));
    URoundCharacterData* Character = LoadObject<URoundCharacterData>(nullptr, TEXT("/Game/TopDown/Characters/Bladecaller/BladecallerDetails.BladecallerDetails"));
    if (!Progression || !Character) return;
    Progression->Modify(); Progression->CharacterId = TEXT("Bladecaller"); Progression->DisplayName = FText::FromString(TEXT("Bladecaller"));
    Progression->Description = FText::FromString(TEXT("Spezialisiert den Kreislauf aus Klingenaufbau, Umlenkung, Positionswechsel und Rueckruf."));
    Progression->Tiers = {
        MakeCharacterTier(TEXT("Klingenbindung"), {
            MakeCharacterSkillpoint(TEXT("Dauerhafte Bindung"), TEXT("Erhoeht die Lebensdauer jeder Klinge um {BladeLifeTime} Initiative."), {{TEXT("BladeLifeTime"), 60.0}}),
            MakeCharacterSkillpoint(TEXT("Leichter Schritt"), TEXT("Verringert die Bewegungskosten um {-MoveCostAdd}."), {{TEXT("MoveCostAdd"), -2.0}}),
            MakeCharacterSkillpoint(TEXT("Fernlenkung"), TEXT("Erhoeht die Reichweite von Standardangriffen um {AttackRangeAdd}."), {{TEXT("AttackRangeAdd"), 1.0}})
        }),
        MakeCharacterTier(TEXT("Kreislauf"), {
            MakeCharacterSkillpoint(TEXT("Rueckgewinnung"), TEXT("Das Aufheben einer Klinge gibt {InitiativeGainOnBladePickup} Initiative."), {{TEXT("InitiativeGainOnBladePickup"), 14.0}}),
            MakeCharacterSkillpoint(TEXT("Klingenheilung"), TEXT("Das Aufheben einer Klinge heilt {HealOnBladePickup} Leben."), {{TEXT("HealOnBladePickup"), 85.0}}),
            MakeCharacterSkillpoint(TEXT("Schneller Aufbau"), TEXT("Das Erzeugen einer Klinge gibt {InitiativeGainOnBladeSummon} Initiative."), {{TEXT("InitiativeGainOnBladeSummon"), 8.0}})
        }),
        MakeCharacterTier(TEXT("Schwarmkontrolle"), {
            MakeCharacterSkillpoint(TEXT("Neue Bindung"), TEXT("Das Erzeugen einer Klinge erneuert die Lebensdauer aller aktiven Klingen."), {{TEXT("RefreshBladesOnSummon"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Ruecklauf"), TEXT("Das Aufheben einer Klinge reduziert alle aktuellen Faehigkeitscooldowns um {CooldownReductionOnBladePickup} Initiative."), {{TEXT("CooldownReductionOnBladePickup"), 22.0}}),
            MakeCharacterSkillpoint(TEXT("Klingendruck"), TEXT("Jede aktive Klinge erhoeht den gesamten verursachten Schaden um 4%."), {{TEXT("DamageMultiplierAddPerActiveBlade"), 0.04}})
        }),
        MakeCharacterTier(TEXT("Tanz der Klingen"), {
            MakeCharacterSkillpoint(TEXT("Unvergaenglicher Schwarm"), TEXT("Erhoeht die Klingenlebensdauer stark; jede neue Klinge erneuert alle aktiven Klingen."), {{TEXT("BladeLifeTime"), 100.0}, {TEXT("RefreshBladesOnSummon"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Geschlossener Kreis"), TEXT("Aufgehobene Klingen heilen, geben Initiative und reduzieren Cooldowns."), {{TEXT("HealOnBladePickup"), 100.0}, {TEXT("InitiativeGainOnBladePickup"), 16.0}, {TEXT("CooldownReductionOnBladePickup"), 25.0}}),
            MakeCharacterSkillpoint(TEXT("Klingensturm"), TEXT("Jede aktive Klinge erhoeht den Gesamtschaden um 7%; erzeugte Klingen geben Initiative."), {{TEXT("DamageMultiplierAddPerActiveBlade"), 0.07}, {TEXT("InitiativeGainOnBladeSummon"), 10.0}})
        })
    };
    Progression->MarkPackageDirty(); Character->Modify(); Character->CharacterProgression = Progression; Character->MarkPackageDirty(); MarkPackageDirty();
#endif
}

void URoundCharacterRegistryData::PopulateClericProgressionDefaults()
{
#if WITH_EDITOR
    URoundCharacterProgressionData* Progression = LoadObject<URoundCharacterProgressionData>(nullptr, TEXT("/Game/TopDown/Characters/Cleric/DA_ClericProgression.DA_ClericProgression"));
    URoundCharacterData* Character = LoadObject<URoundCharacterData>(nullptr, TEXT("/Game/TopDown/Characters/Cleric/ClericDetails.ClericDetails"));
    if (!Progression || !Character) return;
    Progression->Modify(); Progression->CharacterId = TEXT("Cleric"); Progression->DisplayName = FText::FromString(TEXT("Cleric"));
    Progression->Description = FText::FromString(TEXT("Formt Blessing zu schneller Ausloesung, staerkerer Heilung oder zusaetzlicher Unterstuetzung."));
    Progression->Tiers = {
        MakeCharacterTier(TEXT("Segen"), {
            MakeCharacterSkillpoint(TEXT("Anhaltender Segen"), TEXT("Blessing haelt {BlessingDurationInitiative} Initiative laenger."), {{TEXT("BlessingDurationInitiative"), 30.0}}),
            MakeCharacterSkillpoint(TEXT("Staerkender Segen"), TEXT("Erhoeht die Heilung bei voller Blessing-Staerke um {BlessingHeal}."), {{TEXT("BlessingHeal"), 75.0}}),
            MakeCharacterSkillpoint(TEXT("Leichte Robe"), TEXT("Verringert Bewegungskosten um {-MoveCostAdd}."), {{TEXT("MoveCostAdd"), -2.0}})
        }),
        MakeCharacterTier(TEXT("Glaubensaufbau"), {
            MakeCharacterSkillpoint(TEXT("Direkter Zuspruch"), TEXT("Cleric-Zauber geben Verbündeten einen zusaetzlichen Blessing-Stapel."), {{TEXT("AdditionalBlessingStacksPerSpell"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Fruehe Erfuellung"), TEXT("Blessing wird bereits mit einem Stapel weniger ausgeloest."), {{TEXT("BlessingMaxStacks"), -1.0}}),
            MakeCharacterSkillpoint(TEXT("Geteilte Hoffnung"), TEXT("Wenn Blessing heilt, erhaelt der Verbündete {InitiativeOnBlessingResolve} Initiative."), {{TEXT("InitiativeOnBlessingResolve"), 15.0}})
        }),
        MakeCharacterTier(TEXT("Erfuellung"), {
            MakeCharacterSkillpoint(TEXT("Ueberfliessende Gnade"), TEXT("Blessing heilt weitere {BlessingHeal} Leben."), {{TEXT("BlessingHeal"), 125.0}}),
            MakeCharacterSkillpoint(TEXT("Rueckkehrender Glaube"), TEXT("Wenn Blessing heilt, heilt sich der Cleric um 35% der tatsaechlichen Heilung."), {{TEXT("SelfHealFractionOnBlessingResolve"), 0.35}}),
            MakeCharacterSkillpoint(TEXT("Beständiger Zuspruch"), TEXT("Blessing haelt laenger und Zauber geben einen zusaetzlichen Stapel."), {{TEXT("BlessingDurationInitiative"), 35.0}, {TEXT("AdditionalBlessingStacksPerSpell"), 1.0}})
        }),
        MakeCharacterTier(TEXT("Wunder"), {
            MakeCharacterSkillpoint(TEXT("Sofortiges Wunder"), TEXT("Blessing benoetigt zwei Stapel weniger und heilt etwas weniger."), {{TEXT("BlessingMaxStacks"), -2.0}, {TEXT("BlessingHeal"), -50.0}}),
            MakeCharacterSkillpoint(TEXT("Grosses Wunder"), TEXT("Blessing heilt massiv mehr und bleibt laenger bestehen."), {{TEXT("BlessingHeal"), 200.0}, {TEXT("BlessingDurationInitiative"), 40.0}}),
            MakeCharacterSkillpoint(TEXT("Gemeinsames Wunder"), TEXT("Blessing gibt Initiative und heilt den Cleric fuer die Haelfte der tatsaechlichen Heilung."), {{TEXT("InitiativeOnBlessingResolve"), 22.0}, {TEXT("SelfHealFractionOnBlessingResolve"), 0.50}})
        })
    };
    Progression->MarkPackageDirty(); Character->Modify(); Character->CharacterProgression = Progression; Character->MarkPackageDirty(); MarkPackageDirty();
#endif
}

void URoundCharacterRegistryData::PopulateGamblerProgressionDefaults()
{
#if WITH_EDITOR
    URoundCharacterProgressionData* Progression = LoadObject<URoundCharacterProgressionData>(nullptr, TEXT("/Game/TopDown/Characters/Gambler/DA_GamblerProgression.DA_GamblerProgression"));
    URoundCharacterData* Character = LoadObject<URoundCharacterData>(nullptr, TEXT("/Game/TopDown/Characters/Gambler/GamblerDetails.GamblerDetails"));
    if (!Progression || !Character) return;
    Progression->Modify(); Progression->CharacterId = TEXT("Gambler"); Progression->DisplayName = FText::FromString(TEXT("Gambler"));
    Progression->Description = FText::FromString(TEXT("Spezialisiert Handgroesse, Kartenumsatz und den Wert hoher Karten."));
    Progression->Tiers = {
        MakeCharacterTier(TEXT("Eroeffnungshand"), {
            MakeCharacterSkillpoint(TEXT("Tiefe Taschen"), TEXT("Erhoeht die maximale Handgroesse um {MaxHandCards}."), {{TEXT("MaxHandCards"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Heisse Hand"), TEXT("Zieht zu Kampfbeginn {StartingHandCardsAdd} zusaetzliche Karte."), {{TEXT("StartingHandCardsAdd"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Schneller Schritt"), TEXT("Verringert Bewegungskosten um {-MoveCostAdd}."), {{TEXT("MoveCostAdd"), -2.0}})
        }),
        MakeCharacterTier(TEXT("Der Einsatz"), {
            MakeCharacterSkillpoint(TEXT("Schneller Geber"), TEXT("Zieht zu Zugbeginn {TurnStartDrawAdd} zusaetzliche Karte."), {{TEXT("TurnStartDrawAdd"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Geduldiges Spiel"), TEXT("Nach einem Zug ohne Aktion wird {IdleTurnDrawAdd} zusaetzliche Karte gezogen."), {{TEXT("IdleTurnDrawAdd"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Abwurfgewinn"), TEXT("Manuell abgeworfene Karten geben je {InitiativePerDiscardedCard} Initiative, maximal 30 pro Abwurf."), {{TEXT("InitiativePerDiscardedCard"), 6.0}})
        }),
        MakeCharacterTier(TEXT("Einsatz erhoehen"), {
            MakeCharacterSkillpoint(TEXT("Gezinktes Deck"), TEXT("Entfernt alle Karten unter Rang 4 aus dem Deck."), {{TEXT("MinimumDeckRankAdd"), 2.0}}),
            MakeCharacterSkillpoint(TEXT("High Roller"), TEXT("Erhoeht den Rangmultiplikator jeder Karte um {CardRankMultiplierAdd}."), {{TEXT("CardRankMultiplierAdd"), 0.15}}),
            MakeCharacterSkillpoint(TEXT("Neues Spiel"), TEXT("Beim Mischen des Ablagestapels in das Deck heilt der Gambler {HealOnDeckReshuffle} Leben."), {{TEXT("HealOnDeckReshuffle"), 180.0}})
        }),
        MakeCharacterTier(TEXT("All In"), {
            MakeCharacterSkillpoint(TEXT("Koenigliche Hand"), TEXT("Erhoeht Handgroesse und Kartenmultiplikator."), {{TEXT("MaxHandCards"), 2.0}, {TEXT("CardRankMultiplierAdd"), 0.10}}),
            MakeCharacterSkillpoint(TEXT("Kartenfluss"), TEXT("Nach jeder erfolgreich eingesetzten Faehigkeit wird {DrawAfterAbilityUse} Karte gezogen."), {{TEXT("DrawAfterAbilityUse"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Pokerface"), TEXT("Verringert allen eingehenden Schaden um 15%."), {{TEXT("IncomingDamageMultiplierAdd"), -0.15}})
        })
    };
    Progression->MarkPackageDirty(); Character->Modify(); Character->CharacterProgression = Progression; Character->MarkPackageDirty(); MarkPackageDirty();
#endif
}

void URoundCharacterRegistryData::PopulatePaladinProgressionDefaults()
{
#if WITH_EDITOR
    URoundCharacterProgressionData* Progression = LoadObject<URoundCharacterProgressionData>(nullptr, TEXT("/Game/TopDown/Characters/Paladin/DA_PaladinProgression.DA_PaladinProgression"));
    URoundCharacterData* Character = LoadObject<URoundCharacterData>(nullptr, TEXT("/Game/TopDown/Characters/Paladin/PaladinDetails.PaladinDetails"));
    if (!Progression || !Character) return;
    Progression->Modify(); Progression->CharacterId = TEXT("Paladin"); Progression->DisplayName = FText::FromString(TEXT("Paladin"));
    Progression->Description = FText::FromString(TEXT("Formt die Schutzaura zu enger Formation, standhafter Selbstaufopferung oder offensiver Vergeltung."));
    Progression->Tiers = {
        MakeCharacterTier(TEXT("Schutzaura"), {
            MakeCharacterSkillpoint(TEXT("Weiter Schutzkreis"), TEXT("Erhoeht den Radius der Schutzaura um {AuraRadius}."), {{TEXT("AuraRadius"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Staerkerer Schwur"), TEXT("Erhoeht die Schadensreduktion der Aura um 5 Prozentpunkte."), {{TEXT("AuraDamageReduction"), 0.05}}),
            MakeCharacterSkillpoint(TEXT("Gepanzerter Schritt"), TEXT("Erhoeht die Verteidigung um {DefenseAdd}."), {{TEXT("DefenseAdd"), 30.0}})
        }),
        MakeCharacterTier(TEXT("Getragene Last"), {
            MakeCharacterSkillpoint(TEXT("Geteilter Segen"), TEXT("Die Schutzaura wirkt nun auch auf den Paladin selbst."), {{TEXT("AuraAffectSelf"), 1.0}}),
            MakeCharacterSkillpoint(TEXT("Glaube aus Schmerz"), TEXT("Bei jeder Schadensverhinderung durch die Aura erhaelt der Paladin {InitiativeOnAuraPrevent} Initiative."), {{TEXT("InitiativeOnAuraPrevent"), 3.0}}),
            MakeCharacterSkillpoint(TEXT("Heiliger Rueckhalt"), TEXT("Der Paladin heilt sich um 20% des von seiner Aura verhinderten Schadens."), {{TEXT("HealFractionOfAuraPreventedDamage"), 0.20}})
        }),
        MakeCharacterTier(TEXT("Standhafte Formation"), {
            MakeCharacterSkillpoint(TEXT("Ungebrochener Kreis"), TEXT("Erhoeht Auraradius und maximale Initiative."), {{TEXT("AuraRadius"), 1.0}, {TEXT("MaxInitiativeAdd"), 20.0}}),
            MakeCharacterSkillpoint(TEXT("Schwurgepanzer"), TEXT("Erhoeht die Aurareduktion und die eigene Verteidigung."), {{TEXT("AuraDamageReduction"), 0.05}, {TEXT("DefenseAdd"), 35.0}}),
            MakeCharacterSkillpoint(TEXT("Antwortender Glaube"), TEXT("Jede Schadensverhinderung reduziert alle aktuellen Faehigkeitscooldowns um {CooldownReductionOnAuraPrevent} Initiative."), {{TEXT("CooldownReductionOnAuraPrevent"), 4.0}})
        }),
        MakeCharacterTier(TEXT("Lebendes Bollwerk"), {
            MakeCharacterSkillpoint(TEXT("Heilige Bastion"), TEXT("Die Aura wirkt auf den Paladin, reicht weiter und reduziert mehr Schaden."), {{TEXT("AuraAffectSelf"), 1.0}, {TEXT("AuraRadius"), 1.0}, {TEXT("AuraDamageReduction"), 0.05}}),
            MakeCharacterSkillpoint(TEXT("Maertyrer"), TEXT("Verhinderter Schaden heilt stark und erzeugt Initiative."), {{TEXT("HealFractionOfAuraPreventedDamage"), 0.35}, {TEXT("InitiativeOnAuraPrevent"), 4.0}}),
            MakeCharacterSkillpoint(TEXT("Unerschuetterlicher Schwur"), TEXT("Erhoeht Leben und Verteidigung; Aura-Procs beschleunigen alle Faehigkeiten."), {{TEXT("MaxHealthAdd"), 350.0}, {TEXT("DefenseAdd"), 45.0}, {TEXT("CooldownReductionOnAuraPrevent"), 5.0}})
        })
    };
    Progression->MarkPackageDirty(); Character->Modify(); Character->CharacterProgression = Progression; Character->MarkPackageDirty(); MarkPackageDirty();
#endif
}

void URoundCharacterRegistryData::EnsureFiveAbilityLoadouts()
{
#if WITH_EDITOR
    int32 UpdatedCharacters = 0;

    UpdatedCharacters += AppendAbilityToCharacterIfMissing(
        TEXT("/Game/TopDown/Characters/Character_2/Character2Details.Character2Details"),
        TEXT("/Game/TopDown/Data/Abilities/DA_Ability_SuppressingFire.DA_Ability_SuppressingFire")) ? 1 : 0;
    UpdatedCharacters += AppendAbilityToCharacterIfMissing(
        TEXT("/Game/TopDown/Characters/Character_3/Character3Details.Character3Details"),
        TEXT("/Game/TopDown/Data/Abilities/DA_Ability_DeathMark.DA_Ability_DeathMark")) ? 1 : 0;
    UpdatedCharacters += AppendAbilityToCharacterIfMissing(
        TEXT("/Game/TopDown/Characters/Character_4/Character4Details.Character4Details"),
        TEXT("/Game/TopDown/Data/Abilities/DA_Ability_ArcaneShift.DA_Ability_ArcaneShift")) ? 1 : 0;
    UpdatedCharacters += AppendAbilityToCharacterIfMissing(
        TEXT("/Game/TopDown/Characters/Character_5/Character5Details.Character5Details"),
        TEXT("/Game/TopDown/Data/Abilities/DA_Ability_Rend.DA_Ability_Rend")) ? 1 : 0;
    UpdatedCharacters += AppendAbilityToCharacterIfMissing(
        TEXT("/Game/TopDown/Characters/Character_6/Character6Details.Character6Details"),
        TEXT("/Game/TopDown/Data/Abilities/DA_Ability_StringSnare.DA_Ability_StringSnare")) ? 1 : 0;

    if (UpdatedCharacters > 0)
    {
        MarkPackageDirty();
    }

    UE_LOG(LogRoundCharacterRegistryImport, Log, TEXT("EnsureFiveAbilityLoadouts updated %d character loadouts"), UpdatedCharacters);
#endif
}
