#include "Actors/RoundTeamActor.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundFieldObjectActor.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Game/RoundGameState.h"
#include "Game/RoundPlayerPawn.h"
#include "Interfaces/RoundDeviceInterface.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"

ARoundTeamActor::ARoundTeamActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);
}

void ARoundTeamActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundTeamActor, TeamId);
    DOREPLIFETIME(ARoundTeamActor, TeamDefinition);
    DOREPLIFETIME(ARoundTeamActor, VisionFields);
}

ARoundGameState* ARoundTeamActor::GetRoundGameState() const
{
    return GetWorld() ? GetWorld()->GetGameState<ARoundGameState>() : nullptr;
}

void ARoundTeamActor::SetVisionFields()
{
    VisionFields.Reset();

    ARoundGameState* RoundGameState = GetRoundGameState();
    if (!RoundGameState)
    {
        return;
    }

    for (ARoundCharacter* Character : RoundGameState->RegisteredCharacters)
    {
        if (!Character || Character->Team != TeamId)
        {
            continue;
        }

        for (ARoundGridTile* Field : Character->GetVisionFields())
        {
            if (Field)
            {
                VisionFields.AddUnique(Field);
            }
        }
    }

    if (GetWorld())
    {
        for (TActorIterator<ARoundFieldObjectActor> It(GetWorld()); It; ++It)
        {
            ARoundFieldObjectActor* FieldObject = *It;
            if (!FieldObject || !FieldObject->GetClass()->ImplementsInterface(URoundDeviceInterface::StaticClass()))
            {
                continue;
            }

            ARoundCharacter* DeviceOwner = IRoundDeviceInterface::Execute_GetDeviceOwnerCharacter(FieldObject);
            if (!DeviceOwner || DeviceOwner->Team != TeamId)
            {
                continue;
            }

            for (ARoundGridTile* Field : IRoundDeviceInterface::Execute_GetDeviceVisionFields(FieldObject))
            {
                if (Field)
                {
                    VisionFields.AddUnique(Field);
                }
            }
        }
    }

    ForceNetUpdate();
}

void ARoundTeamActor::OnRep_VisionFields()
{
    if (!GetWorld())
    {
        return;
    }

    for (TActorIterator<ARoundPlayerPawn> It(GetWorld()); It; ++It)
    {
        if (ARoundPlayerPawn* PlayerPawn = *It; PlayerPawn && PlayerPawn->IsLocallyControlled())
        {
            PlayerPawn->UpdateVision();
        }
    }
}

void ARoundTeamActor::SpawnCharacters(const FRoundTeamDefinition& Team, const FVector2D& Position, const FVector2D& PosMult)
{
    TeamDefinition = Team;
    UE_LOG(LogTemp, Log, TEXT("RoundTeamActor SpawnCharacters - Team=%d Name=%s Characters=%d Position=%s PosMult=%s"),
        static_cast<int32>(TeamId),
        *Team.Name,
        Team.Characters.Num(),
        *Position.ToString(),
        *PosMult.ToString());

    ARoundGameState* RoundGameState = GetRoundGameState();
    if (!RoundGameState)
    {
        UE_LOG(LogTemp, Warning, TEXT("RoundTeamActor SpawnCharacters skipped - no GameState"));
        return;
    }

    for (const FRoundCharacterEntry& CharacterEntry : Team.Characters)
    {
        const FVector2D SpawnPosition = Position + (CharacterEntry.StartPosition * PosMult);
        UE_LOG(LogTemp, Log, TEXT("RoundTeamActor spawning character - Character=%s Start=%s Spawn=%s Skillpoints=%lld"),
            *CharacterEntry.Character,
            *CharacterEntry.StartPosition.ToString(),
            *SpawnPosition.ToString(),
            CharacterEntry.Skillpoints);
        RoundGameState->SpawnCharacterByRowName(
            *CharacterEntry.Character,
            SpawnPosition,
            this,
            CharacterEntry.Skillpoints,
            CharacterEntry.CharacterSkillSelections,
            CharacterEntry.Skills);
    }
}
