#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundCharacterRegistryData.generated.h"

UCLASS(BlueprintType)
class ROUND_API URoundCharacterRegistryData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Runtime")
    TArray<TSoftObjectPtr<class URoundCharacterData>> RuntimeCharacters;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Import")
    FString CharacterDataDirectory = TEXT("/Game/TopDown/Characters");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Import")
    FString CharacterProgressionDirectory = TEXT("/Game/TopDown/Characters");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Import")
    FString ImportRelativeCsvPath = TEXT("currentexport/CharacterClassDetails.csv");

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void ImportFromCharacterClassDetails();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void SyncBaseStatsFromDetails();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void ImportCharacterProgressionFromLegacySkillpoints();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateCharacter1ProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateCharacter2ProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateCharacter3ProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateCharacter4ProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateCharacter5ProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateCharacter6ProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateEngineerProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateChronomancerProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateDruidProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateDragoonProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateGeomancerProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateBladecallerProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateClericProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulateGamblerProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void PopulatePaladinProgressionDefaults();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Round|Import")
    void EnsureFiveAbilityLoadouts();
};
