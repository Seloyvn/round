#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Data/RoundTeamData.h"
#include "RoundTypes.h"
#include "RoundGameInstance.generated.h"

UCLASS(Blueprintable)
class ROUND_API URoundGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;

    static TArray<FRoundTeamDefinition> GetBuiltInTeamPresets();
    static bool TryGetBuiltInFormatDefinition(ERoundFormat Format, FRoundFormatDefinition& OutFormat);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TObjectPtr<class UDataTable> FormatTable = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TArray<FTeamLoadout> SavedTeams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    FRoundTeamDefinition StrTeam;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Round")
    TArray<FRoundTeamDefinition> LoadedTeams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Round")
    TMap<ERoundFormat, FRoundFormatDefinition> FormatDefinitions;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Round|Room")
    bool bHostingRoom = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Round|Room")
    bool bJoinedRoom = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Round|Room")
    FString RoomName = TEXT("Test Room");

    UFUNCTION(BlueprintCallable, Category = "Round")
    bool AddOrReplaceTeam(const FTeamLoadout& TeamLoadout);

    UFUNCTION(BlueprintPure, Category = "Round")
    bool TryGetFormatDefinition(ERoundFormat Format, FRoundFormatDefinition& OutFormat) const;

    UFUNCTION(BlueprintPure, Category = "Round")
    bool TryGetPrimaryFormatDefinition(FRoundFormatDefinition& OutFormat) const;

private:
    void InitializeDefaultFormats();
    void LoadSavedRoundTeams();
    TArray<FRoundTeamDefinition> LoadDefaultTeamPresets() const;
};
