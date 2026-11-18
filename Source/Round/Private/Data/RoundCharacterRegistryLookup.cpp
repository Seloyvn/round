#include "Data/RoundCharacterRegistryLookup.h"

#include "Actors/RoundCharacter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/RoundCharacterData.h"
#include "Data/RoundGameDataRegistryLookup.h"
#include "Data/RoundCharacterRegistryData.h"
#include "Engine/AssetManager.h"
#include "Modules/ModuleManager.h"

namespace
{
FString NormalizeCharacterLookupKey(FString Value)
{
    Value = Value.TrimStartAndEnd();
    Value.ReplaceInline(TEXT("BP_"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT("_C"), TEXT(""), ESearchCase::IgnoreCase);
    Value.ReplaceInline(TEXT(" "), TEXT(""));
    Value.ReplaceInline(TEXT("_"), TEXT(""));
    Value.ReplaceInline(TEXT("-"), TEXT(""));
    Value.ReplaceInline(TEXT("Charcter"), TEXT("Character"), ESearchCase::IgnoreCase);

    if (Value.IsNumeric())
    {
        Value = FString::Printf(TEXT("Character%s"), *Value);
    }

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

bool IsPuppetCharacterData(const URoundCharacterData* CharacterData)
{
    if (!CharacterData)
    {
        return false;
    }

    const FString CharacterId = CharacterData->CharacterId.ToString();
    return CharacterId.Contains(TEXT("Puppet"), ESearchCase::IgnoreCase)
        || CharacterData->GetName().Contains(TEXT("Puppet"), ESearchCase::IgnoreCase);
}

FString GetCharacterSearchDirectory()
{
    if (const URoundCharacterRegistryData* Registry = RoundCharacterRegistry::ResolveCharacterRegistry())
    {
        if (!Registry->CharacterDataDirectory.IsEmpty())
        {
            return Registry->CharacterDataDirectory;
        }
    }

    return TEXT("/Game/TopDown/Characters");
}

void SortCharacterAssets(TArray<URoundCharacterData*>& CharacterDataAssets)
{
    CharacterDataAssets.Sort([](const URoundCharacterData& Left, const URoundCharacterData& Right)
    {
        const int32 LeftNumericId = Left.CharacterNumericId;
        const int32 RightNumericId = Right.CharacterNumericId;
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

        const FString LeftKey = !Left.CharacterId.IsNone() ? Left.CharacterId.ToString() : Left.GetName();
        const FString RightKey = !Right.CharacterId.IsNone() ? Right.CharacterId.ToString() : Right.GetName();
        return LeftKey < RightKey;
    });
}
}

namespace RoundCharacterRegistry
{
URoundCharacterRegistryData* ResolveCharacterRegistry()
{
    if (URoundCharacterRegistryData* Registry = RoundGameDataRegistryLookup::ResolveCharacterRegistry())
    {
        return Registry;
    }

    if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
    {
        TArray<FPrimaryAssetId> RegistryIds;
        AssetManager->GetPrimaryAssetIdList(TEXT("RoundCharacterRegistryData"), RegistryIds);
        for (const FPrimaryAssetId& RegistryId : RegistryIds)
        {
            UObject* AssetObject = AssetManager->GetPrimaryAssetObject(RegistryId);
            if (!AssetObject)
            {
                const FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(RegistryId);
                if (AssetPath.IsValid())
                {
                    AssetObject = AssetPath.TryLoad();
                }
            }

            if (URoundCharacterRegistryData* Registry = Cast<URoundCharacterRegistryData>(AssetObject))
            {
                return Registry;
            }
        }
    }

    return nullptr;
}

void GetCharacterDataAssets(TArray<URoundCharacterData*>& OutCharacterDataAssets, const bool bIncludePuppets)
{
    OutCharacterDataAssets.Reset();
    TSet<FString> AddedKeys;

    if (const URoundCharacterRegistryData* Registry = ResolveCharacterRegistry())
    {
        for (const TSoftObjectPtr<URoundCharacterData>& CharacterAsset : Registry->RuntimeCharacters)
        {
            URoundCharacterData* CharacterData = CharacterAsset.LoadSynchronous();
            if (!CharacterData || (!bIncludePuppets && IsPuppetCharacterData(CharacterData)))
            {
                continue;
            }

            const FString Key = NormalizeCharacterLookupKey(!CharacterData->CharacterId.IsNone()
                ? CharacterData->CharacterId.ToString()
                : CharacterData->GetName());
            if (!AddedKeys.Contains(Key))
            {
                AddedKeys.Add(Key);
                OutCharacterDataAssets.Add(CharacterData);
            }
        }
    }

    if (OutCharacterDataAssets.Num() == 0)
    {
        FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
        FARFilter Filter;
        Filter.ClassPaths.Add(URoundCharacterData::StaticClass()->GetClassPathName());
        Filter.PackagePaths.Add(*GetCharacterSearchDirectory());
        Filter.bRecursivePaths = true;

        TArray<FAssetData> AssetDataList;
        AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);
        for (const FAssetData& AssetData : AssetDataList)
        {
            URoundCharacterData* CharacterData = Cast<URoundCharacterData>(AssetData.GetAsset());
            if (!CharacterData || (!bIncludePuppets && IsPuppetCharacterData(CharacterData)))
            {
                continue;
            }

            const FString Key = NormalizeCharacterLookupKey(!CharacterData->CharacterId.IsNone()
                ? CharacterData->CharacterId.ToString()
                : CharacterData->GetName());
            if (!AddedKeys.Contains(Key))
            {
                AddedKeys.Add(Key);
                OutCharacterDataAssets.Add(CharacterData);
            }
        }
    }

    SortCharacterAssets(OutCharacterDataAssets);
}

void GetCharacterIds(TArray<FName>& OutCharacterIds, const bool bIncludePuppets)
{
    OutCharacterIds.Reset();

    TArray<URoundCharacterData*> CharacterDataAssets;
    GetCharacterDataAssets(CharacterDataAssets, bIncludePuppets);
    for (const URoundCharacterData* CharacterData : CharacterDataAssets)
    {
        if (!CharacterData)
        {
            continue;
        }

        OutCharacterIds.Add(!CharacterData->CharacterId.IsNone()
            ? CharacterData->CharacterId
            : FName(*CharacterData->GetName()));
    }
}

URoundCharacterData* FindCharacterDataById(const FName CharacterId)
{
    if (CharacterId.IsNone())
    {
        return nullptr;
    }

    const FString RequestedKey = NormalizeCharacterLookupKey(CharacterId.ToString());
    TArray<URoundCharacterData*> CharacterDataAssets;
    GetCharacterDataAssets(CharacterDataAssets, true);
    for (URoundCharacterData* CharacterData : CharacterDataAssets)
    {
        if (!CharacterData)
        {
            continue;
        }

        const FString CharacterIdKey = NormalizeCharacterLookupKey(!CharacterData->CharacterId.IsNone()
            ? CharacterData->CharacterId.ToString()
            : CharacterData->GetName());
        const FString AssetNameKey = NormalizeCharacterLookupKey(CharacterData->GetName());
        if (CharacterIdKey == RequestedKey || AssetNameKey == RequestedKey)
        {
            return CharacterData;
        }
    }

    return nullptr;
}

TSoftObjectPtr<URoundCharacterData> FindCharacterDataAssetById(const FName CharacterId)
{
    if (URoundCharacterData* CharacterData = FindCharacterDataById(CharacterId))
    {
        return TSoftObjectPtr<URoundCharacterData>(CharacterData);
    }

    return TSoftObjectPtr<URoundCharacterData>();
}

TSubclassOf<ARoundCharacter> FindCharacterClassById(const FName CharacterId)
{
    if (const URoundCharacterData* CharacterData = FindCharacterDataById(CharacterId))
    {
        return CharacterData->CharacterClass;
    }

    return nullptr;
}

FName NormalizeCharacterId(FString InCharacterId)
{
    const FString RequestedKey = NormalizeCharacterLookupKey(InCharacterId);
    TArray<URoundCharacterData*> CharacterDataAssets;
    GetCharacterDataAssets(CharacterDataAssets, true);
    for (const URoundCharacterData* CharacterData : CharacterDataAssets)
    {
        if (!CharacterData)
        {
            continue;
        }

        const FString CharacterIdKey = NormalizeCharacterLookupKey(!CharacterData->CharacterId.IsNone()
            ? CharacterData->CharacterId.ToString()
            : CharacterData->GetName());
        if (CharacterIdKey == RequestedKey)
        {
            return !CharacterData->CharacterId.IsNone()
                ? CharacterData->CharacterId
                : FName(*CharacterData->GetName());
        }
    }

    return FName(*InCharacterId.Replace(TEXT("_"), TEXT("")));
}
}
