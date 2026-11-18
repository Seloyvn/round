#include "Actors/RoundCharacterCleric.h"

#include "Actors/RoundCharacter.h"
#include "Effects/RoundBlessingEffect.h"

ARoundCharacterCleric::ARoundCharacterCleric()
{
}

int32 ARoundCharacterCleric::GetBlessingDurationInitiative() const
{
    const int32 BaseValue = FMath::RoundToInt(GetValueFromKey(TEXT("BlessingDurationInitiative")));
    return FMath::Max(1, (BaseValue > 0 ? BaseValue : 60) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("BlessingDurationInitiative"))));
}

int32 ARoundCharacterCleric::GetBlessingMaxStacks() const
{
    const int32 BaseValue = FMath::RoundToInt(GetValueFromKey(TEXT("BlessingMaxStacks")));
    return FMath::Max(1, (BaseValue > 0 ? BaseValue : 5) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("BlessingMaxStacks"))));
}

int32 ARoundCharacterCleric::GetBlessingHealAmount() const
{
    const int32 BaseValue = FMath::RoundToInt(GetValueFromKey(TEXT("BlessingHeal")));
    return FMath::Max(1, (BaseValue > 0 ? BaseValue : 200) + FMath::RoundToInt(GetPassiveValueFromKey(TEXT("BlessingHeal"))));
}

ARoundBlessingEffect* ARoundCharacterCleric::FindBlessingEffectOn(ARoundCharacter* Ally) const
{
    if (!Ally)
    {
        return nullptr;
    }

    for (ARoundEffectActor* Effect : Ally->ActiveEffects)
    {
        ARoundBlessingEffect* BlessingEffect = Cast<ARoundBlessingEffect>(Effect);
        if (!BlessingEffect)
        {
            continue;
        }

        if (BlessingEffect->CauserCharacter == this)
        {
            return BlessingEffect;
        }
    }

    return nullptr;
}

ARoundBlessingEffect* ARoundCharacterCleric::BlessAlly(ARoundCharacter* Ally, int32 AddedStacks)
{
    if (!HasAuthority() || !GetWorld() || !Ally || Ally == this || Ally->Team != Team || AddedStacks <= 0)
    {
        return nullptr;
    }

    ARoundBlessingEffect* BlessingEffect = FindBlessingEffectOn(Ally);
    if (!BlessingEffect)
    {
        BlessingEffect = GetWorld()->SpawnActor<ARoundBlessingEffect>(ARoundBlessingEffect::StaticClass());
        if (!BlessingEffect)
        {
            return nullptr;
        }

        BlessingEffect->EffectId = TEXT("ClericBlessing");
        BlessingEffect->DisplayName = TEXT("Blessing");
        BlessingEffect->Description = TEXT("Wird durch Schutz und erlittenen Schaden verstaerkt und heilt bei voller Staerke.");
        BlessingEffect->CauserCharacter = this;
        BlessingEffect->MaxStacks = GetBlessingMaxStacks();
        BlessingEffect->HealAmount = GetBlessingHealAmount();
        BlessingEffect->StackHealThreshold = BlessingEffect->MaxStacks;
        BlessingEffect->SetDurationInitiative(GetBlessingDurationInitiative(), false);
        BlessingEffect->IntegerState.Add(TEXT("BlessingBaseDurationInitiative"), GetBlessingDurationInitiative());
        Ally->AddEffect(BlessingEffect);
    }
    else
    {
        BlessingEffect->MaxStacks = GetBlessingMaxStacks();
        BlessingEffect->HealAmount = GetBlessingHealAmount();
        BlessingEffect->StackHealThreshold = BlessingEffect->MaxStacks;
        BlessingEffect->IntegerState.Add(TEXT("BlessingBaseDurationInitiative"), GetBlessingDurationInitiative());
    }

    const int32 ExtraSpellStacks = FMath::Max(0, FMath::RoundToInt(GetPassiveValueFromKey(TEXT("AdditionalBlessingStacksPerSpell"))));
    BlessingEffect->AddBlessingStack(Ally, AddedStacks + ExtraSpellStacks, true);
    return BlessingEffect;
}
