#include "Actors/RoundCharacterPaladin.h"

#include "Effects/RoundPaladinAuraEffect.h"
#include "Data/RoundCharacterData.h"

ARoundCharacterPaladin::ARoundCharacterPaladin()
{
}

double ARoundCharacterPaladin::GetAuraDamageReduction() const
{
    const double BaseValue = CharacterData && CharacterData->Details.Contains(TEXT("AuraDamageReduction"))
        ? CharacterData->Details[TEXT("AuraDamageReduction")] : 0.15;
    return FMath::Clamp(
        BaseValue + GetPassiveValueFromKey(TEXT("AuraDamageReduction")),
        0.0,
        0.95);
}

int32 ARoundCharacterPaladin::GetAuraRadius() const
{
    const double BaseValue = CharacterData && CharacterData->Details.Contains(TEXT("AuraRadius"))
        ? CharacterData->Details[TEXT("AuraRadius")] : 2.0;
    return FMath::Max(1, FMath::RoundToInt(BaseValue + GetPassiveValueFromKey(TEXT("AuraRadius"))));
}

void ARoundCharacterPaladin::InitializeCharacter()
{
    Super::InitializeCharacter();
    EnsurePassiveAura();
}
void ARoundCharacterPaladin::EnsurePassiveAura()
{
    if (!HasAuthority() || !GetWorld())
    {
        return;
    }

    if (!PassiveAuraEffect || !IsValid(PassiveAuraEffect))
    {
        PassiveAuraEffect = GetWorld()->SpawnActor<ARoundPaladinAuraEffect>(ARoundPaladinAuraEffect::StaticClass());
        if (!PassiveAuraEffect)
        {
            return;
        }

        PassiveAuraEffect->EffectId = TEXT("PaladinPassiveAura");
        PassiveAuraEffect->DisplayName = TEXT("Paladin Aura");
        PassiveAuraEffect->Description = TEXT("Verbuendete in Reichweite erleiden weniger Schaden.");
        PassiveAuraEffect->RemainingInitiative = 0;
        PassiveAuraEffect->RemainingTurns = 0;
        PassiveAuraEffect->bAutoExpireOnTurnEnd = false;
        AddEffect(PassiveAuraEffect);
    }

    PassiveAuraEffect->Radius = GetAuraRadius();
    PassiveAuraEffect->bAffectAllies = true;
    PassiveAuraEffect->bAffectEnemies = false;
    PassiveAuraEffect->bAffectSelf = GetPassiveValueFromKey(TEXT("AuraAffectSelf")) > 0.0;
    PassiveAuraEffect->CauserCharacter = this;
}
