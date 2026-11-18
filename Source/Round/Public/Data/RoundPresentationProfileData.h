#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoundPresentationProfileData.generated.h"

class USoundBase;

UCLASS(BlueprintType)
class ROUND_API URoundPresentationProfileData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    FLinearColor TextColor = FLinearColor::White;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    float TextHeightOffset = 125.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    float BeamThicknessScale = 0.14f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    FVector PulseStartScale = FVector(0.18f, 0.18f, 0.18f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    FVector PulseEndScale = FVector(0.44f, 0.44f, 0.44f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    float SoundVolumeMultiplier = 0.45f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    float SoundPitchMultiplier = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    float SoundMaxDurationSeconds = 0.12f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    float ScalePunch = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    FString TextOverride;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation")
    TObjectPtr<USoundBase> SoundOverride = nullptr;
};
