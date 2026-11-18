#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "RoundEffectEntryWidget.generated.h"

class ARoundEffectActor;
class UImage;
class UTextBlock;

UCLASS(Blueprintable)
class ROUND_API URoundEffectEntryWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SetEffect(ARoundEffectActor* InEffect);

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|UI")
    TObjectPtr<ARoundEffectActor> Effect = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_49 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_203 = nullptr;

private:
    void RefreshFromEffect();
    FText BuildLabelText() const;
    FText BuildTooltipText() const;
    FString BuildGenericDescription() const;
};
