#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundDeviceFieldObjectActor.h"
#include "Interfaces/RoundBoardDamageableInterface.h"
#include "RoundEngineerDeviceActor.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundEngineerDeviceActor : public ARoundDeviceFieldObjectActor, public IRoundBoardDamageableInterface
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual bool CanOccupyTile(const class ARoundGridTile* Tile) const override;

    UFUNCTION(BlueprintCallable, Category = "Round|Engineer|Device")
    virtual void InitializeDevice(class ARoundCharacterEngineer* InOwner, int32 InLifetimeInitiative, int32 InHealth);

    UFUNCTION(BlueprintPure, Category = "Round|Engineer|Device")
    virtual int32 GetCurrentHealth() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Engineer|Device")
    virtual int32 ApplyDeviceDamage(int32 DamageAmount);

    UFUNCTION(BlueprintCallable, Category = "Round|Engineer|Device")
    virtual int32 RepairDevice(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "Round|Engineer|Device")
    virtual void DestroyDevice();

    virtual bool CanReceiveBoardDamage_Implementation() const override;
    virtual FRoundBoardDamageResult ApplyBoardDamage_Implementation(int32 Damage, ERoundDamageType DamageType, int32 DefenseIgnore, class ARoundCharacter* SourceCharacter) override;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Engineer|Device")
    int32 MaxHealth = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Engineer|Device")
    int32 CurrentHealth = 1;
};
