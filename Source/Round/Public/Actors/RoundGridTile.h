#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoundTypes.h"
#include "RoundGridTile.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundGridTile : public AActor
{
    GENERATED_BODY()

public:
    ARoundGridTile();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    TObjectPtr<class UStaticMeshComponent> TileMesh = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid|Debug")
    TObjectPtr<class UTextRenderComponent> DebugTerrainLabel = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid|Debug")
    TObjectPtr<class UStaticMeshComponent> DebugRegionOverlay = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<class UMaterialInstanceDynamic> DebugRegionMaterial = nullptr;

    // Mirror the Blueprint field position, which is addressed with Vector2D coordinates.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Grid")
    FVector2D GridPosition = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Grid")
    FIntPoint GridCoordinate = FIntPoint::ZeroValue;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Grid")
    ERoundTerrainType TerrainType = ERoundTerrainType::Ground;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Grid")
    int32 MoveCost = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Grid")
    bool bBlocksVision = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Grid|Region")
    FName TerrainRegionId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Grid|Region")
    int32 TerrainRegionIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
    TMap<ERoundTerrainType, TSubclassOf<AActor>> TerrainVisualClasses;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Grid")
    TObjectPtr<AActor> TerrainObject = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Grid")
    TObjectPtr<class ARoundCharacter> OccupyingCharacter = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Grid")
    TObjectPtr<class ARoundFieldObjectActor> OccupyingFieldObject = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
    bool bHasVision = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    bool bSelected = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    ERoundHoverFieldState HoverState = ERoundHoverFieldState::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    TArray<TObjectPtr<class ARoundFieldTriggerActor>> FieldTriggers;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    bool IsWalkable() const;

    UFUNCTION(BlueprintPure, Category = "Grid")
    class ARoundCharacter* GetResolvedOccupyingCharacter() const;

    UFUNCTION(BlueprintPure, Category = "Grid")
    class ARoundFieldObjectActor* GetResolvedOccupyingFieldObject() const;

    UFUNCTION(BlueprintPure, Category = "Grid")
    bool HasBlockingFieldObject() const;

    UFUNCTION(BlueprintPure, Category = "Grid")
    bool IsMoveable() const;

    UFUNCTION(BlueprintPure, Category = "Grid")
    int32 GetTerrainMoveCost(int32 BaseCost = 0) const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    int32 GetTerrainVisionCost() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    int32 GetTerrainVisionRangeModifier() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    bool TerrainStopsMovement() const;

    UFUNCTION(BlueprintPure, Category = "Grid")
    bool MatchesGridPosition(const FVector2D& InPosition) const;

    UFUNCTION(BlueprintPure, Category = "Grid")
    int32 GetHexDistanceTo(const ARoundGridTile* OtherTile) const;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    int32 GetDistance(const ARoundGridTile* OtherTile) const;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    TArray<ARoundGridTile*> GetDistanceFields(int32 Distance) const;

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void SetTerrainType(ERoundTerrainType NewTerrainType);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void SetHoverState(ERoundHoverFieldState NewState);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void SetSelectedState(bool bInSelected);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void SetHasVision(bool bInHasVision);

    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void SetDebugTerrainLabelVisible(bool bVisible);

    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void SetDebugRegionOverlay(bool bVisible, FLinearColor Color, class UMaterialInterface* OverlayMaterial);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void AddFieldTrigger(class ARoundFieldTriggerActor* TriggerActor);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void RemoveFieldTrigger(class ARoundFieldTriggerActor* TriggerActor);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void NotifyCharacterEntered(class ARoundCharacter* EnteringCharacter, ARoundGridTile* PreviousTile);

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void OnHover();

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void OnEndHover();

    UFUNCTION(BlueprintCallable, Category = "Grid")
    void SetDefaultColor();

protected:
    void ApplyCurrentMaterial();

    UFUNCTION()
    void HandleTileClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);

    UFUNCTION()
    void HandleTileBeginCursorOver(UPrimitiveComponent* TouchedComponent);

    UFUNCTION()
    void HandleTileEndCursorOver(UPrimitiveComponent* TouchedComponent);
};
