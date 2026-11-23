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
    friend class FRoundCoreMatchAdapter;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnInitiativeAdvanced(int32 InitiativeAmount) override;
    virtual bool CanOccupyTile(const class ARoundGridTile* Tile) const override;
    virtual bool CanTriggerDevice_Implementation(const FRoundDeviceTriggerContext& TriggerContext) const override;

    UFUNCTION(BlueprintCallable, Category = "Round|Engineer|Device")
    virtual void InitializeDevice(class ARoundCharacterEngineer* InOwner, int32 InLifetimeInitiative, int32 InHealth);

    UFUNCTION(BlueprintPure, Category = "Round|Engineer|Device")
    virtual int32 GetCurrentHealth() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Engineer|Device")
    virtual int32 ApplyDeviceDamage(int32 DamageAmount);

    UFUNCTION(BlueprintCallable, Category = "Round|Engineer|Device")
    virtual int32 RepairDevice(int32 Amount);

    void ApplyCoreDeviceState(int32 InLifetimeInitiative, int32 InMaximumHealth, int32 InCurrentHealth,
        int32 InTriggerIntervalInitiative, int32 InTriggerProgressInitiative);

    bool IsCoreAuthoritative() const { return bCoreAuthoritative; }

    UFUNCTION(BlueprintCallable, Category = "Round|Engineer|Device")
    virtual void DestroyDevice();

    virtual bool CanReceiveBoardDamage_Implementation() const override;
    virtual FRoundBoardDamageResult ApplyBoardDamage_Implementation(int32 Damage, ERoundDamageType DamageType, int32 DefenseIgnore, class ARoundCharacter* SourceCharacter) override;

protected:
    bool bCoreAuthoritative = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Engineer|Device")
    int32 MaxHealth = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Engineer|Device")
    int32 CurrentHealth = 1;
};
