#include "Game/RoundPlayerPawn.h"

#include "Blueprint/UserWidget.h"
#include "Actors/RoundCharacter.h"
#include "Actors/RoundFieldObjectActor.h"
#include "Actors/RoundGridTile.h"
#include "Actors/RoundTeamActor.h"
#include "Data/RoundGameDataRegistryLookup.h"
#include "Camera/CameraComponent.h"
#include "Game/RoundGameState.h"
#include "Game/RoundGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/SpringArmComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UI/RoundGameUIWidget.h"
#include "UI/RoundDebugWidget.h"
#include "Data/RoundGameDataRegistry.h"
#include "Data/RoundTerrainRegionData.h"
#include "Actors/RoundAbilityActor.h"
#include "Interfaces/RoundDeviceInterface.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/AudioComponent.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "EngineUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogRoundVision, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogRoundVisionStart, Log, All);

namespace
{
bool IsPuppetCharacter(const ARoundCharacter* Character)
{
    if (!Character)
    {
        return false;
    }

    const FString RowName = Character->CharacterRowName.ToString();
    return RowName.Contains(TEXT("Puppet"), ESearchCase::IgnoreCase);
}

int32 CountPlayerTeamPackedSelections(const int64 PackedSelections)
{
    int32 Count = 0;
    for (int32 TierIndex = 0; TierIndex < 4; ++TierIndex)
    {
        if (((PackedSelections >> (TierIndex * 2)) & 0x3) == 0)
        {
            break;
        }
        ++Count;
    }
    return Count;
}

bool IsSubmittedTeamValid(const FRoundTeamDefinition& TeamDefinition, const URoundGameInstance* GameInstance)
{
    FRoundFormatDefinition FormatDefinition;
    if (!GameInstance || !GameInstance->TryGetFormatDefinition(TeamDefinition.Format, FormatDefinition))
    {
        return false;
    }
    if (TeamDefinition.Characters.IsEmpty() || TeamDefinition.Characters.Num() != FormatDefinition.Characters)
    {
        return false;
    }

    for (const FRoundCharacterEntry& Character : TeamDefinition.Characters)
    {
        if (Character.Character.IsEmpty())
        {
            return false;
        }

        int32 SpentPoints = CountPlayerTeamPackedSelections(Character.CharacterSkillSelections);
        for (const FRoundSkillEntry& Skill : Character.Skills)
        {
            SpentPoints += CountPlayerTeamPackedSelections(Skill.Points);
        }
        if (SpentPoints != FormatDefinition.Skillpoints)
        {
            return false;
        }
    }
    return true;
}

void AssignServerTeamIfMissing(ARoundPlayerPawn* PlayerPawn)
{
    if (!PlayerPawn || !PlayerPawn->HasAuthority() || PlayerPawn->TeamActor)
    {
        return;
    }

    ARoundGameState* RoundGameState = PlayerPawn->GetRoundGameState();
    if (!RoundGameState)
    {
        return;
    }

    if (RoundGameState->Player1 == PlayerPawn)
    {
        if (RoundGameState->Teams.IsValidIndex(0))
        {
            PlayerPawn->SetTeamActor(Cast<ARoundTeamActor>(RoundGameState->Teams[0]));
        }
        return;
    }

    if (RoundGameState->Player2 == PlayerPawn)
    {
        if (RoundGameState->Teams.IsValidIndex(1))
        {
            PlayerPawn->SetTeamActor(Cast<ARoundTeamActor>(RoundGameState->Teams[1]));
        }
    }
}

void ApplyPendingHandSelectionToAbility(ARoundCharacter* Character, const int32 AbilityIndex, const TArray<int32>& HandIndices)
{
    if (!Character || !Character->GrantedAbilities.IsValidIndex(AbilityIndex) || !Character->GrantedAbilities[AbilityIndex])
    {
        return;
    }

    Character->GrantedAbilities[AbilityIndex]->SetPendingHandSelectionIndices(HandIndices);
}
}

ARoundPlayerPawn::ARoundPlayerPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);
}

void ARoundPlayerPawn::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundPlayerPawn, Team);
    DOREPLIFETIME(ARoundPlayerPawn, TeamDefinition);
    DOREPLIFETIME(ARoundPlayerPawn, TeamActor);
}

void ARoundPlayerPawn::BeginPlay()
{
    Super::BeginPlay();
    SubmitSelectedTeam();
    AssignServerTeamIfMissing(this);
    BindToGameState(GetRoundGameState());
    ConfigureLocalInput();
    SetUI();
    if (IsLocallyControlled() && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
        {
            if (IsValid(this))
            {
                UpdateVision();
                ScheduleVisionRetry();
                if (!HasAuthority())
                {
                    ServerRequestInitialVisionRefresh();
                }
            }
        });

        FTimerHandle VisionStartDebugHandle;
        GetWorld()->GetTimerManager().SetTimer(
            VisionStartDebugHandle,
            [this]()
            {
                if (!IsValid(this) || !IsLocallyControlled())
                {
                    return;
                }

                const ERoundTeam EffectiveTeam = GetEffectiveTeam();
                int32 TotalCharacters = 0;
                int32 TeamCharacters = 0;
                int32 TeamCharactersWithTile = 0;
                TSet<TObjectPtr<ARoundGridTile>> VisibleFieldSet;

                for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
                {
                    ARoundCharacter* Character = *It;
                    ++TotalCharacters;
                    if (!Character || !Character->IsAlive() || Character->Team != EffectiveTeam)
                    {
                        continue;
                    }

                    ++TeamCharacters;
                    if (Character->CurrentTile)
                    {
                        ++TeamCharactersWithTile;
                    }

                    for (ARoundGridTile* Field : Character->GetVisionFields())
                    {
                        if (Field)
                        {
                            VisibleFieldSet.Add(Field);
                        }
                    }
                }

                UE_LOG(LogRoundVisionStart, Log, TEXT("StartVisionState - Pawn=%s Local=%d Authority=%d Team=%d EffectiveTeam=%d TeamActor=%s TotalChars=%d TeamChars=%d TeamCharsWithTile=%d VisibleFields=%d"),
                    *GetNameSafe(this),
                    IsLocallyControlled() ? 1 : 0,
                    HasAuthority() ? 1 : 0,
                    static_cast<int32>(Team),
                    static_cast<int32>(EffectiveTeam),
                    *GetNameSafe(TeamActor),
                    TotalCharacters,
                    TeamCharacters,
                    TeamCharactersWithTile,
                    VisibleFieldSet.Num());
            },
            1.0f,
            false);
    }
}

void ARoundPlayerPawn::PawnClientRestart()
{
    Super::PawnClientRestart();
    SubmitSelectedTeam();
    AssignServerTeamIfMissing(this);
    BindToGameState(GetRoundGameState());
    ConfigureLocalInput();
    SetUI();
    if (IsLocallyControlled() && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
        {
            if (IsValid(this))
            {
                UpdateVision();
                ScheduleVisionRetry();
                if (!HasAuthority())
                {
                    ServerRequestInitialVisionRefresh();
                }
            }
        });
    }
}

void ARoundPlayerPawn::SubmitSelectedTeam()
{
    if (!IsLocallyControlled() || !GetWorld())
    {
        return;
    }

    const URoundGameInstance* GameInstance = GetWorld()->GetGameInstance<URoundGameInstance>();
    if (!GameInstance)
    {
        return;
    }

    const FRoundTeamDefinition* SelectedTeam = &GameInstance->StrTeam;
    if (!GameInstance->bHostingRoom && !GameInstance->bJoinedRoom)
    {
        if (const ARoundGameState* RoundGameState = GetRoundGameState())
        {
            const int32 DefaultTeamIndex = RoundGameState->Player2 == this ? 1 : 0;
            if (GameInstance->LoadedTeams.IsValidIndex(DefaultTeamIndex))
            {
                SelectedTeam = &GameInstance->LoadedTeams[DefaultTeamIndex];
            }
        }
    }
    if (!IsSubmittedTeamValid(*SelectedTeam, GameInstance))
    {
        return;
    }

    if (HasAuthority())
    {
        TeamDefinition = *SelectedTeam;
        if (ARoundGameState* RoundGameState = GetRoundGameState())
        {
            RoundGameState->TryCreateTeamsFromPlayerSelections();
        }
    }
    else
    {
        ServerSubmitSelectedTeam(*SelectedTeam);
    }
}

