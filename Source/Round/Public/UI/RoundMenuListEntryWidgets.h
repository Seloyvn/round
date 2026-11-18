#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "RoundMenuListEntryWidgets.generated.h"

class UTextBlock;
class URoundMainMenuWidget;

UCLASS(Blueprintable)
class ROUND_API URoundTeamsListEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI", meta = (ExposeOnSpawn = "true"))
    FString TeamName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI", meta = (ExposeOnSpawn = "true"))
    int32 Index = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<URoundMainMenuWidget> MainMenu = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SetTeamName(const FString& InTeamName);

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_338 = nullptr;

private:
    void RefreshLabel();
};

UCLASS(Blueprintable)
class ROUND_API URoundSessionsListEntryWidget : public UUserWidget, public IUserObjectListEntry
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SetSessionLabel(const FString& InSessionLabel);

protected:
    virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|UI")
    FString SessionLabel;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_338 = nullptr;

private:
    void RefreshLabel();
};
