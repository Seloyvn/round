#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundCharacterSelectionOptionWidget.generated.h"

class UTextBlock;
class URoundMainMenuWidget;

UCLASS(Blueprintable)
class ROUND_API URoundCharacterSelectionOptionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI", meta = (ExposeOnSpawn = "true"))
    FName Champion = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI", meta = (ExposeOnSpawn = "true"))
    int32 SlotIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<URoundMainMenuWidget> MainMenu = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SetChampion(FName InChampion);

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_66 = nullptr;

private:
    void RefreshLabel();
};
