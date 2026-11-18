#pragma once

#include "CoreMinimal.h"
#include "RoundTypes.h"
#include "UObject/Interface.h"
#include "RoundDeviceInterface.generated.h"

UENUM(BlueprintType)
enum class ERoundDeviceTriggerType : uint8
{
    Manual,
    Timed,
    OnEnter,
    OnLeave,
    Proximity,
    OnDestroyed
};

USTRUCT(BlueprintType)
struct ROUND_API FRoundDeviceTriggerContext
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Device")
    ERoundDeviceTriggerType TriggerType = ERoundDeviceTriggerType::Manual;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Device")
    TObjectPtr<class ARoundCharacter> TriggeringCharacter = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Device")
    TObjectPtr<class ARoundAbilityActor> TriggeringAbility = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Device")
    TObjectPtr<class ARoundGridTile> TriggerTile = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|Device")
    int32 InitiativeAmount = 0;
};

UINTERFACE(BlueprintType)
class ROUND_API URoundDeviceInterface : public UInterface
{
    GENERATED_BODY()
};

class ROUND_API IRoundDeviceInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Round|Device")
    bool CanTriggerDevice(const FRoundDeviceTriggerContext& TriggerContext) const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Round|Device")
    FRoundActionResult TriggerDevice(const FRoundDeviceTriggerContext& TriggerContext);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Round|Device")
    class ARoundCharacter* GetDeviceOwnerCharacter() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Round|Device")
    class ARoundGridTile* GetDeviceTile() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Round|Device")
    bool BlocksCharactersOnTile() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Round|Device")
    TArray<class ARoundGridTile*> GetDeviceVisionFields() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Round|Device")
    bool IsDeviceVisibleToTeam(ERoundTeam ViewingTeam, bool bTileInVision) const;
};
