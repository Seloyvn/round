#pragma once

#include "CoreMinimal.h"

class ARoundAbilityActor;
class URoundAbilityData;
class URoundAbilityRegistryData;

namespace RoundAbilityRegistry
{
URoundAbilityRegistryData* ResolveAbilityRegistry();
URoundAbilityData* FindAbilityDataById(FName AbilityId);
TSubclassOf<ARoundAbilityActor> FindAbilityClassById(FName AbilityId);
}
