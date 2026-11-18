#include "Effects/RoundBindEffect.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundModifierEffect.h"

int32 ARoundBindEffect::GetMovecost_Implementation(int32 In) const
{
    int32 MoveCost = In + 2;
    if (bBlockMovementBonusesOnTarget && OwningCharacter && OwningCharacter->Attributes)
    {
        MoveCost = FMath::Max(MoveCost, OwningCharacter->Attributes->BaseStats.MoveCost);
    }

    return FMath::Max(1, MoveCost);
}

void ARoundBindEffect::OnOwnerMoved(ARoundCharacter* TargetCharacter, ARoundGridTile* PreviousTile, ARoundGridTile* NewTile)
{
    if (!TargetCharacter || !PreviousTile || !NewTile || PreviousTile == NewTile)
    {
        return;
    }

    if (TriggerDamage > 0)
    {
        bool bDidDie = false;
        TargetCharacter->ApplyRoundDamage(TriggerDamage, ERoundDamageType::Physical, 0, CauserCharacter, bDidDie);
    }

    if (DelayAmount > 0)
    {
        TargetCharacter->ApplyDelay(DelayAmount);
    }

    if (TriggerOutgoingDamageMultiplier > 0.0 && TriggerOutgoingDamageMultiplier < 1.0 && TargetCharacter->GetWorld())
    {
        if (ARoundModifierEffect* ModifierEffect = TargetCharacter->GetWorld()->SpawnActor<ARoundModifierEffect>(ARoundModifierEffect::StaticClass()))
        {
            ModifierEffect->EffectId = TEXT("BindTriggerWeakness");
            ModifierEffect->DisplayName = TEXT("Bind");
            ModifierEffect->Description = TEXT("Verursacht nach dem Ausloesen weniger Schaden.");
            ModifierEffect->OutgoingDamageMultiplier = TriggerOutgoingDamageMultiplier;
            ModifierEffect->CauserCharacter = CauserCharacter;
            ModifierEffect->SetDurationInitiative(40, false);
            TargetCharacter->AddEffect(ModifierEffect);
        }
    }

    RemoveEffect();
}
