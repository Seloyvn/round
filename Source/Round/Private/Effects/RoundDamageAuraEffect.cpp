#include "Effects/RoundDamageAuraEffect.h"

#include "Actors/RoundCharacter.h"
#include "EngineUtils.h"

void ARoundDamageAuraEffect::OnInitiativeAdvanced(ARoundCharacter* TargetCharacter, int32 InitiativeAmount)
{
    Super::OnInitiativeAdvanced(TargetCharacter, InitiativeAmount);

    if (InitiativeAmount <= 0 || AuraTriggerDamage <= 0 || !GetWorld())
    {
        return;
    }

    for (int32 Step = 0; Step < InitiativeAmount; ++Step)
    {
        for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
        {
            ARoundCharacter* Character = *It;
            if (!Character || !Character->IsAlive() || !AffectsCharacter(Character))
            {
                continue;
            }

            bool bDidDie = false;
            Character->ApplyRoundDamage(AuraTriggerDamage, AuraTriggerDamageType, 0, CauserCharacter ? CauserCharacter : OwningCharacter, bDidDie);
        }
    }
}
