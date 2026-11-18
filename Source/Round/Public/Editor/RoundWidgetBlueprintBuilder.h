#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RoundWidgetBlueprintBuilder.generated.h"

UCLASS()
class ROUND_API URoundWidgetBlueprintBuilder : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Round|Editor")
    static bool RebuildMenuWidgetBlueprints();

    UFUNCTION(BlueprintCallable, Category = "Round|Editor")
    static bool RebuildDebugWidgetBlueprint();
};
