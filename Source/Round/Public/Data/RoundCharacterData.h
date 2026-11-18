#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundTypes.h"
#include "RoundCharacterData.generated.h"

class ARoundCharacter;
class URoundAbilityData;
class UDataTable;
class UTexture2D;
class UUserWidget;
class UObject;
class URoundCharacterProgressionData;
class URoundHoverInfoWidget;
class URoundPresentationProfileData;

UCLASS(BlueprintType)
class ROUND_API URoundCharacterData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    TMap<FString, double> Details;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    FName CharacterId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    int32 CharacterNumericId = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    FRoundStatBlock BaseStats;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    TArray<TObjectPtr<URoundAbilityData>> Abilities;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    TSubclassOf<class ARoundCharacter> CharacterClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    TSubclassOf<UUserWidget> CustomUIClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    TSubclassOf<UUserWidget> CharacterSkillpointMenuClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    TSubclassOf<URoundHoverInfoWidget> HoverInfoWidgetClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    TSubclassOf<UObject> SkillMenuClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    TObjectPtr<UTexture2D> InitiativeIcon = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    TObjectPtr<URoundCharacterProgressionData> CharacterProgression = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character")
    TObjectPtr<UDataTable> SkillpointsTable = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<URoundPresentationProfileData> MovePresentationProfile = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<URoundPresentationProfileData> BasicAttackPresentationProfile = nullptr;
};
