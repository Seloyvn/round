#pragma once

#include "CoreMinimal.h"

class URoundAbilityRegistryData;
class URoundAuraEntryWidget;
class URoundCharacterSelectionOptionWidget;
class URoundCharacterRegistryData;
class URoundGameDataRegistry;
class URoundGameUIWidget;
class URoundDebugWidget;
class URoundHealthbarWidget;
class URoundHoverInfoWidget;
class URoundInitiativeIconWidget;
class URoundMainMenuWidget;
class URoundSkillMenuWidget;
class URoundSkillmenuSelectorPointWidget;
class URoundSkillpointMenuWidget;
class URoundStartFieldWidget;
class URoundTerrainRegistryData;
class URoundTerrainRegionRegistryData;
class URoundTeamPresetData;
class URoundTeamsListEntryWidget;
class UDataTable;
class UWorld;
class AActor;

namespace RoundGameDataRegistryLookup
{
URoundGameDataRegistry* ResolveGameDataRegistry();
URoundCharacterRegistryData* ResolveCharacterRegistry();
URoundAbilityRegistryData* ResolveAbilityRegistry();
URoundTerrainRegistryData* ResolveTerrainRegistry();
URoundTerrainRegionRegistryData* ResolveTerrainRegionRegistry();
URoundTeamPresetData* ResolveDefaultTeamPresets();
TSubclassOf<URoundGameUIWidget> ResolveGameUIWidgetClass();
TSubclassOf<URoundDebugWidget> ResolveDebugWidgetClass();
TSubclassOf<URoundHealthbarWidget> ResolveHealthbarWidgetClass();
TSubclassOf<URoundMainMenuWidget> ResolveMainMenuWidgetClass();
TSubclassOf<URoundTeamsListEntryWidget> ResolveTeamEntryWidgetClass();
TSubclassOf<URoundCharacterSelectionOptionWidget> ResolveCharacterSelectionOptionWidgetClass();
TSubclassOf<URoundSkillMenuWidget> ResolveSkillMenuWidgetClass();
TSubclassOf<URoundSkillpointMenuWidget> ResolveSkillpointMenuWidgetClass();
TSubclassOf<URoundSkillmenuSelectorPointWidget> ResolveSkillSelectorPointWidgetClass();
TSubclassOf<URoundStartFieldWidget> ResolveStartFieldWidgetClass();
TSubclassOf<URoundInitiativeIconWidget> ResolveInitiativeIconWidgetClass();
TSubclassOf<URoundAuraEntryWidget> ResolveAuraEntryWidgetClass();
TSubclassOf<URoundHoverInfoWidget> ResolveDefaultHoverInfoWidgetClass();
UDataTable* ResolveFormatTable();
TSoftObjectPtr<UWorld> ResolveMatchMap();
TSubclassOf<AActor> ResolveAimVisualActorClass();
}