void ARoundPlayerPawn::ServerSubmitSelectedTeam_Implementation(const FRoundTeamDefinition& SelectedTeam)
{
    const URoundGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance<URoundGameInstance>() : nullptr;
    const FRoundTeamDefinition* ResolvedTeam = &SelectedTeam;
    if (GameInstance && !GameInstance->bHostingRoom && !GameInstance->bJoinedRoom)
    {
        if (const ARoundGameState* RoundGameState = GetRoundGameState())
        {
            const int32 DefaultTeamIndex = RoundGameState->Player2 == this ? 1 : 0;
            if (GameInstance->LoadedTeams.IsValidIndex(DefaultTeamIndex))
            {
                ResolvedTeam = &GameInstance->LoadedTeams[DefaultTeamIndex];
            }
        }
    }
    if (!IsSubmittedTeamValid(*ResolvedTeam, GameInstance))
    {
        return;
    }

    TeamDefinition = *ResolvedTeam;
    ForceNetUpdate();
    if (ARoundGameState* RoundGameState = GetRoundGameState())
    {
        RoundGameState->TryCreateTeamsFromPlayerSelections();
    }
}

void ARoundPlayerPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearActionPreview();

    if (BoundGameState.IsValid())
    {
        BoundGameState->OnTurnChanged.RemoveDynamic(this, &ARoundPlayerPawn::HandleTurnChanged);
        BoundGameState->OnBoardStateChanged.RemoveDynamic(this, &ARoundPlayerPawn::HandleBoardStateChanged);
        BoundGameState.Reset();
    }

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(VisionRetryTimerHandle);
    }

    if (UI)
    {
        UI->RemoveFromParent();
        UI = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void ARoundPlayerPawn::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UpdateCameraMovement(DeltaSeconds);
    UpdateCameraZoom();

    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (PlayerController->IsLocalController() && PlayerController->WasInputKeyJustPressed(EKeys::RightMouseButton))
        {
            Unselect();
        }
    }
}

void ARoundPlayerPawn::SetTeamActor(ARoundTeamActor* InTeamActor)
{
    TeamActor = InTeamActor;

    if (InTeamActor)
    {
        Team = InTeamActor->TeamId;
        TeamDefinition = InTeamActor->TeamDefinition;
    }

    if (IsLocallyControlled())
    {
        UpdateVision();
    }
}

void ARoundPlayerPawn::OnRep_Team()
{
    if (TeamActor && TeamActor->TeamId != ERoundTeam::Neutral)
    {
        Team = TeamActor->TeamId;
        TeamDefinition = TeamActor->TeamDefinition;
    }

    SetUI();
    BindToGameState(GetRoundGameState());

    if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
    {
        GameUI->RefreshFromPlayer();
    }

    UpdateVision();
    ScheduleVisionRetry();
    RefreshUI();
}

void ARoundPlayerPawn::OnRep_TeamActor()
{
    if (TeamActor)
    {
        Team = TeamActor->TeamId;
        TeamDefinition = TeamActor->TeamDefinition;
    }

    SetUI();
    BindToGameState(GetRoundGameState());

    if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
    {
        GameUI->RefreshFromPlayer();
    }

    UpdateVision();
    ScheduleVisionRetry();
    RefreshUI();
}

ARoundGameState* ARoundPlayerPawn::GetRoundGameState() const
{
    return GetWorld() ? GetWorld()->GetGameState<ARoundGameState>() : nullptr;
}

ARoundCharacter* ARoundPlayerPawn::SelectedCharacter() const
{
    if (!SelectedField)
    {
        return nullptr;
    }

    if (SelectedField->OccupyingCharacter)
    {
        return CanSeeCharacter(SelectedField->OccupyingCharacter) ? SelectedField->OccupyingCharacter : nullptr;
    }

    if (const FObjectProperty* CharacterProperty = FindFProperty<FObjectProperty>(SelectedField->GetClass(), TEXT("Character")))
    {
        if (ARoundCharacter* FieldCharacter = Cast<ARoundCharacter>(CharacterProperty->GetObjectPropertyValue_InContainer(SelectedField)))
        {
            return CanSeeCharacter(FieldCharacter) ? FieldCharacter : nullptr;
        }
    }

    if (SelectedCharacterOverride)
    {
        return CanSeeCharacter(SelectedCharacterOverride) ? SelectedCharacterOverride : nullptr;
    }

    return nullptr;
}

bool ARoundPlayerPawn::CanControlCharacter(const ARoundCharacter* Character) const
{
    if (!Character || Character->Team != Team || IsPuppetCharacter(Character))
    {
        return false;
    }

    const ARoundGameState* RoundGameState = GetRoundGameState();
    const ARoundCharacter* ActiveCharacter = RoundGameState ? RoundGameState->GetActiveCharacter() : nullptr;
    return Character == ActiveCharacter;
}

bool ARoundPlayerPawn::CanIssueCommandForCharacter(const ARoundCharacter* Character) const
{
    if (!Character || Character->Team != Team || IsPuppetCharacter(Character))
    {
        return false;
    }

    const ARoundGameState* RoundGameState = GetRoundGameState();
    const ARoundCharacter* ActiveCharacter = RoundGameState ? RoundGameState->GetActiveCharacter() : nullptr;
    return Character == ActiveCharacter;
}

void ARoundPlayerPawn::SelectField(ARoundGridTile* Field)
{
    if (SelectedField && SelectedField != Field)
    {
        SelectedField->SetSelectedState(false);
        SelectedField->SetDefaultColor();
    }

    SelectedField = Field;
    if (SelectedField)
    {
        SelectedField->SetSelectedState(true);
    }

    SelectedCharacterOverride = nullptr;
    if (SelectedField)
    {
        if (SelectedField->OccupyingCharacter)
        {
            SelectedCharacterOverride = SelectedField->OccupyingCharacter;
        }
        else if (const FObjectProperty* CharacterProperty = FindFProperty<FObjectProperty>(SelectedField->GetClass(), TEXT("Character")))
        {
            SelectedCharacterOverride = Cast<ARoundCharacter>(CharacterProperty->GetObjectPropertyValue_InContainer(SelectedField));
        }
    }

    SetUI();

    if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
    {
        GameUI->RefreshFromPlayer();
    }

    if (bSnapCameraToSelectedField && IsLocallyControlled() && SelectedField)
    {
        SnapCameraToField(SelectedField, true);
    }

    RefreshActionPreview();
    RefreshUI();
}

void ARoundPlayerPawn::SetUI()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    const bool bIsLocalPawn = PlayerController && PlayerController->IsLocalController();

    if (!bIsLocalPawn)
    {
        if (UI)
        {
            UI->RemoveFromParent();
            UI = nullptr;
        }
        return;
    }

    if (!GameUIClass)
    {
        GameUIClass = RoundGameDataRegistryLookup::ResolveGameUIWidgetClass();
    }

    if (!UI && GameUIClass && GetWorld())
    {
        if (PlayerController)
        {
            UI = CreateWidget<UUserWidget>(PlayerController, GameUIClass);
        }
        else
        {
            UI = CreateWidget<UUserWidget>(GetWorld(), GameUIClass);
        }
    }

    BindToGameState(GetRoundGameState());

    if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
    {
        GameUI->SetPlayerPawn(this);
    }

    if (UI && !UI->IsInViewport())
    {
        UI->AddToViewport();
    }

    RefreshUI();
}

