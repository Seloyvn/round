#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundFieldObjectActor.h"
#include "RoundTypes.h"
#include "RoundBladeActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundBladeActor : public ARoundFieldObjectActor
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Round|Blade")
    virtual FRoundFieldObjectMoveResult MoveToField(class ARoundGridTile* TargetTile);

    UFUNCTION(BlueprintCallable, Category = "Round|Blade")
    virtual FRoundFieldObjectMoveResult MoveAlongPath(const TArray<class ARoundGridTile*>& Path);
};
