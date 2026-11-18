#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundInitiativeIconWidget.generated.h"

class ARoundCharacter;
class FReply;
class UImage;
class URoundInitiativeBarWidget;

UCLASS(Blueprintable)
class ROUND_API URoundInitiativeIconWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<ARoundCharacter> Character = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<URoundInitiativeBarWidget> InitativeBar = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SetInitiativeBar(URoundInitiativeBarWidget* InBar);

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void UpdatePosition();

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void Refresh();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_33 = nullptr;

private:
    void RefreshVisuals();
    UTexture2D* ResolveInitiativeTexture() const;
};