void ARoundPlayerPawn::HandleTurnChanged(ARoundCharacter* ActiveCharacter)
{
    if (!IsLocallyControlled())
    {
        return;
    }

    if (!ActiveCharacter)
    {
        Unselect();
        return;
    }

    if (TrySelectActiveCharacterForLocalTurn(true))
    {
        return;
    }

    if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
    {
        GameUI->RefreshFromPlayer();
    }

    RefreshUI();
}

void ARoundPlayerPawn::BindToGameState(ARoundGameState* RoundGameState)
{
    if (BoundGameState.Get() == RoundGameState)
    {
        return;
    }

    if (BoundGameState.IsValid())
    {
        BoundGameState->OnTurnChanged.RemoveDynamic(this, &ARoundPlayerPawn::HandleTurnChanged);
        BoundGameState->OnBoardStateChanged.RemoveDynamic(this, &ARoundPlayerPawn::HandleBoardStateChanged);
    }

    BoundGameState = RoundGameState;
    if (RoundGameState)
    {
        RoundGameState->OnTurnChanged.AddDynamic(this, &ARoundPlayerPawn::HandleTurnChanged);
        RoundGameState->OnBoardStateChanged.AddDynamic(this, &ARoundPlayerPawn::HandleBoardStateChanged);
    }
}

void ARoundPlayerPawn::HandleBoardStateChanged()
{
    if (!IsLocallyControlled())
    {
        return;
    }

    TrySelectActiveCharacterForLocalTurn(false);

    ++ActionPreviewBoardRevision;
    bActionRangePreviewDirty = true;

    UpdateVision();
    RefreshActionPreview();
}

bool ARoundPlayerPawn::TrySelectActiveCharacterForLocalTurn(const bool bSnapCamera)
{
    ARoundGameState* RoundGameState = GetRoundGameState();
    ARoundCharacter* ActiveCharacter = RoundGameState ? RoundGameState->GetActiveCharacter() : nullptr;
    if (!CanControlCharacter(ActiveCharacter) || !ActiveCharacter || !ActiveCharacter->CurrentTile || !CanSeeCharacter(ActiveCharacter))
    {
        return false;
    }

    if (SelectedCharacter() == ActiveCharacter && SelectedField == ActiveCharacter->CurrentTile)
    {
        if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
        {
            GameUI->RefreshFromPlayer();
        }
        RefreshUI();
        return true;
    }

    SelectField(ActiveCharacter->CurrentTile);
    if (bSnapCamera)
    {
        SnapCameraToField(ActiveCharacter->CurrentTile, true);
    }
    return true;
}

void ARoundPlayerPawn::AbilitySelected(const int32 AbilityIndex)
{
    if (!CanControlCharacter(SelectedCharacter()))
    {
        return;
    }

    SelectedAbilityIndex = AbilityIndex;
    SelectedAction = AbilityIndex >= 0 ? ERoundActionType::Ability : ERoundActionType::None;

    if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
    {
        GameUI->RefreshButtons();
    }

    RefreshActionPreview();
    RefreshUI();
}

void ARoundPlayerPawn::UseAbilityCmd(ARoundGridTile* Field, const int32 AbilityIndex)
{
    if (AbilityIndex < 0)
    {
        return;
    }

    ARoundCharacter* Character = SelectedCharacter();
    if (!Character || !Field || !CanControlCharacter(Character))
    {
        return;
    }

    const FRoundActionResult Result = Character->UseAbilityIndex(AbilityIndex, Field);
    if (Result.bSuccess)
    {
        SelectedAbilityIndex = INDEX_NONE;
        SelectedAction = ERoundActionType::None;
        ARoundGridTile* FieldToKeep = Character->CurrentTile ? Character->CurrentTile.Get() : Field;
        SelectField(FieldToKeep);
        RefreshActionPreview();
    }
}

void ARoundPlayerPawn::UseAbilityDirect(int32 AbilityIndex)
{
    UseAbilityDirectWithHandSelection(AbilityIndex, {});
}

void ARoundPlayerPawn::UseAbilityDirectWithHandSelection(int32 AbilityIndex, const TArray<int32>& HandIndices)
{
    if (AbilityIndex < 0)
    {
        return;
    }

    ARoundCharacter* Character = SelectedCharacter();
    if (!Character || !CanControlCharacter(Character))
    {
        return;
    }

    if (HasAuthority())
    {
        ApplyPendingHandSelectionToAbility(Character, AbilityIndex, HandIndices);
        const FRoundActionResult Result = Character->UseAbilityIndex(AbilityIndex, nullptr);
        ApplyPendingHandSelectionToAbility(Character, AbilityIndex, {});
        if (Result.bSuccess)
        {
            SelectedAbilityIndex = INDEX_NONE;
            SelectedAction = ERoundActionType::None;
            if (Character->CurrentTile)
            {
                SelectField(Character->CurrentTile);
            }
            RefreshActionPreview();
        }
    }
    else
    {
        ServerUseAbilityDirectWithHandSelection(Character, AbilityIndex, HandIndices);
    }

    if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
    {
        GameUI->RefreshFromPlayer();
    }
}

void ARoundPlayerPawn::Unselect()
{
    if (SelectedField)
    {
        SelectedField->SetSelectedState(false);
        SelectedField->SetDefaultColor();
    }

    SelectedField = nullptr;
    SelectedCharacterOverride = nullptr;
    SelectedAction = ERoundActionType::None;
    SelectedAbilityIndex = INDEX_NONE;
    HoveredField = nullptr;
    ClearActionPreview();

    if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
    {
        GameUI->RefreshFromPlayer();
    }

    RefreshUI();
}

void ARoundPlayerPawn::Attack()
{
    if (!CanControlCharacter(SelectedCharacter()))
    {
        return;
    }

    SelectedAction = ERoundActionType::Attack;
    SelectedAbilityIndex = INDEX_NONE;

    if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
    {
        GameUI->RefreshButtons();
    }

    RefreshActionPreview();
    RefreshUI();
}

void ARoundPlayerPawn::Move()
{
    if (!CanControlCharacter(SelectedCharacter()))
    {
        return;
    }

    SelectedAction = ERoundActionType::Move;
    SelectedAbilityIndex = INDEX_NONE;

    if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
    {
        GameUI->RefreshButtons();
    }

    RefreshActionPreview();
    RefreshUI();
}

void ARoundPlayerPawn::EndTurnServer()
{
    EndTurn();
}

void ARoundPlayerPawn::EndTurn()
{
    if (ARoundCharacter* Character = SelectedCharacter(); CanControlCharacter(Character))
    {
        if (HasAuthority())
        {
            Character->EndTurn();
        }
        else
        {
            ServerEndTurnCharacter(Character);
        }
    }

    if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
    {
        GameUI->RefreshFromPlayer();
    }
}

void ARoundPlayerPawn::ObjectiveClaim()
{
    if (ARoundCharacter* Character = SelectedCharacter(); CanControlCharacter(Character))
    {
        if (HasAuthority())
        {
            if (ARoundGameState* RoundGameState = GetRoundGameState())
            {
                FString Reason;
                RoundGameState->TryStartObjectiveClaim(Character, Reason);
            }
        }
        else
        {
            ServerStartObjectiveClaim(Character);
        }
    }

    if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
    {
        GameUI->RefreshFromPlayer();
    }

    RefreshActionPreview();
}

