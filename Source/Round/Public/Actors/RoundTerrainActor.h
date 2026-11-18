#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/RoundBoardDamageableInterface.h"
#include "RoundTypes.h"
#include "RoundTerrainActor.generated.h"

class ARoundGridTile;
class ARoundCharacter;
class UStaticMeshComponent;
class URoundTerrainData;
enum class ERoundDamageType : uint8;

UCLASS(Blueprintable)
class ROUND_API ARoundTerrainActor : public AActor, public IRoundBoardDamageableInterface
{
    GENERATED_BODY()

public:
    ARoundTerrainActor();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    TObjectPtr<USceneComponent> SceneRoot = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    TObjectPtr<UStaticMeshComponent> MeshComponent = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Terrain")
    TObjectPtr<ARoundGridTile> Field = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Terrain")
    ERoundTerrainType TerrainType = ERoundTerrainType::Ground;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    int32 TerrainBaseMoveCost = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    bool bTerrainMoveable = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Round|Terrain")
    TObjectPtr<URoundTerrainData> TerrainData = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Terrain")
    int32 MaxHealth = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Terrain")
    int32 CurrentHealth = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Terrain")
    int32 RemainingInitiative = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Terrain")
    int32 PeriodicTriggerProgressInitiative = 0;

    UFUNCTION(BlueprintCallable, Category = "Round|Terrain")
    virtual void SetField(ARoundGridTile* InField);

    UFUNCTION(BlueprintCallable, Category = "Round|Terrain")
    virtual void SetTerrainData(URoundTerrainData* InTerrainData);

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual bool IsMoveable() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual bool BlocksVision() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual int32 GetVisionCost() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual int32 GetVisionRangeModifier() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual bool StopsMovement() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual int32 GetTerrainBaseMoveCost() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual FText GetTerrainDisplayNameText() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual FText GetTerrainDescriptionText() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual float GetTerrainDefaultWeight() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual float GetTerrainMoveCostMultiplier() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual float GetIncomingRangedDamageMultiplier() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual int32 GetTurnStartDamage() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual ERoundDamageType GetTurnStartDamageType() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual int32 GetOnEnterDamage() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual ERoundDamageType GetOnEnterDamageType() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual int32 GetPeriodicDamage() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual ERoundDamageType GetPeriodicDamageType() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual int32 GetPeriodicDamageIntervalInitiative() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual bool IsDestructible() const;

    UFUNCTION(BlueprintPure, Category = "Round|Terrain")
    virtual int32 GetTerrainMaxHealth() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Terrain")
    virtual void SetRemainingInitiative(int32 DurationInitiative);

    UFUNCTION(BlueprintCallable, Category = "Round|Terrain")
    virtual void AdvanceTerrainInitiative(int32 InitiativeAmount);

    UFUNCTION(BlueprintNativeEvent, Category = "Round|Terrain")
    int32 GetMoveCost(int32 BaseCost) const;
    virtual int32 GetMoveCost_Implementation(int32 BaseCost) const;

    UFUNCTION(BlueprintNativeEvent, Category = "Round|Terrain")
    int32 ModifyIncomingDamage(ARoundCharacter* Defender, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const;
    virtual int32 ModifyIncomingDamage_Implementation(ARoundCharacter* Defender, ARoundCharacter* SourceCharacter, int32 Damage, ERoundDamageType DamageType) const;

    UFUNCTION(BlueprintNativeEvent, Category = "Round|Terrain")
    void OnCharacterTurnStarted(ARoundCharacter* OccupyingCharacter);
    virtual void OnCharacterTurnStarted_Implementation(ARoundCharacter* OccupyingCharacter);

    UFUNCTION(BlueprintNativeEvent, Category = "Round|Terrain")
    void OnCharacterEntered(ARoundCharacter* EnteringCharacter, ARoundGridTile* PreviousTile);
    virtual void OnCharacterEntered_Implementation(ARoundCharacter* EnteringCharacter, ARoundGridTile* PreviousTile);

    virtual bool CanReceiveBoardDamage_Implementation() const override;
    virtual FRoundBoardDamageResult ApplyBoardDamage_Implementation(int32 Damage, ERoundDamageType DamageType, int32 DefenseIgnore, class ARoundCharacter* SourceCharacter) override;
};
