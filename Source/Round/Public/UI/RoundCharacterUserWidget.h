#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundTypes.h"
#include "RoundCharacterUserWidget.generated.h"

class UTextBlock;
class URoundSkillMenuWidget;

UCLASS(Blueprintable)
class ROUND_API URoundCharacterUserWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<URoundSkillMenuWidget> SkillMenu = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu", meta = (ExposeOnSpawn = "true"))
    int32 CharacterIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu", meta = (ExposeOnSpawn = "true"))
    FRoundCharacterEntry CharacterEntry;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    void RefreshEntry();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_338 = nullptr;
};
