#include "UI/RoundEffectEntryWidget.h"

#include "Actors/RoundEffectActor.h"
#include "Components/TextBlock.h"
#include "UObject/UnrealType.h"

namespace
{
int32 GetDisplayedRemainingInitiative(const ARoundEffectActor* Effect)
{
    if (!Effect)
    {
        return 0;
    }

    return Effect->GetRemainingDurationInitiative();
}
}

void URoundEffectEntryWidget::SetEffect(ARoundEffectActor* InEffect)
{
    Effect = InEffect;
    RefreshFromEffect();
}

void URoundEffectEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
    IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
    Effect = Cast<ARoundEffectActor>(ListItemObject);
    RefreshFromEffect();
}

void URoundEffectEntryWidget::RefreshFromEffect()
{
    if (!TextBlock_203 || !IsValid(Effect))
    {
        return;
    }

    TextBlock_203->SetText(BuildLabelText());
    SetToolTipText(BuildTooltipText());
}

FText URoundEffectEntryWidget::BuildLabelText() const
{
    if (!IsValid(Effect))
    {
        return FText::GetEmpty();
    }

    const FString EffectName = !Effect->DisplayName.IsEmpty()
        ? Effect->DisplayName
        : (Effect->EffectId.IsNone() ? Effect->GetName() : Effect->EffectId.ToString());
    const int32 RemainingInitiative = GetDisplayedRemainingInitiative(Effect);
    return RemainingInitiative > 0
        ? FText::FromString(FString::Printf(TEXT("%s %d Ini"), *EffectName, RemainingInitiative))
        : FText::FromString(EffectName);
}

FText URoundEffectEntryWidget::BuildTooltipText() const
{
    if (!IsValid(Effect))
    {
        return FText::GetEmpty();
    }

    if (!Effect->Description.IsEmpty())
    {
        return FText::FromString(Effect->Description);
    }

    static const FName DescriptionPropertyName(TEXT("Description"));
    if (const FStrProperty* DescriptionProperty = FindFProperty<FStrProperty>(Effect->GetClass(), DescriptionPropertyName))
    {
        const FString RuntimeDescription = DescriptionProperty->GetPropertyValue_InContainer(Effect);
        if (!RuntimeDescription.IsEmpty())
        {
            return FText::FromString(RuntimeDescription);
        }
    }

    return FText::FromString(BuildGenericDescription());
}

FString URoundEffectEntryWidget::BuildGenericDescription() const
{
    if (!IsValid(Effect))
    {
        return FString();
    }

    const FString EffectName = !Effect->DisplayName.IsEmpty()
        ? Effect->DisplayName
        : (Effect->EffectId.IsNone() ? Effect->GetName() : Effect->EffectId.ToString());

    const int32 RemainingInitiative = GetDisplayedRemainingInitiative(Effect);
    if (RemainingInitiative > 0)
    {
        return FString::Printf(TEXT("%s fuer %d Initiative."), *EffectName, RemainingInitiative);
    }

    return EffectName;
}
