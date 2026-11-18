#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundGameDataRegistry.generated.h"

class URoundAbilityRegistryData;
class URoundAuraEntryWidget;
class URoundCharacterSelectionOptionWidget;
class URoundCharacterRegistryData;
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
class UMaterialInterface;

UCLASS(BlueprintType)
class ROUND_API URoundGameDataRegistry : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Registry")
    TSoftObjectPtr<URoundCharacterRegistryData> CharacterRegistry;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Registry")
    TSoftObjectPtr<URoundAbilityRegistryData> AbilityRegistry;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Registry")
    TSoftObjectPtr<URoundTerrainRegistryData> TerrainRegistry;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Registry")
    TSoftObjectPtr<URoundTerrainRegionRegistryData> TerrainRegionRegistry;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Gameplay")
    TSoftObjectPtr<URoundTeamPresetData> DefaultTeamPresets;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TSoftClassPtr<URoundGameUIWidget> GameUIWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Debug")
    TSoftClassPtr<URoundDebugWidget> DebugWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Debug")
    bool bEnableDebugTools = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Debug", meta=(ClampMin="1"))
    int32 DebugDamageStep = 100;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Debug", meta=(ClampMin="1"))
    int32 DebugInitiativeStep = 20;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Debug")
    TSoftObjectPtr<UMaterialInterface> DebugRegionOverlayMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TSoftClassPtr<URoundHealthbarWidget> HealthbarWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TSoftClassPtr<URoundMainMenuWidget> MainMenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TSoftClassPtr<URoundTeamsListEntryWidget> TeamEntryWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TSoftClassPtr<URoundCharacterSelectionOptionWidget> CharacterSelectionOptionWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TSoftClassPtr<URoundSkillMenuWidget> SkillMenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TSoftClassPtr<URoundSkillpointMenuWidget> SkillpointMenuWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TSoftClassPtr<URoundSkillmenuSelectorPointWidget> SkillSelectorPointWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TSoftClassPtr<URoundStartFieldWidget> StartFieldWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TSoftClassPtr<URoundInitiativeIconWidget> InitiativeIconWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TSoftClassPtr<URoundAuraEntryWidget> AuraEntryWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TSoftClassPtr<URoundHoverInfoWidget> DefaultHoverInfoWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Gameplay")
    TSoftObjectPtr<UDataTable> FormatTable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Gameplay")
    TSoftObjectPtr<UWorld> MatchMap;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Gameplay")
    TSoftClassPtr<AActor> AimVisualActorClass;
};
