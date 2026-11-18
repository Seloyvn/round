#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundTypes.h"
#include "RoundAbilityData.generated.h"

UCLASS(BlueprintType)
class ROUND_API URoundAbilityData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    TSubclassOf<class ARoundAbilityActor> AbilityClass = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    TObjectPtr<class URoundSkillProgressionData> SkillProgression = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    TMap<FString, double> Values;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    FName AbilityId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    int32 AbilityNumericId = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    int32 InitiativeCost = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    int32 Cooldown = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    int32 Range = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    int32 BaseDamage = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    ERoundDamageType DamageType = ERoundDamageType::Physical;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    ERoundFieldState FieldState = ERoundFieldState::Any;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    bool bRequiresTargetCharacter = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    bool bRequiresTargetTile = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<class URoundPresentationProfileData> PresentationProfile = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<class URoundSkillHoverInfoWidget> SkillHoverWidgetClass;
};
