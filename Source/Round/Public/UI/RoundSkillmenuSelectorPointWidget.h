#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundTypes.h"
#include "RoundSkillmenuSelectorPointWidget.generated.h"

class UButton;
class UTextBlock;
class URoundSkillMenuWidget;
class URoundSkillpointMenuWidget;

UCLASS(Blueprintable)
class ROUND_API URoundSkillmenuSelectorPointWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<URoundSkillMenuWidget> Skillmenu = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<URoundSkillpointMenuWidget> SkillpointMenu = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu", meta = (ExposeOnSpawn = "true"))
    FString SkillName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu", meta = (ExposeOnSpawn = "true"))
    FRoundSkillDetails SkillDetail;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu", meta = (ExposeOnSpawn = "true"))
    int32 SelectorIndex = 0;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    void SetLearned(bool bLearned);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> Button_42 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_40 = nullptr;

private:
    UFUNCTION()
    void HandleClicked();
};