void ARoundPlayerPawn::Klick(ARoundGridTile* Field)
{
    if (!Field)
    {
        return;
    }

    ARoundCharacter* ActiveCharacter = SelectedCharacter();

    switch (SelectedAction)
    {
    case ERoundActionType::Move:
        if (CanControlCharacter(ActiveCharacter))
        {
            TArray<ARoundGridTile*> MovePath;
            int32 PathCost = 0;
            if (!ActiveCharacter->TryBuildMovePath(Field, MovePath, PathCost, true))
            {
                break;
            }

            if (HasAuthority())
            {
                const FRoundActionResult Result = ActiveCharacter->MoveAlongPath(MovePath);
                if (Result.bSuccess)
                {
                    SelectedAction = ERoundActionType::None;
                    SelectField(ActiveCharacter->CurrentTile);
                    return;
                }
            }
            else
            {
                ServerExecuteMoveAlongPath(ActiveCharacter, MovePath);
                SelectedAction = ERoundActionType::None;
                if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
                {
                    GameUI->RefreshButtons();
                }
                return;
            }
        }
        break;

    case ERoundActionType::Attack:
        if (CanControlCharacter(ActiveCharacter))
        {
            if (HasAuthority())
            {
                const FRoundActionResult Result = ActiveCharacter->AttackField(Field);
                if (Result.bSuccess)
                {
                    SelectedAction = ERoundActionType::None;
                    SelectField(ActiveCharacter->CurrentTile);
                    return;
                }

                if (ActiveCharacter->EnemyField(Field))
                {
                    return;
                }
            }
            else
            {
                ServerExecuteAction(ActiveCharacter, Field, ERoundActionType::Attack, INDEX_NONE);
                SelectedAction = ERoundActionType::None;
                if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
                {
                    GameUI->RefreshButtons();
                }
                return;
            }
        }
        break;

    case ERoundActionType::Ability:
        if (CanControlCharacter(ActiveCharacter))
        {
            if (HasAuthority())
            {
                UseAbilityCmd(Field, SelectedAbilityIndex);
            }
            else
            {
                ServerExecuteAction(ActiveCharacter, Field, ERoundActionType::Ability, SelectedAbilityIndex);
                if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
                {
                    GameUI->RefreshButtons();
                }
            }
        }
        return;

    default:
        break;
    }

    if (Field->OccupyingCharacter)
    {
        SelectField(Field);
        return;
    }

    if (!Field->OccupyingCharacter)
    {
        SelectField(Field);
    }
}

void ARoundPlayerPawn::ServerExecuteAction_Implementation(ARoundCharacter* Character, ARoundGridTile* Field, const ERoundActionType Action, const int32 AbilityIndex)
{
    if (!Character || !Field || !CanIssueCommandForCharacter(Character))
    {
        return;
    }

    switch (Action)
    {
    case ERoundActionType::Move:
    {
            const FRoundActionResult Result = Character->MoveToTile(Field);
            if (Result.bSuccess)
            {
                SelectedAction = ERoundActionType::None;
                SelectedAbilityIndex = INDEX_NONE;
                SelectField(Field);
                ClientPostActionResult(Field, Character);
            }
            break;
        }
    case ERoundActionType::Attack:
    {
        const FRoundActionResult Result = Character->AttackField(Field);
        if (Result.bSuccess)
        {
            SelectedAction = ERoundActionType::None;
            SelectedAbilityIndex = INDEX_NONE;
            SelectField(Character->CurrentTile);
            ClientPostActionResult(Character->CurrentTile, Character);
        }
        break;
    }
    case ERoundActionType::Ability:
    {
        const FRoundActionResult Result = Character->UseAbilityIndex(AbilityIndex, Field);
        if (Result.bSuccess)
        {
            SelectedAction = ERoundActionType::None;
            SelectedAbilityIndex = INDEX_NONE;
            ARoundGridTile* FieldToKeep = Character->CurrentTile ? Character->CurrentTile.Get() : Field;
            SelectField(FieldToKeep);
            ClientPostActionResult(FieldToKeep, Character);
        }
        break;
    }
    default:
        break;
    }
}

void ARoundPlayerPawn::ServerExecuteMoveAlongPath_Implementation(ARoundCharacter* Character, const TArray<ARoundGridTile*>& Path)
{
    if (!Character || !CanIssueCommandForCharacter(Character) || Path.Num() == 0)
    {
        return;
    }

    const FRoundActionResult Result = Character->MoveAlongPath(Path);
    if (Result.bSuccess)
    {
        SelectedAction = ERoundActionType::None;
        SelectedAbilityIndex = INDEX_NONE;
        SelectField(Character->CurrentTile);
        ClientPostActionResult(Character->CurrentTile, Character);
    }
}

void ARoundPlayerPawn::ServerEndTurnCharacter_Implementation(ARoundCharacter* Character)
{
    if (!Character || !CanIssueCommandForCharacter(Character))
    {
        return;
    }

    Character->EndTurn();
}

void ARoundPlayerPawn::ServerUseAbilityDirect_Implementation(ARoundCharacter* Character, int32 AbilityIndex)
{
    ServerUseAbilityDirectWithHandSelection(Character, AbilityIndex, {});
}

void ARoundPlayerPawn::ServerUseAbilityDirectWithHandSelection_Implementation(ARoundCharacter* Character, int32 AbilityIndex, const TArray<int32>& HandIndices)
{
    if (!Character || !CanIssueCommandForCharacter(Character))
    {
        return;
    }

    ApplyPendingHandSelectionToAbility(Character, AbilityIndex, HandIndices);
    const FRoundActionResult Result = Character->UseAbilityIndex(AbilityIndex, nullptr);
    ApplyPendingHandSelectionToAbility(Character, AbilityIndex, {});
    if (Result.bSuccess)
    {
        SelectedAction = ERoundActionType::None;
        SelectedAbilityIndex = INDEX_NONE;
        ARoundGridTile* FieldToKeep = Character->CurrentTile;
        if (FieldToKeep)
        {
            SelectField(FieldToKeep);
        }
        ClientPostActionResult(FieldToKeep, Character);
    }
}

void ARoundPlayerPawn::ServerStartObjectiveClaim_Implementation(ARoundCharacter* Character)
{
    if (!Character || !CanIssueCommandForCharacter(Character))
    {
        return;
    }

    if (ARoundGameState* RoundGameState = GetRoundGameState())
    {
        FString Reason;
        if (RoundGameState->TryStartObjectiveClaim(Character, Reason))
        {
            SelectedAction = ERoundActionType::None;
            SelectedAbilityIndex = INDEX_NONE;
            ClientPostActionResult(Character->CurrentTile, Character);
        }
    }
}

void ARoundPlayerPawn::ServerRequestInitialVisionRefresh_Implementation()
{
    if (ARoundGameState* RoundGameState = GetRoundGameState())
    {
        RoundGameState->SetVision();
    }
}

void ARoundPlayerPawn::ClientPostActionResult_Implementation(ARoundGridTile* FieldToSelect, ARoundCharacter* CharacterToKeepSelected)
{
    if (FieldToSelect)
    {
        SelectField(FieldToSelect);
        if (!SelectedField->OccupyingCharacter && CharacterToKeepSelected)
        {
            SelectedCharacterOverride = CharacterToKeepSelected;
        }
    }

    if (URoundGameUIWidget* GameUI = Cast<URoundGameUIWidget>(UI))
    {
        GameUI->RefreshFromPlayer();
        GameUI->RefreshButtons();
    }

    RefreshActionPreview();
    RefreshUI();
}

void ARoundPlayerPawn::HoverField(ARoundGridTile* Field)
{
    HoveredField = Field;
    RefreshActionPreview();
    RefreshUI();
}

void ARoundPlayerPawn::EndHoverField(ARoundGridTile* Field)
{
    if (HoveredField == Field)
    {
        HoveredField = nullptr;
    }

    RefreshActionPreview();
    RefreshUI();
}

void ARoundPlayerPawn::RefreshActionPreview()
{
    ARoundCharacter* Character = SelectedCharacter();
    if (!Character || !CanControlCharacter(Character))
    {
        ClearActionPreview();
        return;
    }

    RefreshActionRangePreview(Character);
    RefreshActionImpactPreview(Character);
}

