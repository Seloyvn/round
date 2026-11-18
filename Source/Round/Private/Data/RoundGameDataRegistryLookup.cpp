#include "Data/RoundGameDataRegistryLookup.h"

#include "Data/RoundAbilityRegistryData.h"
#include "Data/RoundCharacterRegistryData.h"
#include "Data/RoundGameDataRegistry.h"
#include "Data/RoundTerrainData.h"
#include "Data/RoundTerrainRegionData.h"
#include "Data/RoundTeamPresetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UI/RoundAuraEntryWidget.h"
#include "UI/RoundCharacterSelectionOptionWidget.h"
#include "UI/RoundGameUIWidget.h"
#include "UI/RoundDebugWidget.h"
#include "UI/RoundHealthbarWidget.h"
#include "UI/RoundHoverInfoWidget.h"
#include "UI/RoundInitiativeIconWidget.h"
#include "UI/RoundMainMenuWidget.h"
#include "UI/RoundMenuListEntryWidgets.h"
#include "UI/RoundSkillMenuWidget.h"
#include "UI/RoundSkillmenuSelectorPointWidget.h"
#include "UI/RoundSkillpointMenuWidget.h"
#include "UI/RoundStartFieldWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"

namespace RoundGameDataRegistryLookup
{
namespace
{
template <typename TObjectType>
TObjectType* LoadSoftObject(const TSoftObjectPtr<TObjectType>& SoftObject)
{
    return SoftObject.LoadSynchronous();
}

template <typename TObjectType>
TSubclassOf<TObjectType> LoadSoftClass(const TSoftClassPtr<TObjectType>& SoftClass)
{
    if (UClass* LoadedClass = SoftClass.LoadSynchronous())
    {
        return LoadedClass;
    }

    return nullptr;
}
}

URoundGameDataRegistry* ResolveGameDataRegistry()
{
    if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
    {
        TArray<FPrimaryAssetId> RegistryIds;
        AssetManager->GetPrimaryAssetIdList(TEXT("RoundGameDataRegistry"), RegistryIds);
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

            if (URoundGameDataRegistry* Registry = Cast<URoundGameDataRegistry>(AssetObject))
            {
                return Registry;
            }
        }
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    FARFilter Filter;
    Filter.ClassPaths.Add(URoundGameDataRegistry::StaticClass()->GetClassPathName());
    Filter.bRecursiveClasses = true;

    TArray<FAssetData> Assets;
    AssetRegistryModule.Get().GetAssets(Filter, Assets);
    for (const FAssetData& Asset : Assets)
    {
        if (URoundGameDataRegistry* Registry = Cast<URoundGameDataRegistry>(Asset.GetAsset()))
        {
            return Registry;
        }
    }

    return nullptr;
}

URoundCharacterRegistryData* ResolveCharacterRegistry()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftObject(Registry->CharacterRegistry);
    }

    return nullptr;
}

URoundAbilityRegistryData* ResolveAbilityRegistry()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftObject(Registry->AbilityRegistry);
    }

    return nullptr;
}

URoundTerrainRegistryData* ResolveTerrainRegistry()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftObject(Registry->TerrainRegistry);
    }

    return nullptr;
}

URoundTerrainRegionRegistryData* ResolveTerrainRegionRegistry()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftObject(Registry->TerrainRegionRegistry);
    }

    return nullptr;
}

URoundTeamPresetData* ResolveDefaultTeamPresets()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftObject(Registry->DefaultTeamPresets);
    }

    return nullptr;
}

TSubclassOf<URoundGameUIWidget> ResolveGameUIWidgetClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->GameUIWidgetClass);
    }

    return nullptr;
}

TSubclassOf<URoundDebugWidget> ResolveDebugWidgetClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->DebugWidgetClass);
    }
    return nullptr;
}

TSubclassOf<URoundHealthbarWidget> ResolveHealthbarWidgetClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->HealthbarWidgetClass);
    }

    return nullptr;
}

TSubclassOf<URoundMainMenuWidget> ResolveMainMenuWidgetClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->MainMenuWidgetClass);
    }

    return nullptr;
}

TSubclassOf<URoundTeamsListEntryWidget> ResolveTeamEntryWidgetClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->TeamEntryWidgetClass);
    }

    return nullptr;
}

TSubclassOf<URoundCharacterSelectionOptionWidget> ResolveCharacterSelectionOptionWidgetClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->CharacterSelectionOptionWidgetClass);
    }

    return nullptr;
}

TSubclassOf<URoundSkillMenuWidget> ResolveSkillMenuWidgetClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->SkillMenuWidgetClass);
    }

    return nullptr;
}

TSubclassOf<URoundSkillpointMenuWidget> ResolveSkillpointMenuWidgetClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->SkillpointMenuWidgetClass);
    }

    return nullptr;
}

TSubclassOf<URoundSkillmenuSelectorPointWidget> ResolveSkillSelectorPointWidgetClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->SkillSelectorPointWidgetClass);
    }

    return nullptr;
}

TSubclassOf<URoundStartFieldWidget> ResolveStartFieldWidgetClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->StartFieldWidgetClass);
    }

    return nullptr;
}

TSubclassOf<URoundInitiativeIconWidget> ResolveInitiativeIconWidgetClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->InitiativeIconWidgetClass);
    }

    return nullptr;
}

TSubclassOf<URoundAuraEntryWidget> ResolveAuraEntryWidgetClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->AuraEntryWidgetClass);
    }

    return nullptr;
}

TSubclassOf<URoundHoverInfoWidget> ResolveDefaultHoverInfoWidgetClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->DefaultHoverInfoWidgetClass);
    }

    return nullptr;
}

UDataTable* ResolveFormatTable()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftObject(Registry->FormatTable);
    }

    return nullptr;
}

TSoftObjectPtr<UWorld> ResolveMatchMap()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return Registry->MatchMap;
    }

    return nullptr;
}

TSubclassOf<AActor> ResolveAimVisualActorClass()
{
    if (URoundGameDataRegistry* Registry = ResolveGameDataRegistry())
    {
        return LoadSoftClass(Registry->AimVisualActorClass);
    }

    return nullptr;
}
}
