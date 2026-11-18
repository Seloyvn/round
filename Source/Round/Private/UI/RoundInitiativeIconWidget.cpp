#include "UI/RoundInitiativeIconWidget.h"

#include "Actors/RoundCharacter.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Data/RoundCharacterData.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Game/RoundPlayerPawn.h"
#include "Game/RoundGameState.h"
#include "Slate/SObjectWidget.h"
#include "UI/RoundInitiativeBarWidget.h"

void URoundInitiativeIconWidget::SetInitiativeBar(URoundInitiativeBarWidget* InBar)
{
    InitativeBar = InBar;
}

void URoundInitiativeIconWidget::UpdatePosition()
{
    UCanvasPanelSlot* IconSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(this);
    if (!IconSlot || !IsValid(Character))
    {
        return;
    }

    double BarWidth = 0.0;
    if (IsValid(InitativeBar))
    {
        if (UCanvasPanelSlot* BarSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(InitativeBar))
        {
            BarWidth = BarSlot->GetSize().X;
        }

        if (BarWidth <= 0.0)
        {
            BarWidth = InitativeBar->GetDesiredSize().X;
        }
    }

    if (BarWidth <= 0.0)
    {
        return;
    }

    const int32 MaxInitiative = Character->Attributes ? Character->Attributes->BaseStats.MaxInitiative : 100;
    const int32 DistanceToTurn = FMath::Max(0, MaxInitiative - Character->InitiativeToTurn());
    const double ClampedDistance = static_cast<double>(FMath::Clamp(DistanceToTurn, 0, 150));
    const double NormalizedPosition = ClampedDistance / 150.0;
    const FVector2D NewPosition(static_cast<float>(BarWidth * NormalizedPosition), IconSlot->GetPosition().Y);
    IconSlot->SetPosition(NewPosition);
}

void URoundInitiativeIconWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Refresh();
}

FReply URoundInitiativeIconWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

    if (!IsValid(Character) || Character->CurrentTile == nullptr)
    {
        return FReply::Unhandled();
    }

    APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController)
    {
        PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    }

    ARoundPlayerPawn* PlayerPawn = PlayerController ? Cast<ARoundPlayerPawn>(PlayerController->GetPawn()) : nullptr;
    if (!PlayerPawn)
    {
        return FReply::Unhandled();
    }

    PlayerPawn->SelectField(Character->CurrentTile);
    return FReply::Handled();
}

void URoundInitiativeIconWidget::Refresh()
{
    RefreshVisuals();
    UpdatePosition();
}

void URoundInitiativeIconWidget::RefreshVisuals()
{
    if (!IsValid(Character) || !Image_33)
    {
        return;
    }

    APlayerController* PlayerController = GetOwningPlayer();
    if (!PlayerController)
    {
        PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    }

    const ARoundPlayerPawn* PlayerPawn = PlayerController ? Cast<ARoundPlayerPawn>(PlayerController->GetPawn()) : nullptr;
    const bool bIsFriendly = PlayerPawn && PlayerPawn->Team == Character->Team;
    const ARoundGameState* GameState = Character->GetRoundGameState();
    const bool bIsActiveCharacter = GameState && GameState->GetActiveCharacter() == Character;
    const FLinearColor TintColor = bIsActiveCharacter
        ? FLinearColor(1.0f, 0.72f, 0.05f, 1.0f)
        : (bIsFriendly
            ? FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)
            : FLinearColor(1.0f, 0.0f, 0.124931f, 1.0f));

    Image_33->SetBrushTintColor(FSlateColor(TintColor));
    SetRenderScale(bIsActiveCharacter ? FVector2D(1.35f, 1.35f) : FVector2D(1.0f, 1.0f));

    if (bIsActiveCharacter)
    {
        const FString CharacterName = Character->CharacterData && !Character->CharacterData->DisplayName.IsEmpty()
            ? Character->CharacterData->DisplayName.ToString()
            : Character->CharacterRowName.ToString();
        SetToolTipText(FText::FromString(FString::Printf(TEXT("Am Zug: %s"), *CharacterName)));
    }
    else
    {
        SetToolTipText(FText::GetEmpty());
    }

    if (UTexture2D* InitiativeTexture = ResolveInitiativeTexture())
    {
        Image_33->SetBrushFromTexture(InitiativeTexture, false);
    }
}

UTexture2D* URoundInitiativeIconWidget::ResolveInitiativeTexture() const
{
    if (!IsValid(Character))
    {
        return nullptr;
    }

    if (Character->CharacterData && Character->CharacterData->InitiativeIcon)
    {
        return Character->CharacterData->InitiativeIcon;
    }

    static const FName ClassDetailsPropertyName(TEXT("ClassDetails"));
    static const FName InitiativeIconPropertyName(TEXT("InitativeIcon"));

    if (const FStructProperty* ClassDetailsProperty = FindFProperty<FStructProperty>(Character->GetClass(), ClassDetailsPropertyName))
    {
        const void* ClassDetailsPtr = ClassDetailsProperty->ContainerPtrToValuePtr<void>(Character);
        if (ClassDetailsPtr)
        {
            if (const FObjectProperty* InitiativeIconProperty = FindFProperty<FObjectProperty>(ClassDetailsProperty->Struct, InitiativeIconPropertyName))
            {
                return Cast<UTexture2D>(InitiativeIconProperty->GetObjectPropertyValue(InitiativeIconProperty->ContainerPtrToValuePtr<void>(ClassDetailsPtr)));
            }
        }
    }

    return nullptr;
}
