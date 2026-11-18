#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundStartPositionWidget.generated.h"

class URoundMainMenuWidget;
class URoundSkillMenuWidget;
class URoundStartFieldWidget;
class UCanvasPanel;

UCLASS(Blueprintable)
class ROUND_API URoundStartPositionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    URoundStartPositionWidget(const FObjectInitializer& ObjectInitializer);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|Start", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<URoundMainMenuWidget> MainMenu = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Start")
    void SetMainMenuWidget(URoundMainMenuWidget* InMainMenu);

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Start")
    void RegisterField(URoundStartFieldWidget* Field);

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Start")
    void Refresh();

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Start")
    void HandleFieldClicked(URoundStartFieldWidget* Field);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|UI|Start")
    TArray<TObjectPtr<URoundStartFieldWidget>> Fields;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> CanvasPanel_58 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_1 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_2 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_3 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_4 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_5 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_6 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_7 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_8 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_9 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_10 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_11 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_12 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_13 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_14 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_15 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_16 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_17 = nullptr;
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<URoundStartFieldWidget> UI_StartField_18 = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|Start")
    TSubclassOf<URoundStartFieldWidget> StartFieldWidgetClass;

private:
    TArray<URoundStartFieldWidget*> GetAllStartFields() const;
    void EnsureFieldsCreated();
};
