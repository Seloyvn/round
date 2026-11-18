#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundDebugWidget.generated.h"

class ARoundPlayerPawn;
class UButton;
class UCheckBox;
class UTextBlock;

UCLASS(Blueprintable)
class ROUND_API URoundDebugWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Round|Debug")
    void SetPlayerPawn(ARoundPlayerPawn* InPlayer);

    UFUNCTION(BlueprintCallable, Category="Round|Debug")
    void RefreshDebugInfo();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_TileInfo;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Text_CharacterInfo;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCheckBox> Check_RevealMap;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UCheckBox> Check_TerrainLabels;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_FullHeal;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Damage;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_InitiativePlus;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_InitiativeMinus;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_ResetCooldowns;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> Button_Close;

private:
    UFUNCTION() void HandleRevealChanged(bool bChecked);
    UFUNCTION() void HandleTerrainLabelsChanged(bool bChecked);
    UFUNCTION() void HandleFullHeal();
    UFUNCTION() void HandleDamage();
    UFUNCTION() void HandleInitiativePlus();
    UFUNCTION() void HandleInitiativeMinus();
    UFUNCTION() void HandleResetCooldowns();
    UFUNCTION() void HandleClose();

    UPROPERTY(Transient) TObjectPtr<ARoundPlayerPawn> Player;
    float RefreshAccumulator = 0.0f;
};
