#pragma once

#include "CoreMinimal.h"
#include "RoundTypes.h"
#include "UObject/Interface.h"
#include "RoundBoardDamageableInterface.generated.h"

USTRUCT(BlueprintType)
struct ROUND_API FRoundBoardDamageResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Round|BoardDamage")
    int32 AppliedDamage = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Round|BoardDamage")
    bool bDestroyed = false;
};

UINTERFACE(BlueprintType)
class ROUND_API URoundBoardDamageableInterface : public UInterface
{
    GENERATED_BODY()
};

class ROUND_API IRoundBoardDamageableInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Round|BoardDamage")
    bool CanReceiveBoardDamage() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Round|BoardDamage")
    FRoundBoardDamageResult ApplyBoardDamage(int32 Damage, ERoundDamageType DamageType, int32 DefenseIgnore, class ARoundCharacter* SourceCharacter);
};