void ARoundPlayerPawn::RefreshActionRangePreview(ARoundCharacter* Character)
{
    const ARoundGridTile* SourceTile = Character ? Character->CurrentTile.Get() : nullptr;
    const bool bNeedsRebuild =
        bActionRangePreviewDirty
        || CachedRangePreviewCharacter.Get() != Character
        || CachedRangePreviewSourceTile.Get() != SourceTile
        || CachedRangePreviewAction != SelectedAction
        || CachedRangePreviewAbilityIndex != SelectedAbilityIndex
        || CachedRangePreviewBoardRevision != ActionPreviewBoardRevision;

    if (!bNeedsRebuild)
    {
        return;
    }

    ResetPreviewTiles(ActionRangePreviewTiles);

    TArray<ARoundGridTile*> RangeTiles;
    switch (SelectedAction)
    {
    case ERoundActionType::Move:
        BuildMoveRangePreview(Character, RangeTiles);
        break;
    case ERoundActionType::Attack:
        BuildAttackRangePreview(Character, RangeTiles);
        break;
    case ERoundActionType::Ability:
        if (Character->GrantedAbilities.IsValidIndex(SelectedAbilityIndex))
        {
            BuildAbilityRangePreview(Character, Character->GrantedAbilities[SelectedAbilityIndex], RangeTiles);
        }
        break;
    default:
        break;
    }

    for (ARoundGridTile* Tile : RangeTiles)
    {
        if (!Tile)
        {
            continue;
        }

        ActionRangePreviewTiles.AddUnique(Tile);
        ApplyPreviewStateToTile(Tile, ERoundHoverFieldState::Range);
    }

    CachedRangePreviewCharacter = Character;
    CachedRangePreviewSourceTile = Character ? Character->CurrentTile.Get() : nullptr;
    CachedRangePreviewAction = SelectedAction;
    CachedRangePreviewAbilityIndex = SelectedAbilityIndex;
    CachedRangePreviewBoardRevision = ActionPreviewBoardRevision;
    bActionRangePreviewDirty = false;
}

void ARoundPlayerPawn::RefreshActionImpactPreview(ARoundCharacter* Character)
{
    ResetPreviewTiles(ActionImpactPreviewTiles);

    TArray<ARoundGridTile*> ImpactTiles;
    ERoundHoverFieldState ImpactState = ERoundHoverFieldState::None;
    BuildCurrentHoverImpactPreview(Character, ImpactTiles, ImpactState);
    for (ARoundGridTile* Tile : ImpactTiles)
    {
        if (!Tile)
        {
            continue;
        }

        ActionImpactPreviewTiles.AddUnique(Tile);
        ApplyPreviewStateToTile(Tile, ImpactState);
    }
}

void ARoundPlayerPawn::ClearActionPreview()
{
    ResetPreviewTiles(ActionImpactPreviewTiles);
    ResetPreviewTiles(ActionRangePreviewTiles);
    InvalidateActionRangePreview();
}

void ARoundPlayerPawn::InvalidateActionRangePreview()
{
    CachedRangePreviewCharacter = nullptr;
    CachedRangePreviewSourceTile = nullptr;
    CachedRangePreviewAction = ERoundActionType::None;
    CachedRangePreviewAbilityIndex = INDEX_NONE;
    CachedRangePreviewBoardRevision = ActionPreviewBoardRevision;
    bActionRangePreviewDirty = true;
}

void ARoundPlayerPawn::ResetPreviewTiles(TArray<TObjectPtr<ARoundGridTile>>& Tiles)
{
    const bool bResettingImpactPreview = &Tiles == &ActionImpactPreviewTiles;
    for (ARoundGridTile* Tile : Tiles)
    {
        if (!Tile)
        {
            continue;
        }

        if (Tile == SelectedField && Tile->bSelected)
        {
            Tile->SetSelectedState(true);
            continue;
        }

        if (bResettingImpactPreview && ActionRangePreviewTiles.Contains(Tile))
        {
            Tile->SetHoverState(ERoundHoverFieldState::Range);
            continue;
        }

        Tile->OnEndHover();
    }

    Tiles.Reset();
}

void ARoundPlayerPawn::BuildMoveRangePreview(ARoundCharacter* Character, TArray<ARoundGridTile*>& OutRangeTiles) const
{
    ARoundGameState* RoundGameState = GetRoundGameState();
    if (!Character || !RoundGameState || !Character->CurrentTile || !Character->Attributes)
    {
        return;
    }

    const int32 AvailableInitiative = FMath::Max(0, Character->Attributes->CurrentInitiative);
    if (AvailableInitiative <= 0 || Character->HasMovementBlock())
    {
        return;
    }

    // Calculate the complete reachable area once. Calling TryBuildMovePath for
    // every board tile repeats the same Dijkstra search hundreds of times.
    TMap<TObjectPtr<ARoundGridTile>, int32> BestCosts;
    TArray<TObjectPtr<ARoundGridTile>> OpenSet;
    BestCosts.Add(Character->CurrentTile, 0);
    OpenSet.Add(Character->CurrentTile);

    while (OpenSet.Num() > 0)
    {
        int32 BestIndex = 0;
        int32 BestCost = MAX_int32;
        for (int32 Index = 0; Index < OpenSet.Num(); ++Index)
        {
            if (const int32* CandidateCost = BestCosts.Find(OpenSet[Index]); CandidateCost && *CandidateCost < BestCost)
            {
                BestIndex = Index;
                BestCost = *CandidateCost;
            }
        }

        ARoundGridTile* Current = OpenSet[BestIndex];
        OpenSet.RemoveAtSwap(BestIndex);
        if (!Current || BestCost > AvailableInitiative)
        {
            continue;
        }

        if (Current != Character->CurrentTile)
        {
            OutRangeTiles.Add(Current);
            if (Current->TerrainStopsMovement())
            {
                continue;
            }
        }

        for (ARoundGridTile* Neighbor : RoundGameState->GetNeighborFields(Current))
        {
            if (!Neighbor || !Neighbor->IsMoveable() || Neighbor->TerrainType == ERoundTerrainType::Wall)
            {
                continue;
            }

            const bool bVisibleOccupantBlocksMovement = Neighbor->OccupyingCharacter != nullptr
                && Neighbor != Character->CurrentTile
                && Neighbor->bHasVision;
            if (bVisibleOccupantBlocksMovement)
            {
                continue;
            }

            const int32 StepCost = Neighbor->GetTerrainMoveCost(Character->GetMoveCost());
            if (StepCost == MAX_int32 || StepCost <= 0 || BestCost > AvailableInitiative - StepCost)
            {
                continue;
            }

            const int32 NewCost = BestCost + StepCost;
            const int32* ExistingCost = BestCosts.Find(Neighbor);
            if (!ExistingCost || NewCost < *ExistingCost)
            {
                BestCosts.Add(Neighbor, NewCost);
                OpenSet.AddUnique(Neighbor);
            }
        }
    }
}

void ARoundPlayerPawn::ApplyPreviewStateToTile(ARoundGridTile* Tile, const ERoundHoverFieldState State) const
{
    if (!Tile)
    {
        return;
    }

    if (Tile == SelectedField && Tile->bSelected)
    {
        Tile->SetSelectedState(true);
        return;
    }

    Tile->SetHoverState(State);
}

void ARoundPlayerPawn::BuildAttackRangePreview(ARoundCharacter* Character, TArray<ARoundGridTile*>& OutRangeTiles) const
{
    ARoundGameState* RoundGameState = GetRoundGameState();
    if (!Character || !RoundGameState || !Character->CurrentTile)
    {
        return;
    }

    const int32 AttackRange = FMath::Max(0, Character->GetAttackRange());
    for (ARoundGridTile* Tile : RoundGameState->GetFields())
    {
        if (!Tile || Tile == Character->CurrentTile)
        {
            continue;
        }

        if (Character->CurrentTile->GetDistance(Tile) <= AttackRange)
        {
            OutRangeTiles.AddUnique(Tile);
        }
    }
}

