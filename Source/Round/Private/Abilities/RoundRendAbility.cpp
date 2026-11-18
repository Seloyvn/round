#include "Abilities/RoundRendAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacter5.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Effects/RoundBloodMarkEffect.h"
#include "Effects/RoundModifierEffect.h"
#include "Effects/RoundRendEffect.h"

FRoundActionResult ARoundRendAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    ARoundCharacter* InitialTarget = ResolveTargetCharacter(TargetTile);
    ARoundRendEffect* ExistingRend = nullptr;
    if (InitialTarget)
    {
        for (ARoundEffectActor* Effect : InitialTarget->ActiveEffects)
        {
            ARoundRendEffect* Rend = Cast<ARoundRendEffect>(Effect);
            if (Rend && Rend->CauserCharacter == OwningCharacter) { ExistingRend = Rend; break; }
        }
    }
    FRoundActionResult Result = ExecuteDefaultAbility(TargetTile);
    if (!Result.bSuccess || !OwningCharacter || !TargetTile)
    {
        return Result;
    }

    ARoundCharacter* TargetCharacter = ResolveTargetCharacter(TargetTile);
    if (!TargetCharacter)
    {
        return Result;
    }

    if (IsValid(ExistingRend) && TargetCharacter->IsAlive() && GetAbilityValueFromKey(TEXT("ImmediateBleedTickOnReapply")) > 0.0)
    {
        bool bDidDie = false;
        TargetCharacter->ApplyDirectDamage(FMath::Max(0, FMath::RoundToInt(ExistingRend->Amount)), ERoundDamageType::Physical, OwningCharacter, bDidDie);
    }
    if (IsValid(ExistingRend)) ExistingRend->RemoveEffect();

    const int32 BleedDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BleedDamagePerTurn"))));
    const int32 BleedDuration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BleedDurationInitiative"))));
    if (BleedDamage > 0 && BleedDuration > 0 && TargetCharacter->IsAlive())
    {
        if (ARoundRendEffect* Bleed = GetWorld()->SpawnActor<ARoundRendEffect>())
        {
            Bleed->DisplayName = TEXT("Rend"); Bleed->Description = TEXT("Verliert bei Zugbeginn Leben."); Bleed->Amount = BleedDamage;
            Bleed->BloodPerTick = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("BloodPerBleedTick"))));
            Bleed->MovementTriggerDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MovementTriggerDamage"))));
            Bleed->CauserCharacter = OwningCharacter; Bleed->SetDurationInitiative(BleedDuration, false); TargetCharacter->AddEffect(Bleed);
        }
    }

    const double LifestealFraction = FMath::Max(0.0, GetAbilityValueFromKey(TEXT("LifestealFractionOnHit")));
    if (LifestealFraction > 0.0)
    {
        OwningCharacter->Heal(FMath::Max(0, FMath::RoundToInt(static_cast<double>(GetAbilityDamage()) * LifestealFraction)));
    }

    if (ARoundCharacter5* Character5 = Cast<ARoundCharacter5>(OwningCharacter))
    {
        Character5->GainRawBlood(FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ImmediateBloodGain")))));
    }

    const int32 MoveCostAdd = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("MoveCostAddOnTarget"))));
    const int32 DebuffDuration = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("DebuffDurationInitiative"))));
    if (MoveCostAdd > 0 && DebuffDuration > 0 && TargetCharacter->IsAlive())
    {
        if (ARoundModifierEffect* Hamstring = GetWorld()->SpawnActor<ARoundModifierEffect>())
        {
            Hamstring->DisplayName = TEXT("Gerissene Sehne"); Hamstring->MoveCostAdd = MoveCostAdd; Hamstring->bTreatAsDebuff = true;
            Hamstring->CauserCharacter = OwningCharacter; Hamstring->SetDurationInitiative(DebuffDuration, false); TargetCharacter->AddEffect(Hamstring);
        }
    }

    const double MarkMultiplier = GetAbilityValueFromKey(TEXT("IncomingDamageMultiplierFromCauser"));
    if (MarkMultiplier > 0.0 && TargetCharacter->IsAlive())
    {
        if (ARoundBloodMarkEffect* Mark = GetWorld()->SpawnActor<ARoundBloodMarkEffect>())
        {
            Mark->DisplayName = TEXT("Offene Wunde"); Mark->DamageMultiplier = MarkMultiplier; Mark->CauserCharacter = OwningCharacter;
            Mark->SetDurationInitiative(FMath::Max(1, DebuffDuration), false); TargetCharacter->AddEffect(Mark);
        }
    }

    return Result;
}
