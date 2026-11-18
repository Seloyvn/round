#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoundFieldObjectActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundFieldObjectActor : public AActor
{
    GENERATED_BODY()

public:
    ARoundFieldObjectActor();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|FieldObject")
    TObjectPtr<class USceneComponent> SceneRoot = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentTile, Category = "Round|FieldObject")
    TObjectPtr<class ARoundGridTile> CurrentTile = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|FieldObject")
    TObjectPtr<class ARoundCharacter> OwningCharacter = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|FieldObject")
    int32 RemainingInitiative = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round|FieldObject")
    float FieldObjectHeightOffset = 60.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round|FieldObject")
    float MoveSpeed = 2400.0f;

    UFUNCTION(BlueprintCallable, Category = "Round|FieldObject")
    virtual bool CanOccupyTile(const class ARoundGridTile* Tile) const;

    UFUNCTION(BlueprintCallable, Category = "Round|FieldObject")
    virtual bool SetField(class ARoundGridTile* NewTile);

    UFUNCTION(BlueprintCallable, Category = "Round|FieldObject")
    virtual void RemoveFromField();

    UFUNCTION(BlueprintCallable, Category = "Round|FieldObject")
    virtual void SetLifetimeInitiative(int32 DurationInitiative);

    UFUNCTION(BlueprintCallable, Category = "Round|FieldObject")
    virtual void OnInitiativeAdvanced(int32 InitiativeAmount);

protected:
    UFUNCTION()
    void OnRep_CurrentTile();

    virtual FVector GetTileWorldLocation(const class ARoundGridTile* Tile) const;
    virtual void SnapToCurrentTile();
    virtual void StartMoveToCurrentTile();

    UPROPERTY(Transient)
    FVector TargetWorldLocation = FVector::ZeroVector;

    UPROPERTY(Transient)
    bool bAnimateMovement = false;

    UPROPERTY(Transient)
    bool bHasBeenPlacedOnTile = false;
};