void ARoundPlayerPawn::BuildAbilityRangePreview(ARoundCharacter* Character, ARoundAbilityActor* Ability, TArray<ARoundGridTile*>& OutRangeTiles) const
{
    ARoundGameState* RoundGameState = GetRoundGameState();
    if (!Character || !Ability || !RoundGameState)
    {
        return;
    }

    const int32 AbilityRange = FMath::Max(0, Ability->GetAbilityRange());
    ARoundGridTile* SourceTile = Character->CurrentTile;

    for (ARoundGridTile* Tile : RoundGameState->GetFields())
    {
        if (!Tile)
        {
            continue;
        }

        bool bWithinRange = false;
        if (SourceTile && AbilityRange > 0)
        {
            bWithinRange = SourceTile->GetDistance(Tile) <= AbilityRange;
        }
        else
        {
            bWithinRange = (Tile == SourceTile);
        }

        if (bWithinRange)
        {
            OutRangeTiles.AddUnique(Tile);
        }
    }
}

void ARoundPlayerPawn::BuildCurrentHoverImpactPreview(ARoundCharacter* Character, TArray<ARoundGridTile*>& OutImpactTiles, ERoundHoverFieldState& OutState) const
{
    OutState = ERoundHoverFieldState::None;

    if (!Character || !HoveredField)
    {
        return;
    }

    switch (SelectedAction)
    {
    case ERoundActionType::Move:
    {
        TArray<ARoundGridTile*> MovePath;
        int32 PathCost = 0;
        ARoundGridTile* OriginalTile = Character->CurrentTile;
        if (SelectedField)
        {
            Character->CurrentTile = SelectedField;
        }

        if (Character->TryBuildMovePath(HoveredField, MovePath, PathCost, true))
        {
            OutImpactTiles = MovePath;
            const bool bEnoughInitiative = Character->Attributes && Character->Attributes->CurrentInitiative >= PathCost;
            OutState = bEnoughInitiative ? ERoundHoverFieldState::Ok : ERoundHoverFieldState::NotOk;
        }
        else
        {
            OutImpactTiles.AddUnique(HoveredField);
            OutState = ERoundHoverFieldState::NotOk;
        }

        Character->CurrentTile = OriginalTile;
        break;
    }
    case ERoundActionType::Attack:
    {
        OutImpactTiles.AddUnique(HoveredField);
        const bool bInRange = Character->CurrentTile && Character->CurrentTile->GetDistance(HoveredField) <= Character->GetAttackRange();
        const bool bCanAttack = GetRoundGameState() && GetRoundGameState()->IsSimAttackLegal(Character, HoveredField);
        OutState = bCanAttack ? ERoundHoverFieldState::Ok : ERoundHoverFieldState::NotOk;
        break;
    }
    case ERoundActionType::Ability:
    {
        if (!Character->GrantedAbilities.IsValidIndex(SelectedAbilityIndex) || !Character->GrantedAbilities[SelectedAbilityIndex])
        {
            OutImpactTiles.AddUnique(HoveredField);
            OutState = ERoundHoverFieldState::NotOk;
            break;
        }

        ARoundAbilityActor* Ability = Character->GrantedAbilities[SelectedAbilityIndex];
        const bool bCanTarget = Ability->CanTargetTile(HoveredField);
        Ability->GetPreviewTiles(HoveredField, OutImpactTiles);
        if (OutImpactTiles.Num() == 0)
        {
            OutImpactTiles.AddUnique(HoveredField);
        }

        OutState = bCanTarget ? ERoundHoverFieldState::Ok : ERoundHoverFieldState::NotOk;
        break;
    }
    default:
        break;
    }
}

void ARoundPlayerPawn::UpdateVision()
{
    ARoundGameState* RoundGameState = GetRoundGameState();
    if (!RoundGameState)
    {
        return;
    }

    if (RoundGameState->IsReplayActive())
    {
        for (ARoundGridTile* Field : RoundGameState->GetFields())
        {
            if (Field)
            {
                Field->SetHasVision(true);
            }
        }

        for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
        {
            if (ARoundCharacter* Character = *It)
            {
                Character->SetActorHiddenInGame(false);
                Character->SetActorEnableCollision(true);
            }
        }

        for (TActorIterator<ARoundFieldObjectActor> It(GetWorld()); It; ++It)
        {
            if (ARoundFieldObjectActor* FieldObject = *It)
            {
                FieldObject->SetActorHiddenInGame(false);
                FieldObject->SetActorEnableCollision(true);
            }
        }

        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(VisionRetryTimerHandle);
        }
        RemainingVisionRetryCount = 0;
        return;
    }

    const ERoundTeam EffectiveTeam = GetEffectiveTeam();

    TSet<TObjectPtr<ARoundGridTile>> VisibleFields;
    int32 TotalCharacterCount = 0;
    int32 TeamCharacterCount = 0;
    int32 TeamCharactersWithTile = 0;

    for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
    {
        ARoundCharacter* Character = *It;
        ++TotalCharacterCount;
        if (!Character || !Character->IsAlive() || Character->Team != EffectiveTeam)
        {
            continue;
        }

        ++TeamCharacterCount;
        if (Character->CurrentTile)
        {
            ++TeamCharactersWithTile;
        }

        for (ARoundGridTile* Field : Character->GetVisionFields())
        {
            if (Field)
            {
                VisibleFields.Add(Field);
            }
        }
    }

    const bool bCanUseReplicatedTeamVision = TeamActor
        && TeamActor->TeamId == EffectiveTeam
        && TeamActor->VisionFields.Num() > 0;

    if (VisibleFields.Num() == 0 && bCanUseReplicatedTeamVision)
    {
        for (ARoundGridTile* Field : TeamActor->VisionFields)
        {
            if (Field)
            {
                VisibleFields.Add(Field);
            }
        }
    }

    if (bDebugRevealMap)
    {
        for (ARoundGridTile* Field : RoundGameState->GetFields())
        {
            if (Field) VisibleFields.Add(Field);
        }
    }

    for (ARoundGridTile* Field : RoundGameState->GetFields())
    {
        if (Field)
        {
            Field->SetHasVision(VisibleFields.Contains(Field));
        }
    }

    for (TActorIterator<ARoundCharacter> It(GetWorld()); It; ++It)
    {
        ARoundCharacter* Character = *It;
        if (!Character)
        {
            continue;
        }

        const bool bVisible = Character->IsAlive()
            && (Character->Team == EffectiveTeam
                || (Character->CurrentTile && VisibleFields.Contains(Character->CurrentTile)));
        Character->SetActorHiddenInGame(!bVisible);
        Character->SetActorEnableCollision(bVisible);
    }

    for (TActorIterator<ARoundFieldObjectActor> It(GetWorld()); It; ++It)
    {
        ARoundFieldObjectActor* FieldObject = *It;
        if (!FieldObject || !FieldObject->GetClass()->ImplementsInterface(URoundDeviceInterface::StaticClass()))
        {
            continue;
        }

        ARoundGridTile* DeviceTile = IRoundDeviceInterface::Execute_GetDeviceTile(FieldObject);
        const bool bTileInVision = DeviceTile && VisibleFields.Contains(DeviceTile);
        const bool bVisible = IRoundDeviceInterface::Execute_IsDeviceVisibleToTeam(FieldObject, EffectiveTeam, bTileInVision);
        FieldObject->SetActorHiddenInGame(!bVisible);
        FieldObject->SetActorEnableCollision(bVisible);
    }

    if (VisibleFields.Num() > 0)
    {
        RemainingVisionRetryCount = 0;
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().ClearTimer(VisionRetryTimerHandle);
        }
    }
    else if (IsLocallyControlled()
        && EffectiveTeam != ERoundTeam::Neutral
        && (TeamCharacterCount == 0 || TeamCharactersWithTile < TeamCharacterCount || VisibleFields.Num() == 0))
    {
        ScheduleVisionRetry();
    }
}

