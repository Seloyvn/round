#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoundTypes.h"
#include "RoundCharacterSkillMenuWidget.generated.h"

class URoundMainMenuWidget;

UCLASS(Blueprintable)
class ROUND_API URoundCharacterSkillMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round|UI|SkillMenu", meta = (ExposeOnSpawn = "true"))
    TObjectPtr<URoundMainMenuWidget> MainMenu = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    FRoundCharacterEntry Character() const;

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    void SetSelectedCharacter(const FRoundCharacterEntry& CharacterEntry);

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    void SetPosition(FVector2D InPosition);

    UFUNCTION(BlueprintCallable, Category = "Round|UI|SkillMenu")
    void LoadSkillpoints();

    UFUNCTION(BlueprintImplementableEvent, Category = "Round|UI|SkillMenu")
    void BP_OnCharacterSkillMenuRefreshed();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round|UI|SkillMenu")
    FVector2D Position = FVector2D::ZeroVector;
};
