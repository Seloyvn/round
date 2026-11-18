#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundObjectiveMarkerWidget.generated.h"

class ARoundGridTile;
class UImage;
class URoundInitiativeBarWidget;

UCLASS()
class ROUND_API URoundObjectiveMarkerWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    void InitializeMarker(URoundInitiativeBarWidget* InOwnerBar, ARoundGridTile* InTargetTile, const FString& InLabel, const FString& InTooltip);
    void UpdatePosition(float XPosition);

private:
    void SelectTargetTile() const;

    UPROPERTY(Transient)
    TObjectPtr<URoundInitiativeBarWidget> OwnerBar = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<ARoundGridTile> TargetTile = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UImage> MarkerImage = nullptr;

    FString PendingLabel;
    FString PendingTooltip;
};