ERoundTeam ARoundPlayerPawn::GetEffectiveTeam() const
{
    if (const ARoundGameState* RoundGameState = GetRoundGameState())
    {
        if (RoundGameState->IsReplayActive())
        {
            return ERoundTeam::Neutral;
        }
    }

    ERoundTeam EffectiveTeam = Team;
    if (EffectiveTeam == ERoundTeam::Neutral && TeamActor)
    {
        EffectiveTeam = TeamActor->TeamId;
    }

    if (EffectiveTeam == ERoundTeam::Neutral)
    {
        if (const ARoundGameState* RoundGameState = GetRoundGameState())
        {
            if (RoundGameState->Player1 == this)
            {
                EffectiveTeam = ERoundTeam::TeamA;
            }
            else if (RoundGameState->Player2 == this)
            {
                EffectiveTeam = ERoundTeam::TeamB;
            }
        }
    }

    return EffectiveTeam;
}

bool ARoundPlayerPawn::CanSeeCharacter(const ARoundCharacter* Character) const
{
    if (!Character)
    {
        return false;
    }

    if (const ARoundGameState* RoundGameState = GetRoundGameState())
    {
        if (RoundGameState->IsReplayActive())
        {
            return true;
        }
    }

    if (bDebugRevealMap) return true;

    const ERoundTeam EffectiveTeam = GetEffectiveTeam();
    if (Character->Team == EffectiveTeam)
    {
        return true;
    }

    return Character->CurrentTile && Character->CurrentTile->bHasVision;
}

bool ARoundPlayerPawn::CanSeeTile(const ARoundGridTile* Tile) const
{
    if (!Tile)
    {
        return false;
    }

    if (const ARoundGameState* RoundGameState = GetRoundGameState())
    {
        if (RoundGameState->IsReplayActive())
        {
            return true;
        }
    }

    return bDebugRevealMap || Tile->bHasVision;
}

ARoundCharacter* ARoundPlayerPawn::GetDebugSelectedCharacter() const
{
    if (SelectedField && SelectedField->OccupyingCharacter) return SelectedField->OccupyingCharacter;
    return SelectedCharacterOverride;
}

void ARoundPlayerPawn::ToggleDebugMode()
{
#if UE_BUILD_SHIPPING
    return;
#else
    const URoundGameDataRegistry* Registry = RoundGameDataRegistryLookup::ResolveGameDataRegistry();
    if (!IsLocallyControlled() || !Registry || !Registry->bEnableDebugTools) return;
    if (!DebugUI)
    {
        const TSubclassOf<URoundDebugWidget> DebugClass = RoundGameDataRegistryLookup::ResolveDebugWidgetClass();
        if (!DebugClass) return;
        DebugUI = CreateWidget<URoundDebugWidget>(Cast<APlayerController>(GetController()), DebugClass);
        if (DebugUI) DebugUI->SetPlayerPawn(this);
    }
    if (!DebugUI) return;
    if (DebugUI->IsInViewport()) DebugUI->RemoveFromParent();
    else { DebugUI->AddToViewport(50); DebugUI->RefreshDebugInfo(); }
#endif
}

void ARoundPlayerPawn::SetDebugRevealMap(const bool bEnabled)
{
#if !UE_BUILD_SHIPPING
    bDebugRevealMap = bEnabled;
    UpdateVision();
#endif
}

void ARoundPlayerPawn::SetDebugTerrainLabels(const bool bEnabled)
{
#if !UE_BUILD_SHIPPING
    bDebugTerrainLabels = bEnabled;
    const URoundGameDataRegistry* GameRegistry = RoundGameDataRegistryLookup::ResolveGameDataRegistry();
    UMaterialInterface* OverlayMaterial = GameRegistry ? GameRegistry->DebugRegionOverlayMaterial.LoadSynchronous() : nullptr;
    URoundTerrainRegionRegistryData* RegionRegistry = RoundGameDataRegistryLookup::ResolveTerrainRegionRegistry();
    if (ARoundGameState* State = GetRoundGameState())
    {
        for (ARoundGridTile* Tile : State->GetFields())
        {
            if (!Tile) continue;
            Tile->SetDebugTerrainLabelVisible(false);
            FLinearColor Color(0.25f, 0.55f, 1.0f, 0.55f);
            if (RegionRegistry)
            {
                if (const URoundTerrainRegionData* Region = RegionRegistry->GetRegion(Tile->TerrainRegionId))
                {
                    Color = Region->DebugColor;
                }
            }
            if (Tile->TerrainRegionIndex != INDEX_NONE)
            {
                const float Brightness = 0.84f + static_cast<float>(Tile->TerrainRegionIndex % 5) * 0.08f;
                Color.R = FMath::Clamp(Color.R * Brightness, 0.0f, 1.0f);
                Color.G = FMath::Clamp(Color.G * Brightness, 0.0f, 1.0f);
                Color.B = FMath::Clamp(Color.B * Brightness, 0.0f, 1.0f);
            }
            Tile->SetDebugRegionOverlay(bEnabled, Color, OverlayMaterial);
        }
    }
#endif
}

void ARoundPlayerPawn::DebugFullHeal() { if (ARoundCharacter* C = GetDebugSelectedCharacter()) ServerDebugModifyCharacter(C, 0, 0); }
void ARoundPlayerPawn::DebugApplyDamage()
{
    const URoundGameDataRegistry* R = RoundGameDataRegistryLookup::ResolveGameDataRegistry();
    if (ARoundCharacter* C = GetDebugSelectedCharacter()) ServerDebugModifyCharacter(C, 1, R ? R->DebugDamageStep : 1);
}
void ARoundPlayerPawn::DebugAdjustInitiative(const bool bIncrease)
{
    const URoundGameDataRegistry* R = RoundGameDataRegistryLookup::ResolveGameDataRegistry();
    if (ARoundCharacter* C = GetDebugSelectedCharacter()) ServerDebugModifyCharacter(C, bIncrease ? 2 : 3, R ? R->DebugInitiativeStep : 1);
}
void ARoundPlayerPawn::DebugResetCooldowns() { if (ARoundCharacter* C = GetDebugSelectedCharacter()) ServerDebugModifyCharacter(C, 4, 0); }

void ARoundPlayerPawn::ServerDebugModifyCharacter_Implementation(ARoundCharacter* Character, const int32 Operation, const int32 Amount)
{
#if !UE_BUILD_SHIPPING
    const URoundGameDataRegistry* Registry = RoundGameDataRegistryLookup::ResolveGameDataRegistry();
    if (!Registry || !Registry->bEnableDebugTools || !Character || !Character->Attributes) return;
    switch (Operation)
    {
    case 0: Character->Attributes->SetCurrentHealthValue(Character->Attributes->BaseStats.MaxHealth); break;
    case 1: Character->Attributes->ApplyFinalDamage(FMath::Max(1, Amount)); break;
    case 2: Character->Attributes->SetCurrentInitiativeValue(Character->Attributes->CurrentInitiative + FMath::Max(1, Amount)); break;
    case 3: Character->Attributes->SetCurrentInitiativeValue(Character->Attributes->CurrentInitiative - FMath::Max(1, Amount)); break;
    case 4: for (ARoundAbilityActor* Ability : Character->GrantedAbilities) if (Ability) Ability->SetCooldown(0); break;
    default: return;
    }
    Character->ForceNetUpdate();
    if (ARoundGameState* State = GetRoundGameState()) State->OnBoardStateChanged.Broadcast();
#endif
}

void ARoundPlayerPawn::ClientRefreshVision_Implementation()
{
    UpdateVision();
    ScheduleVisionRetry();
}

