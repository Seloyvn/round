#include "Abilities/RoundWildBloomAbility.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundCharacterDruid.h"
#include "Actors/RoundGridTile.h"

bool ARoundWildBloomAbility::CanTargetTile(const ARoundGridTile* TargetTile) const
{
    return GetAbilityVisibility() == ERoundButtonState::Enabled;
}

void ARoundWildBloomAbility::GetPreviewTiles(const ARoundGridTile* TargetTile, TArray<ARoundGridTile*>& OutTiles) const
{
    const ARoundCharacterDruid* Druid = Cast<ARoundCharacterDruid>(OwningCharacter);
    if (!Druid)
    {
        return;
    }

    OutTiles = Druid->GetConnectedNatureFields();
}

FRoundActionResult ARoundWildBloomAbility::ExecuteAbility(ARoundGridTile* TargetTile)
{
    FRoundActionResult Result;
    if (!OwningCharacter || !OwningCharacter->Attributes)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    if (GetAbilityVisibility() != ERoundButtonState::Enabled)
    {
        Result.Reason = TEXT("Ability target is not legal.");
        return Result;
    }

    if (!OwningCharacter->Attributes->SpendInitiative(GetAbilityCost()))
    {
        Result.Reason = TEXT("Not enough initiative.");
        return Result;
    }

    if (!ConsumeCharge())
    {
        Result.Reason = TEXT("No charges remaining.");
        return Result;
    }

    if (!UsesCharges())
    {
        SetCooldown(GetAbilityCooldown());
    }

    ARoundCharacterDruid* Druid = Cast<ARoundCharacterDruid>(OwningCharacter);
    if (!Druid)
    {
        Result.Reason = TEXT("Only druids can use Wild Bloom.");
        return Result;
    }

    const int32 ThornDamage = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ThornDamage"))));
    const int32 HealAmount = FMath::Max(
        0,
        FMath::RoundToInt(GetAbilityValueFromKey(TEXT("GroveHeal"))) > 0
            ? FMath::RoundToInt(GetAbilityValueFromKey(TEXT("GroveHeal")))
            : FMath::RoundToInt(GetAbilityValueFromKey(TEXT("Heal"))));
    const int32 ForestInitiativeGain = FMath::Max(
        0,
        FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ForestInitiativeGain"))) > 0
            ? FMath::RoundToInt(GetAbilityValueFromKey(TEXT("ForestInitiativeGain")))
            : FMath::RoundToInt(GetAbilityValueFromKey(TEXT("InitiativeGain"))));

    const TArray<ARoundGridTile*> ConnectedFields = Druid->GetConnectedNatureFields();
    int32 TriggeredFields = 0;
    TArray<ARoundGridTile*> FieldsToWither;
    for (ARoundGridTile* Field : ConnectedFields)
    {
        if (!Field)
        {
            continue;
        }

        if (GetAbilityValueFromKey(TEXT("WitherAfterBloom")) > 0.0) FieldsToWither.Add(Field);
        if (!Field->OccupyingCharacter) continue;

        switch (Field->TerrainType)
        {
        case ERoundTerrainType::Thorns:
        {
            if (!OwningCharacter->IsEnemyFor(Field->OccupyingCharacter)) break;
            bool bDidDie = false;
            Field->OccupyingCharacter->ApplyRoundDamage(ThornDamage > 0 ? ThornDamage : GetAbilityDamage(), GetAbilityDamageType(), 0, OwningCharacter, bDidDie);
            ++TriggeredFields;
            break;
        }
        case ERoundTerrainType::Grove:
            if (OwningCharacter->IsEnemyFor(Field->OccupyingCharacter)) break;
            Field->OccupyingCharacter->Heal(HealAmount > 0 ? HealAmount : FMath::Max(0, GetAbilityDamage()));
            ++TriggeredFields;
            break;
        case ERoundTerrainType::Forest:
            if (!OwningCharacter->IsEnemyFor(Field->OccupyingCharacter) && Field->OccupyingCharacter->Attributes)
            {
                Field->OccupyingCharacter->Attributes->GainInitiative(ForestInitiativeGain > 0 ? ForestInitiativeGain : 20);
                ++TriggeredFields;
            }
            break;
        default:
            break;
        }
    }

    const int32 CooldownReductionPerField = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("CooldownReductionPerTriggeredField"))));
    if (CooldownReductionPerField > 0) ReduceCooldown(FMath::Min(100, TriggeredFields * CooldownReductionPerField));
    const int32 SelfHealPerField = FMath::Max(0, FMath::RoundToInt(GetAbilityValueFromKey(TEXT("SelfHealPerTriggeredField"))));
    if (SelfHealPerField > 0) OwningCharacter->Heal(TriggeredFields * SelfHealPerField);
    for (ARoundGridTile* Field : FieldsToWither) if (Field) Field->SetTerrainType(ERoundTerrainType::Ground);

    Result.bSuccess = true;
    Result.Reason = TEXT("Wild Bloom triggered.");
    return Result;
}
