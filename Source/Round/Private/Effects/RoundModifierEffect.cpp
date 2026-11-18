#include "Effects/RoundModifierEffect.h"

#include "Actors/RoundCharacter.h"
#include "Components/RoundAttributeSetComponent.h"

int32 ARoundModifierEffect::GetMovecost_Implementation(int32 In) const
{
    int32 Cost = In + MoveCostAdd;
    if (bBlockMovementBonuses && OwningCharacter && OwningCharacter->Attributes)
    {
        Cost = FMath::Max(Cost, OwningCharacter->Attributes->BaseStats.MoveCost);
    }

    return FMath::Max(1, Cost);
}

int32 ARoundModifierEffect::ModifyIncomingDamage(ARoundCharacter* TargetCharacter, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const
{
    if (bOnlyAffectCauserDamage && CauserCharacter && SourceCharacter != CauserCharacter)
    {
        return Damage;
    }

    if (bOnlyAgainstRangedSources && SourceCharacter && SourceCharacter->CurrentTile && TargetCharacter && TargetCharacter->CurrentTile)
    {
        if (SourceCharacter->CurrentTile->GetDistance(TargetCharacter->CurrentTile) <= 1)
        {
            return Damage;
        }
    }

    DefenseIgnore = FMath::Max(0, DefenseIgnore + DefenseIgnoreAdd);
    return FMath::Max(0, FMath::RoundToInt(static_cast<double>(Damage) * IncomingDamageMultiplier));
}

int32 ARoundModifierEffect::ModifyOutgoingDamage(ARoundCharacter* SourceCharacter, ARoundCharacter* TargetCharacter, int32 Damage, ERoundDamageType DamageType, int32& DefenseIgnore) const
{
    if (bOnlyAffectCauserDamage && CauserCharacter && TargetCharacter != CauserCharacter)
    {
        return Damage;
    }

    DefenseIgnore = FMath::Max(0, DefenseIgnore + DefenseIgnoreAdd);
    const int32 ModifiedDamage = FMath::Max(0, FMath::RoundToInt(static_cast<double>(Damage) * OutgoingDamageMultiplier));
    if (SourceCharacter && HealFractionFromDamageDealt > 0.0 && ModifiedDamage > 0)
    {
        SourceCharacter->Heal(FMath::Max(0, FMath::RoundToInt(static_cast<double>(ModifiedDamage) * HealFractionFromDamageDealt)));
    }

    return ModifiedDamage;
}
