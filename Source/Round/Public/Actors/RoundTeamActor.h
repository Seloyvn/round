#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoundTypes.h"
#include "RoundTeamActor.generated.h"

class ARoundCharacter;
class ARoundGameState;
class ARoundGridTile;

UCLASS(Blueprintable)
class ROUND_API ARoundTeamActor : public AActor
{
    GENERATED_BODY()

public:
    ARoundTeamActor();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Round|Team")
    ERoundTeam TeamId = ERoundTeam::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Round|Team")
    FRoundTeamDefinition TeamDefinition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_VisionFields, Category = "Round|Team")
    TArray<TObjectPtr<ARoundGridTile>> VisionFields;

    UFUNCTION(BlueprintPure, Category = "Round|Team")
    ARoundGameState* GetRoundGameState() const;

    UFUNCTION(BlueprintCallable, Category = "Round|Team")
    void SetVisionFields();

    UFUNCTION(BlueprintCallable, Category = "Round|Team")
    void SpawnCharacters(const FRoundTeamDefinition& Team, const FVector2D& Position, const FVector2D& PosMult);

private:
    UFUNCTION()
    void OnRep_VisionFields();
};
