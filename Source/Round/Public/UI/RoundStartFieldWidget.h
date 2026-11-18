#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundStartFieldWidget.generated.h"

class UButton;
class UImage;
class URoundStartPositionWidget;

UCLASS(Blueprintable)
class ROUND_API URoundStartFieldWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|Start", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<URoundStartPositionWidget> main = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|Start")
    FVector2D Pos = FVector2D::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|UI|Start")
    bool Obstructed = false;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Start")
    void Refresh();

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Start")
    void SetColor(const FLinearColor& InColor);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_375 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_41 = nullptr;

private:
    UFUNCTION()
    void HandleClicked();
};
