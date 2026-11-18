#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundSkillpointWidget.generated.h"

class UImage;
class URoundSkillpointMenuWidget;

UCLASS(Blueprintable)
class ROUND_API URoundSkillpointWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu")
    TObjectPtr<URoundSkillpointMenuWidget> SkillpointMenu = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu")
    int32 PointIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu")
    int32 TierIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu")
    int32 ChoiceIndex = 0;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    void Load();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_80 = nullptr;
};
