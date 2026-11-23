#pragma once

#include "CoreMinimal.h"
#include "Actors/RoundCharacter.h"
#include "RoundCharacterEngineer.generated.h"

UCLASS(Blueprintable)
class ROUND_API ARoundCharacterEngineer : public ARoundCharacter
{
    GENERATED_BODY()

public:
    friend class FRoundCoreMatchAdapter;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void Die() override;
    virtual void BeginTurn() override;
    virtual void AdvanceGlobalInitiative(int32 Amount = 1) override;

    UFUNCTION(BlueprintPure, Category = "Round|Engineer")
    TSubclassOf<class ARoundTurretDeviceActor> GetTurretDeviceClass() const;

    UFUNCTION(BlueprintPure, Category = "Round|Engineer")
    TSubclassOf<class ARoundMineDeviceActor> GetMineDeviceClass() const;

    UFUNCTION(BlueprintPure, Category = "Round|Engineer")
    TSubclassOf<class ARoundShieldEmitterDeviceActor> GetShieldEmitterDeviceClass() const;

    UFUNCTION(BlueprintPure, Category = "Round|Engineer")
    TSubclassOf<class ARoundSpotterBeaconDeviceActor> GetSpotterBeaconDeviceClass() const;

    UFUNCTION(BlueprintPure, Category = "Round|Engineer")
    int32 GetMaxDevices() const;

    UFUNCTION(BlueprintPure, Category = "Round|Engineer")
    int32 GetActiveDeviceCount() const;

    const TArray<TObjectPtr<class ARoundEngineerDeviceActor>>& GetActiveDevices() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Engineer")
    class ARoundEngineerDeviceActor* SpawnDeviceAtTile(TSubclassOf<class ARoundEngineerDeviceActor> DeviceClass, class ARoundGridTile* Tile);

    UFUNCTION(BlueprintCallable, Category = "Round|Engineer")
    void RegisterDevice(class ARoundEngineerDeviceActor* DeviceActor);

    UFUNCTION(BlueprintCallable, Category = "Round|Engineer")
    void UnregisterDevice(class ARoundEngineerDeviceActor* DeviceActor);

    void SetCoreDevicesAuthoritative(bool bInAuthoritative) { bCoreDevicesAuthoritative = bInAuthoritative; }

protected:
    bool bCoreDevicesAuthoritative = false;
    virtual int32 GetIncomingBonusDefense(ERoundDamageType DamageType) const override;
    void CleanupInvalidDevices();
    void RemoveAllDevices();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round|Engineer")
    TSubclassOf<class ARoundTurretDeviceActor> TurretDeviceClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round|Engineer")
    TSubclassOf<class ARoundMineDeviceActor> MineDeviceClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round|Engineer")
    TSubclassOf<class ARoundShieldEmitterDeviceActor> ShieldEmitterDeviceClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Round|Engineer")
    TSubclassOf<class ARoundSpotterBeaconDeviceActor> SpotterBeaconDeviceClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Round|Engineer")
    TArray<TObjectPtr<class ARoundEngineerDeviceActor>> ActiveDevices;
};
