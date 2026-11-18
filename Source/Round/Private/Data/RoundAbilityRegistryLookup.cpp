#include "Data/RoundAbilityRegistryLookup.h"

#include "Actors/RoundAbilityActor.h"
#include "Data/RoundAbilityData.h"
#include "Data/RoundAbilityRegistryData.h"
#include "Data/RoundGameDataRegistryLookup.h"
#include "Engine/AssetManager.h"

namespace RoundAbilityRegistry
{
URoundAbilityRegistryData* ResolveAbilityRegistry()
{
    if (URoundAbilityRegistryData* Registry = RoundGameDataRegistryLookup::ResolveAbilityRegistry())
    {
        return Registry;
    }

    if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
    {
        TArray<FPrimaryAssetId> RegistryIds;
        AssetManager->GetPrimaryAssetIdList(TEXT("RoundAbilityRegistryData"), RegistryIds);
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

            if (URoundAbilityRegistryData* Registry = Cast<URoundAbilityRegistryData>(AssetObject))
            {
                return Registry;
            }
        }
    }

    return nullptr;
}

URoundAbilityData* FindAbilityDataById(const FName AbilityId)
{
    if (URoundAbilityRegistryData* Registry = ResolveAbilityRegistry())
    {
        return Registry->FindAbilityDataById(AbilityId);
    }

    return nullptr;
}

TSubclassOf<ARoundAbilityActor> FindAbilityClassById(const FName AbilityId)
{
    if (URoundAbilityRegistryData* Registry = ResolveAbilityRegistry())
    {
        return Registry->FindAbilityClassById(AbilityId);
    }

    return nullptr;
}
}
