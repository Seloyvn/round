#pragma once

#include "CoreMinimal.h"

class ARoundCharacter;
class URoundCharacterData;
class URoundCharacterRegistryData;

namespace RoundCharacterRegistry
{
URoundCharacterRegistryData* ResolveCharacterRegistry();
void GetCharacterDataAssets(TArray<URoundCharacterData*>& OutCharacterDataAssets, bool bIncludePuppets = false);
void GetCharacterIds(TArray<FName>& OutCharacterIds, bool bIncludePuppets = false);
URoundCharacterData* FindCharacterDataById(FName CharacterId);
TSoftObjectPtr<URoundCharacterData> FindCharacterDataAssetById(FName CharacterId);
TSubclassOf<ARoundCharacter> FindCharacterClassById(FName CharacterId);
FName NormalizeCharacterId(FString InCharacterId);
}