void ARoundPlayerPawn::ClientPlayPresentationEventSound_Implementation(
    USoundBase* Sound,
    const float VolumeMultiplier,
    const float PitchMultiplier,
    const float MaxDurationSeconds)
{
    if (!Sound)
    {
        return;
    }

    UAudioComponent* AudioComponent = UGameplayStatics::SpawnSound2D(
        this,
        Sound,
        VolumeMultiplier,
        PitchMultiplier);
    if (!AudioComponent || MaxDurationSeconds <= 0.0f || !GetWorld())
    {
        return;
    }

    const float FadeOutSeconds = FMath::Min(0.08f, MaxDurationSeconds);
    const float HoldSeconds = FMath::Max(0.0f, MaxDurationSeconds - FadeOutSeconds);
    if (HoldSeconds <= 0.0f)
    {
        AudioComponent->FadeOut(FadeOutSeconds, 0.0f);
        return;
    }

    TWeakObjectPtr<UAudioComponent> WeakAudioComponent = AudioComponent;
    FTimerHandle LocalFadeHandle;
    GetWorld()->GetTimerManager().SetTimer(
        LocalFadeHandle,
        [WeakAudioComponent, FadeOutSeconds]()
        {
            if (WeakAudioComponent.IsValid())
            {
                WeakAudioComponent->FadeOut(FadeOutSeconds, 0.0f);
            }
        },
        HoldSeconds,
        false);
}

void ARoundPlayerPawn::ScheduleVisionRetry()
{
    if (!GetWorld())
    {
        return;
    }

      if (RemainingVisionRetryCount <= 0)
      {
          RemainingVisionRetryCount = 30;
      }

    if (GetWorld()->GetTimerManager().IsTimerActive(VisionRetryTimerHandle))
    {
        return;
    }

      GetWorld()->GetTimerManager().SetTimer(
          VisionRetryTimerHandle,
          [this]()
          {
            if (!IsValid(this))
            {
                return;
            }

              if (RemainingVisionRetryCount > 0)
              {
                  --RemainingVisionRetryCount;
                  UpdateVision();
              }
              else if (GetWorld())
              {
                  GetWorld()->GetTimerManager().ClearTimer(VisionRetryTimerHandle);
              }
          },
        0.1f,
        true);
}

void ARoundPlayerPawn::ConfigureLocalInput()
{
    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        PlayerController->bShowMouseCursor = true;
        PlayerController->bEnableClickEvents = true;
        PlayerController->bEnableMouseOverEvents = true;

        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
        PlayerController->SetInputMode(InputMode);

        if (!bDebugInputBound)
        {
            EnableInput(PlayerController);
            if (InputComponent)
            {
                InputComponent->BindKey(EKeys::F10, IE_Pressed, this, &ARoundPlayerPawn::ToggleDebugMode);
                bDebugInputBound = true;
            }
        }

    }
}

void ARoundPlayerPawn::UpdateCameraMovement(const float DeltaSeconds)
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController || !PlayerController->IsLocalController())
    {
        return;
    }

    FVector2D MousePosition = FVector2D::ZeroVector;
    if (!PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y))
    {
        return;
    }

    const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(PlayerController);
    if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
    {
        return;
    }

    const double LeftDistance = MousePosition.X;
    const double RightDistance = ViewportSize.X - MousePosition.X;
    const double TopDistance = MousePosition.Y;
    const double BottomDistance = ViewportSize.Y - MousePosition.Y;

    double HorizontalDirection = 0.0;
    if (LeftDistance <= CameraEdgePanPixels)
    {
        HorizontalDirection = -1.0 + FMath::Clamp(static_cast<double>(LeftDistance / FMath::Max(CameraEdgePanPixels, 1.0f)), 0.0, 1.0);
    }
    else if (RightDistance <= CameraEdgePanPixels)
    {
        HorizontalDirection = 1.0 - FMath::Clamp(static_cast<double>(RightDistance / FMath::Max(CameraEdgePanPixels, 1.0f)), 0.0, 1.0);
    }

    double VerticalDirection = 0.0;
    if (TopDistance <= CameraEdgePanPixels)
    {
        VerticalDirection = 1.0 - FMath::Clamp(static_cast<double>(TopDistance / FMath::Max(CameraEdgePanPixels, 1.0f)), 0.0, 1.0);
    }
    else if (BottomDistance <= CameraEdgePanPixels)
    {
        VerticalDirection = -1.0 + FMath::Clamp(static_cast<double>(BottomDistance / FMath::Max(CameraEdgePanPixels, 1.0f)), 0.0, 1.0);
    }

    if (HorizontalDirection == 0.0 && VerticalDirection == 0.0)
    {
        return;
    }

    const FRotator CurrentControlRotation = PlayerController->GetControlRotation();
    const FRotator YawOnlyRotation(0.0f, CurrentControlRotation.Yaw, 0.0f);
    const FVector RightVector = UKismetMathLibrary::GetRightVector(YawOnlyRotation);
    const FVector ForwardVector = UKismetMathLibrary::GetForwardVector(YawOnlyRotation);
    const FVector MoveOffset = ((RightVector * HorizontalDirection) + (ForwardVector * VerticalDirection)) * CameraMoveSpeed * DeltaSeconds;

    SetActorLocation(ClampCameraLocationToBoard(GetActorLocation() + MoveOffset));
}

void ARoundPlayerPawn::UpdateCameraZoom()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController || !PlayerController->PlayerInput || !PlayerController->IsLocalController())
    {
        return;
    }

    const float MouseWheelDelta = PlayerController->PlayerInput->GetKeyValue(EKeys::MouseWheelAxis);
    if (FMath::IsNearlyZero(MouseWheelDelta))
    {
        return;
    }

    if (USpringArmComponent* SpringArm = FindComponentByClass<USpringArmComponent>())
    {
        SpringArm->TargetArmLength = FMath::Clamp(SpringArm->TargetArmLength + (MouseWheelDelta * -40.0f), 500.0f, 10000.0f);
    }
}

FVector ARoundPlayerPawn::ClampCameraLocationToBoard(const FVector& DesiredLocation) const
{
    const ARoundGameState* RoundGameState = GetRoundGameState();
    if (!RoundGameState)
    {
        return DesiredLocation;
    }

    const TArray<ARoundGridTile*> Fields = RoundGameState->GetFields();
    if (Fields.Num() <= 0)
    {
        return DesiredLocation;
    }

    bool bInitializedBounds = false;
    float MinX = 0.0f;
    float MaxX = 0.0f;
    float MinY = 0.0f;
    float MaxY = 0.0f;

    for (const ARoundGridTile* Field : Fields)
    {
        if (!Field)
        {
            continue;
        }

        const FVector FieldLocation = Field->GetActorLocation();
        if (!bInitializedBounds)
        {
            MinX = MaxX = FieldLocation.X;
            MinY = MaxY = FieldLocation.Y;
            bInitializedBounds = true;
            continue;
        }

        MinX = FMath::Min(MinX, FieldLocation.X);
        MaxX = FMath::Max(MaxX, FieldLocation.X);
        MinY = FMath::Min(MinY, FieldLocation.Y);
        MaxY = FMath::Max(MaxY, FieldLocation.Y);
    }

    if (!bInitializedBounds)
    {
        return DesiredLocation;
    }

    FVector ClampedLocation = DesiredLocation;
    ClampedLocation.X = FMath::Clamp(DesiredLocation.X, MinX - CameraFieldBoundsPadding, MaxX + CameraFieldBoundsPadding);
    ClampedLocation.Y = FMath::Clamp(DesiredLocation.Y, MinY - CameraFieldBoundsPadding, MaxY + CameraFieldBoundsPadding);
    return ClampedLocation;
}

void ARoundPlayerPawn::SnapCameraToField(const ARoundGridTile* Field, const bool bInstant)
{
    if (!Field)
    {
        return;
    }

    const FVector TargetLocation = ClampCameraLocationToBoard(FVector(
        Field->GetActorLocation().X,
        Field->GetActorLocation().Y,
        GetActorLocation().Z));

    if (bInstant || CameraSnapInterpolationSpeed <= 0.0f || !GetWorld())
    {
        SetActorLocation(TargetLocation);
        return;
    }

    SetActorLocation(FMath::VInterpTo(GetActorLocation(), TargetLocation, GetWorld()->GetDeltaSeconds(), CameraSnapInterpolationSpeed));
}
