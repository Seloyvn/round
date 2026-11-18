#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundTypes.h"
#include "RoundChampionCustomUIWidget.generated.h"

class ARoundCharacter;
class ARoundAbilityActor;
class UListView;
class UHorizontalBox;
class UImage;
class UTextBlock;
class URoundGameUIWidget;
class URoundHoverInfoWidget;
class URoundSkillHoverListItemData;

UCLASS(Blueprintable)
class ROUND_API URoundChampionCustomUIWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<ARoundCharacter> Character = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void SetCharacter(ARoundCharacter* InCharacter);

    UFUNCTION(BlueprintCallable, Category = "Round|UI")
    virtual void UpdateUI();

    void SetOwningGameUI(URoundGameUIWidget* InOwningGameUI);
    virtual FText GetHoverDescriptionText() const;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Cards")
    virtual bool BeginAbilityHandCardSelection(int32 AbilityIndex, ARoundAbilityActor* Ability);

    UFUNCTION(BlueprintPure, Category = "Round|UI|Cards")
    virtual bool IsHandCardSelectionActive() const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|Cards")
    virtual int32 GetPendingHandCardSelectionAbilityIndex() const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|Cards")
    virtual int32 GetSelectedHandCardCount() const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|Cards")
    virtual bool IsHandCardSelected(int32 CardIndex) const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|Cards")
    virtual bool IsHandCardSelectable(int32 CardIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Cards")
    virtual void ToggleHandCardSelection(int32 CardIndex);

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Cards")
    virtual void ConfirmHandCardSelection();

    UFUNCTION(BlueprintCallable, Category = "Round|UI|Cards")
    virtual void CancelHandCardSelection();

    UFUNCTION(BlueprintPure, Category = "Round|UI|Cards")
    virtual TArray<FRoundPlayingCard> GetVisibleHandCards() const;

    UFUNCTION(BlueprintPure, Category = "Round|UI|Cards")
    virtual FText GetHandCardDisplayText(int32 CardIndex) const;

protected:
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

    void RefreshSkillHoverList();
    FText BuildPassiveDescriptionText() const;
    FText BuildPassiveDetailsText() const;
    UUserWidget* CreateHoverInfoWidget(const FText& Title, const FText& Description, const FText& Details) const;
    void ApplyPassiveHoverTooltip();
    void ShowPassiveHoverWidgetAtScreenPosition(const FVector2D& ScreenPosition);
    void HidePassiveHoverWidget();

    TWeakObjectPtr<ARoundAbilityActor> PendingHandSelectionAbility;
    TArray<int32> PendingHandSelectionIndices;
    int32 PendingHandSelectionAbilityIndex = INDEX_NONE;
    int32 PendingHandSelectionMin = 0;
    int32 PendingHandSelectionMax = 0;
    bool bHandCardSelectionActive = false;

private:
    UFUNCTION()
    void HandleSkillItemHovered(UObject* Item, bool bIsHovered);

    UPROPERTY(Transient)
    TObjectPtr<UListView> ListView_Skills = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<URoundSkillHoverListItemData>> SkillListItems;

    TWeakObjectPtr<URoundGameUIWidget> OwningGameUI;
    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> PassiveHoverTooltip = nullptr;
    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> ActiveHoverViewportWidget = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API URoundChampion1CustomUIWidget : public URoundChampionCustomUIWidget
{
    GENERATED_BODY()

public:
    virtual void UpdateUI() override;
    virtual FText GetHoverDescriptionText() const override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API URoundChampion2CustomUIWidget : public URoundChampionCustomUIWidget
{
    GENERATED_BODY()

public:
    virtual void UpdateUI() override;
    virtual FText GetHoverDescriptionText() const override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API URoundChampion3CustomUIWidget : public URoundChampionCustomUIWidget
{
    GENERATED_BODY()

public:
    virtual void UpdateUI() override;
    virtual FText GetHoverDescriptionText() const override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API URoundChampion4CustomUIWidget : public URoundChampionCustomUIWidget
{
    GENERATED_BODY()

public:
    virtual void UpdateUI() override;
    virtual FText GetHoverDescriptionText() const override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_236 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_Fire = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_Ice = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_Lightning = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_Rock = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API URoundChampion5CustomUIWidget : public URoundChampionCustomUIWidget
{
    GENERATED_BODY()

public:
    virtual void UpdateUI() override;
    virtual FText GetHoverDescriptionText() const override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> Image_236 = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_55 = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API URoundChampion6CustomUIWidget : public URoundChampionCustomUIWidget
{
    GENERATED_BODY()

public:
    virtual void UpdateUI() override;
    virtual FText GetHoverDescriptionText() const override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock_55 = nullptr;

    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};

UCLASS(Blueprintable)
class ROUND_API URoundChampion6PuppetWidget : public URoundChampion6CustomUIWidget
{
    GENERATED_BODY()
};

UCLASS(Blueprintable)
class ROUND_API URoundChampionClericCustomUIWidget : public URoundChampionCustomUIWidget
{
    GENERATED_BODY()

public:
    virtual void UpdateUI() override;
    virtual FText GetHoverDescriptionText() const override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API URoundChampionBladecallerCustomUIWidget : public URoundChampionCustomUIWidget
{
    GENERATED_BODY()

public:
    virtual void UpdateUI() override;
    virtual FText GetHoverDescriptionText() const override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API URoundChampionChronomancerCustomUIWidget : public URoundChampionCustomUIWidget
{
    GENERATED_BODY()

public:
    virtual void UpdateUI() override;
    virtual FText GetHoverDescriptionText() const override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API URoundChampionDruidCustomUIWidget : public URoundChampionCustomUIWidget
{
    GENERATED_BODY()

public:
    virtual void UpdateUI() override;
    virtual FText GetHoverDescriptionText() const override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API URoundChampionDragoonCustomUIWidget : public URoundChampionCustomUIWidget
{
    GENERATED_BODY()

public:
    virtual void UpdateUI() override;
    virtual FText GetHoverDescriptionText() const override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API URoundChampionGeomancerCustomUIWidget : public URoundChampionCustomUIWidget
{
    GENERATED_BODY()

public:
    virtual void UpdateUI() override;
    virtual FText GetHoverDescriptionText() const override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;
};

UCLASS(Blueprintable)
class ROUND_API URoundChampionGamblerCustomUIWidget : public URoundChampionCustomUIWidget
{
    GENERATED_BODY()

public:
    virtual void UpdateUI() override;
    virtual FText GetHoverDescriptionText() const override;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TextBlock = nullptr;
};
