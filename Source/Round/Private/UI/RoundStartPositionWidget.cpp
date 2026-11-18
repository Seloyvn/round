#include "UI/RoundStartPositionWidget.h"

#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Data/RoundGameDataRegistryLookup.h"
#include "UI/RoundMainMenuWidget.h"
#include "UI/RoundSkillMenuWidget.h"
#include "UI/RoundStartFieldWidget.h"

namespace
{
    TArray<FVector2D> BuildStartFieldPositions()
    {
        TArray<FVector2D> Result;
        Result.Reserve(19);

        constexpr int32 Radius = 2;
        for (int32 Q = -Radius; Q <= Radius; ++Q)
        {
            const int32 RMin = FMath::Max(-Radius, -Q - Radius);
            const int32 RMax = FMath::Min(Radius, -Q + Radius);
            for (int32 R = RMin; R <= RMax; ++R)
            {
                Result.Add(FVector2D(Q, R));
            }
        }

        Result.Sort([](const FVector2D& A, const FVector2D& B)
        {
            if (A.Y == B.Y)
            {
                return A.X < B.X;
            }
            return A.Y < B.Y;
        });

        return Result;
    }

    FVector2D HexToCanvasPosition(const FVector2D& Hex)
    {
        constexpr float HorizontalSpacing = 56.f;
        constexpr float VerticalSpacing = 48.f;
        const FVector2D Center(160.f, 130.f);

        const float X = Center.X + ((Hex.X + Hex.Y * 0.5f) * HorizontalSpacing);
        const float Y = Center.Y + (Hex.Y * VerticalSpacing);
        return FVector2D(X, Y);
    }
}

URoundStartPositionWidget::URoundStartPositionWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void URoundStartPositionWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (!StartFieldWidgetClass)
    {
        StartFieldWidgetClass = Cast<UClass>(RoundGameDataRegistryLookup::ResolveStartFieldWidgetClass().Get());
    }

    if (!MainMenu)
    {
        if (URoundSkillMenuWidget* SkillMenu = GetTypedOuter<URoundSkillMenuWidget>())
        {
            MainMenu = SkillMenu->MainMenu;
        }
    }

    EnsureFieldsCreated();

    for (URoundStartFieldWidget* Field : GetAllStartFields())
    {
        RegisterField(Field);
    }

    Refresh();
}

void URoundStartPositionWidget::SetMainMenuWidget(URoundMainMenuWidget* InMainMenu)
{
    MainMenu = InMainMenu;
    Refresh();
}

void URoundStartPositionWidget::RegisterField(URoundStartFieldWidget* Field)
{
    if (!Field || Fields.Contains(Field))
    {
        return;
    }

    Field->main = this;
    Fields.Add(Field);
}

void URoundStartPositionWidget::Refresh()
{
    if (!MainMenu)
    {
        return;
    }

    const FRoundTeamDefinition& Team = MainMenu->GetEditTeamDefinition();
    const int32 SelectedCharacterIndex = MainMenu->GetCurrentSelectedCharacterIndex();

    for (URoundStartFieldWidget* Field : Fields)
    {
        if (!Field)
        {
            continue;
        }

        Field->Obstructed = false;
        bool bOccupiedBySelf = false;
        bool bOccupiedByOther = false;

        for (int32 Index = 0; Index < Team.Characters.Num(); ++Index)
        {
            const FRoundCharacterEntry& Character = Team.Characters[Index];
            if (!Character.StartPosition.Equals(Field->Pos))
            {
                continue;
            }

            if (Index == SelectedCharacterIndex)
            {
                bOccupiedBySelf = true;
            }
            else
            {
                bOccupiedByOther = true;
                Field->Obstructed = true;
            }
        }

        if (bOccupiedBySelf)
        {
            Field->SetColor(FLinearColor(0.2f, 0.75f, 1.f, 1.f));
        }
        else if (bOccupiedByOther)
        {
            Field->SetColor(FLinearColor(1.f, 0.45f, 0.2f, 1.f));
        }
        else
        {
            Field->SetColor(FLinearColor(0.25f, 0.25f, 0.25f, 1.f));
        }
    }
}

void URoundStartPositionWidget::HandleFieldClicked(URoundStartFieldWidget* Field)
{
    if (!MainMenu || !Field || Field->Obstructed)
    {
        return;
    }

    FRoundCharacterEntry Character;
    const int32 SelectedCharacterIndex = MainMenu->GetCurrentSelectedCharacterIndex();
    if (!MainMenu->TryGetEditedCharacter(SelectedCharacterIndex, Character))
    {
        return;
    }

    Character.StartPosition = Field->Pos;
    MainMenu->UpdateEditedCharacter(SelectedCharacterIndex, Character, true);
    Refresh();
}

TArray<URoundStartFieldWidget*> URoundStartPositionWidget::GetAllStartFields() const
{
    return {
        UI_StartField, UI_StartField_1, UI_StartField_2, UI_StartField_3, UI_StartField_4,
        UI_StartField_5, UI_StartField_6, UI_StartField_7, UI_StartField_8, UI_StartField_9,
        UI_StartField_10, UI_StartField_11, UI_StartField_12, UI_StartField_13, UI_StartField_14,
        UI_StartField_15, UI_StartField_16, UI_StartField_17, UI_StartField_18
    };
}

void URoundStartPositionWidget::EnsureFieldsCreated()
{
    if (!CanvasPanel_58 || !StartFieldWidgetClass)
    {
        return;
    }

    if (CanvasPanel_58->GetChildrenCount() > 0)
    {
        return;
    }

    const TArray<FVector2D> Positions = BuildStartFieldPositions();
    for (const FVector2D& Position : Positions)
    {
        URoundStartFieldWidget* Field = CreateWidget<URoundStartFieldWidget>(this, StartFieldWidgetClass);
        if (!Field)
        {
            continue;
        }

        Field->main = this;
        Field->Pos = Position;

        if (UCanvasPanelSlot* CanvasSlot = CanvasPanel_58->AddChildToCanvas(Field))
        {
            CanvasSlot->SetAutoSize(true);
            CanvasSlot->SetPosition(HexToCanvasPosition(Position));
        }

        RegisterField(Field);
    }
}
