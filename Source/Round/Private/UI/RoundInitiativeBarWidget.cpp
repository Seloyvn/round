#include "UI/RoundInitiativeBarWidget.h"

#include "Actors/RoundCharacter.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "UI/RoundInitiativeIconWidget.h"
#include "UI/RoundObjectiveMarkerWidget.h"

void URoundInitiativeBarWidget::AddIcon(UUserWidget* Icon)
{
    if (!CanvasPanel_34 || !IsValid(Icon))
    {
        return;
    }

    CanvasPanel_34->AddChild(Icon);

    if (URoundInitiativeIconWidget* InitiativeIcon = Cast<URoundInitiativeIconWidget>(Icon))
    {
        InitiativeIcon->SetInitiativeBar(this);
        return;
    }

    static const FName InitiativeBarPropertyName(TEXT("InitativeBar"));
    if (FObjectProperty* InitiativeBarProperty = FindFProperty<FObjectProperty>(Icon->GetClass(), InitiativeBarPropertyName))
    {
        InitiativeBarProperty->SetObjectPropertyValue_InContainer(Icon, this);
    }
}

void URoundInitiativeBarWidget::ClearIcons()
{
    if (!CanvasPanel_34)
    {
        return;
    }

    for (int32 ChildIndex = CanvasPanel_34->GetChildrenCount() - 1; ChildIndex >= 0; --ChildIndex)
    {
        UWidget* Child = CanvasPanel_34->GetChildAt(ChildIndex);
        if (!Child || Child->IsA<UImage>())
        {
            continue;
        }

        CanvasPanel_34->RemoveChildAt(ChildIndex);
    }

    CharacterIcons.Reset();
}

void URoundInitiativeBarWidget::SyncIcons(const TArray<ARoundCharacter*>& Characters, TSubclassOf<URoundInitiativeIconWidget> IconClass, APlayerController* OwningController)
{
    if (!CanvasPanel_34 || !IconClass)
    {
        return;
    }

    TSet<TObjectPtr<ARoundCharacter>> ValidCharacters;
    for (ARoundCharacter* Character : Characters)
    {
        if (!Character || !Character->IsAlive())
        {
            continue;
        }

        ValidCharacters.Add(Character);

        URoundInitiativeIconWidget* Icon = CharacterIcons.FindRef(Character);
        if (!Icon)
        {
            Icon = CreateWidget<URoundInitiativeIconWidget>(this, IconClass);
            if (!Icon)
            {
                continue;
            }

            Icon->Character = Character;
            if (OwningController)
            {
                Icon->SetOwningPlayer(OwningController);
            }

            AddIcon(Icon);
            CharacterIcons.Add(Character, Icon);
        }

        Icon->Character = Character;
        Icon->SetVisibility(ESlateVisibility::Visible);
        Icon->Refresh();
    }

    TArray<TObjectPtr<ARoundCharacter>> CharactersToRemove;
    for (const TPair<TObjectPtr<ARoundCharacter>, TObjectPtr<URoundInitiativeIconWidget>>& Pair : CharacterIcons)
    {
        if (!Pair.Key || !ValidCharacters.Contains(Pair.Key))
        {
            if (Pair.Value)
            {
                Pair.Value->RemoveFromParent();
            }
            CharactersToRemove.Add(Pair.Key);
        }
    }

    for (TObjectPtr<ARoundCharacter> Character : CharactersToRemove)
    {
        CharacterIcons.Remove(Character);
    }
}

void URoundInitiativeBarWidget::SyncObjectiveMarkers(const TArray<FRoundObjectivePreviewEntry>& Entries, const int32 CurrentGlobalInitiative)
{
    if (!CanvasPanel_34)
    {
        return;
    }

    double BarWidth = 0.0;
    if (UCanvasPanelSlot* BarSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(this))
    {
        BarWidth = BarSlot->GetSize().X;
    }
    if (BarWidth <= 0.0)
    {
        BarWidth = GetDesiredSize().X;
    }
    if (BarWidth <= 0.0)
    {
        return;
    }

    TSet<FString> ValidKeys;
    for (const FRoundObjectivePreviewEntry& Entry : Entries)
    {
        const FString Key = FString::Printf(TEXT("%d_%d_%s"), static_cast<int32>(Entry.ObjectiveType), Entry.SpawnAtGlobalInitiative, *Entry.RewardName);
        ValidKeys.Add(Key);

        URoundObjectiveMarkerWidget* Marker = ObjectiveMarkers.FindRef(Key);
        if (!Marker)
        {
            Marker = CreateWidget<URoundObjectiveMarkerWidget>(GetOwningPlayer(), URoundObjectiveMarkerWidget::StaticClass());
            if (!Marker)
            {
                continue;
            }

            CanvasPanel_34->AddChild(Marker);
            ObjectiveMarkers.Add(Key, Marker);
        }

        const TCHAR* TypeLabel = TEXT("Objective");
        switch (Entry.ObjectiveType)
        {
        case ERoundObjectiveType::HoldZone:
            TypeLabel = TEXT("Hold");
            break;
        case ERoundObjectiveType::DestroyNode:
            TypeLabel = TEXT("Destroy");
            break;
        case ERoundObjectiveType::Channel:
            TypeLabel = TEXT("Channel");
            break;
        default:
            break;
        }

        Marker->InitializeMarker(
            this,
            Entry.TargetTile,
            TypeLabel,
            Entry.RewardName.IsEmpty()
                ? FString::Printf(TEXT("%s in %d initiative"), TypeLabel, FMath::Max(0, Entry.SpawnAtGlobalInitiative - CurrentGlobalInitiative))
                : FString::Printf(TEXT("%s in %d initiative. Reward: %s. %s"), TypeLabel, FMath::Max(0, Entry.SpawnAtGlobalInitiative - CurrentGlobalInitiative), *Entry.RewardName, *Entry.Description));
        Marker->SetVisibility(ESlateVisibility::Visible);

        const int32 Distance = FMath::Max(0, Entry.SpawnAtGlobalInitiative - CurrentGlobalInitiative);
        const double ClampedDistance = static_cast<double>(FMath::Clamp(Distance, 0, 150));
        const double NormalizedPosition = ClampedDistance / 150.0;
        Marker->UpdatePosition(static_cast<float>(BarWidth * NormalizedPosition));
    }

    TArray<FString> KeysToRemove;
    for (const TPair<FString, TObjectPtr<URoundObjectiveMarkerWidget>>& Pair : ObjectiveMarkers)
    {
        if (!ValidKeys.Contains(Pair.Key))
        {
            if (Pair.Value)
            {
                Pair.Value->RemoveFromParent();
            }
            KeysToRemove.Add(Pair.Key);
        }
    }

    for (const FString& Key : KeysToRemove)
    {
        ObjectiveMarkers.Remove(Key);
    }
}
