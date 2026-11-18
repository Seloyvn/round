#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundAbilityRegistryData.generated.h"

class URoundAbilityData;

UCLASS(BlueprintType)
class ROUND_API URoundAbilityRegistryData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    URoundAbilityRegistryData();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import")
    FString ImportRelativeDirectory = TEXT("currentexport");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Registry")
    TArray<TObjectPtr<URoundAbilityData>> Entries;

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void ImportFromCurrentExport();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void RefreshEntriesFromAbilityAssets();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void NormalizeCharacter4MageDamageTypes();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateCharacter1AbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateCharacter2AbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateCharacter3AbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateCharacter4AbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateCharacter5AbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateCharacter6AbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateEngineerAbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateChronomancerAbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateDruidAbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateDragoonAbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateGeomancerAbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateBladecallerAbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateClericAbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulateGamblerAbilityProgressionDefaults();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "Import")
    void PopulatePaladinAbilityProgressionDefaults();

    UFUNCTION(BlueprintPure, Category = "Registry")
    URoundAbilityData* FindAbilityDataById(FName AbilityId) const;

    UFUNCTION(BlueprintPure, Category = "Registry")
    TSubclassOf<ARoundAbilityActor> FindAbilityClassById(FName AbilityId) const;
};
