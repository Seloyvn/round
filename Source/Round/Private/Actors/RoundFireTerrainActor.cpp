#include "Actors/RoundFireTerrainActor.h"

#include "Actors/RoundCharacter.h"
#include "Data/RoundTerrainData.h"

ARoundFireTerrainActor::ARoundFireTerrainActor()
{
    TerrainType = ERoundTerrainType::Fire;
    TerrainBaseMoveCost = 1;
    bTerrainMoveable = true;
    RemainingInitiative = 200;
}

void ARoundFireTerrainActor::SetTerrainData(URoundTerrainData* InTerrainData)
{
    Super::SetTerrainData(InTerrainData);

    if (!TerrainData)
    {
        RemainingInitiative = 200;
        return;
    }

    auto FindDurationValue = [this](const FString& Key, bool& bOutFound) -> int32
    {
        if (const double* ExactValue = TerrainData->Values.Find(Key))
        {
            bOutFound = true;
            return FMath::RoundToInt(*ExactValue);
        }

        for (const TPair<FString, double>& Pair : TerrainData->Values)
        {
            if (Pair.Key.Equals(Key, ESearchCase::IgnoreCase))
            {
                bOutFound = true;
                return FMath::RoundToInt(Pair.Value);
            }
        }

        bOutFound = false;
        return 0;
    };

    bool bFound = false;
    const int32 ConfiguredDuration = FindDurationValue(TEXT("FireDurationInitiative"), bFound);
    if (bFound)
    {
        RemainingInitiative = FMath::Max(0, ConfiguredDuration);
        return;
    }

    const int32 GenericDuration = FindDurationValue(TEXT("DurationInitiative"), bFound);
    if (bFound)
    {
        RemainingInitiative = FMath::Max(0, GenericDuration);
        return;
    }

    const int32 LegacyDuration = FindDurationValue(TEXT("FireDuration"), bFound);
    RemainingInitiative = bFound ? FMath::Max(0, LegacyDuration) : 200;
}

void ARoundFireTerrainActor::OnCharacterTurnStarted_Implementation(ARoundCharacter* OccupyingCharacter)
{
    if (!OccupyingCharacter)
    {
        return;
    }

    const int32 Damage = TerrainData ? GetTurnStartDamage() : 100;
    if (Damage <= 0)
    {
        return;
    }

    bool bDidDie = false;
    OccupyingCharacter->ApplyRoundDamage(Damage, GetTurnStartDamageType(), 0, nullptr, bDidDie);
}
