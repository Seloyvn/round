#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundTypes.h"
#include "RoundCharacterBladecaller.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FRoundBladePickedUpDelegate, class ARoundGridTile*);

UCLASS(Blueprintable)
class ROUND_API ARoundCharacterBladecaller : public ARoundCharacter
{
    GENERATED_BODY()

public:
    ARoundCharacterBladecaller();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Die() override;
    virtual void AdvanceGlobalInitiative(int32 Amount = 1) override;
    virtual int32 ModifyOutgoingDamage(int32 Amount, ERoundDamageType DamageType, ARoundCharacter* TargetCharacter, int32& DefenseIgnore) const override;

    UFUNCTION(BlueprintPure, Category = "Round|Bladecaller")
    TSubclassOf<class ARoundBladeActor> GetBladeActorClass() const;

    UFUNCTION(BlueprintPure, Category = "Round|Bladecaller")
    int32 GetBladeLifetimeInitiative() const;

    UFUNCTION(BlueprintPure, Category = "Round|Bladecaller")
    int32 GetActiveBladeCount() const;

    const TArray<TObjectPtr<class ARoundBladeActor>>& GetActiveBlades() const;

    UFUNCTION(BlueprintPure, Category = "Round|Bladecaller")
    class ARoundBladeActor* FindBladeAtTile(class ARoundGridTile* Tile) const;

    UFUNCTION(BlueprintCallable, Category = "Round|Bladecaller")
    class ARoundBladeActor* SummonBladeAtTile(class ARoundGridTile* Tile);

    UFUNCTION(BlueprintCallable, Category = "Round|Bladecaller")
    FRoundFieldObjectMoveResult MoveBladeToTile(class ARoundBladeActor* Blade, class ARoundGridTile* Tile);

    UFUNCTION(BlueprintCallable, Category = "Round|Bladecaller")
    FRoundFieldObjectMoveResult RecallBlade(class ARoundBladeActor* Blade);

    UFUNCTION(BlueprintCallable, Category = "Round|Bladecaller")
    bool PickUpBlade(class ARoundBladeActor* Blade);

    FRoundBladePickedUpDelegate& GetBladePickedUpEvent() { return BladePickedUpEvent; }

protected:
    void CleanupInvalidBlades();
    void RemoveAllBlades();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round|Bladecaller")
    TSubclassOf<class ARoundBladeActor> BladeActorClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Bladecaller")
    TArray<TObjectPtr<class ARoundBladeActor>> ActiveBlades;

    FRoundBladePickedUpDelegate BladePickedUpEvent;
};
