#include "UI/RoundDebugWidget.h"

#include "Game/RoundPlayerPawn.h"
#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"

void URoundDebugWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (Button_FullHeal) Button_FullHeal->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleFullHeal);
    if (Button_Damage) Button_Damage->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDamage);
    if (Button_InitiativePlus) Button_InitiativePlus->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleInitiativePlus);
    if (Button_InitiativeMinus) Button_InitiativeMinus->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleInitiativeMinus);
    if (Button_ResetCooldowns) Button_ResetCooldowns->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleResetCooldowns);
    if (Button_Close) Button_Close->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleClose);
    if (Check_RevealMap) Check_RevealMap->OnCheckStateChanged.AddUniqueDynamic(this, &ThisClass::HandleRevealChanged);
    if (Check_TerrainLabels) Check_TerrainLabels->OnCheckStateChanged.AddUniqueDynamic(this, &ThisClass::HandleTerrainLabelsChanged);
    RefreshDebugInfo();
}

void URoundDebugWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

void URoundDebugWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    RefreshAccumulator += InDeltaTime;
    if (RefreshAccumulator >= 0.2f)
    {
        RefreshAccumulator = 0.0f;
        RefreshDebugInfo();
    }
}

void URoundDebugWidget::SetPlayerPawn(ARoundPlayerPawn* InPlayer)
{
    Player = InPlayer;
    RefreshDebugInfo();
}

void URoundDebugWidget::RefreshDebugInfo()
{
    if (!Player) return;
    ARoundGridTile* Tile = Player->HoveredField ? Player->HoveredField.Get() : Player->SelectedField.Get();
    if (Text_TileInfo)
    {
        const FString Value = Tile
            ? FString::Printf(TEXT("Feld %d / %d\nTerrain: %s\nBewegungskosten: %d\nBegehbar: %s\nBlockiert Sicht: %s"),
                Tile->GridCoordinate.X, Tile->GridCoordinate.Y, *UEnum::GetValueAsString(Tile->TerrainType),
                Tile->GetTerrainMoveCost(), Tile->IsMoveable() ? TEXT("Ja") : TEXT("Nein"), Tile->bBlocksVision ? TEXT("Ja") : TEXT("Nein"))
            : TEXT("Kein Feld ausgewählt");
        Text_TileInfo->SetText(FText::FromString(Value));
    }
    if (Text_CharacterInfo)
    {
        ARoundCharacter* Character = Player->GetDebugSelectedCharacter();
        const FString Value = Character && Character->Attributes
            ? FString::Printf(TEXT("%s\nTeam: %s\nLeben: %d / %d\nInitiative: %d"), *Character->CharacterRowName.ToString(),
                *UEnum::GetValueAsString(Character->Team), Character->Attributes->CurrentHealth,
                Character->Attributes->BaseStats.MaxHealth, Character->Attributes->CurrentInitiative)
            : TEXT("Kein Charakter ausgewählt");
        Text_CharacterInfo->SetText(FText::FromString(Value));
    }
    if (Check_RevealMap && Check_RevealMap->IsChecked() != Player->bDebugRevealMap) Check_RevealMap->SetIsChecked(Player->bDebugRevealMap);
    if (Check_TerrainLabels && Check_TerrainLabels->IsChecked() != Player->bDebugTerrainLabels) Check_TerrainLabels->SetIsChecked(Player->bDebugTerrainLabels);
}

void URoundDebugWidget::HandleRevealChanged(const bool bChecked) { if (Player) Player->SetDebugRevealMap(bChecked); }
void URoundDebugWidget::HandleTerrainLabelsChanged(const bool bChecked) { if (Player) Player->SetDebugTerrainLabels(bChecked); }
void URoundDebugWidget::HandleFullHeal() { if (Player) Player->DebugFullHeal(); }
void URoundDebugWidget::HandleDamage() { if (Player) Player->DebugApplyDamage(); }
void URoundDebugWidget::HandleInitiativePlus() { if (Player) Player->DebugAdjustInitiative(true); }
void URoundDebugWidget::HandleInitiativeMinus() { if (Player) Player->DebugAdjustInitiative(false); }
void URoundDebugWidget::HandleResetCooldowns() { if (Player) Player->DebugResetCooldowns(); }
void URoundDebugWidget::HandleClose() { if (Player) Player->ToggleDebugMode(); }
