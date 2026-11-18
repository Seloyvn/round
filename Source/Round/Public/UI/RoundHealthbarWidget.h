#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundHealthbarWidget.generated.h"

class ARoundCharacter;
class UProgressBar;
class UTextBlock;

UCLASS(Blueprintable)
class ROUND_API URoundHealthbarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<ARoundCharacter> Character = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SetCharacter(ARoundCharacter* InCharacter);

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void UpdateName();

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void UpdateHealth();

protected:
    virtual void NativeConstruct() override;

    UFUNCTION()
    void HandleHealthChanged(int32 NewHealth, int32 Delta);

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> ProgressBar_Health = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_Name = nullptr;
};
