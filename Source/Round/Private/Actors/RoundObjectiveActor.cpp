#include "Actors/RoundObjectiveActor.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Actors/RoundObjectiveRewardActor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Data/RoundGameDataRegistryLookup.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Blueprint/UserWidget.h"
#include "Data/RoundObjectiveRewardData.h"
#include "Game/RoundGameState.h"
#include "Net/UnrealNetwork.h"
#include "UI/RoundHealthbarWidget.h"

namespace
{
constexpr float ObjectiveBarWidth = 0.95f;
constexpr float ObjectiveBarHeight = 0.08f;
constexpr float ObjectiveBarDepth = 0.08f;
constexpr float ObjectiveBarVerticalOffset = 135.0f;

FString GetObjectiveTypeLabel(const ERoundObjectiveType ObjectiveType)
{
    switch (ObjectiveType)
    {
    case ERoundObjectiveType::HoldZone:
        return TEXT("Hold");
    case ERoundObjectiveType::DestroyNode:
        return TEXT("Destroy");
    case ERoundObjectiveType::Channel:
        return TEXT("Channel");
    default:
        return TEXT("Objective");
    }
}
}

ARoundObjectiveActor::ARoundObjectiveActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRender"));
    TextRender->SetupAttachment(SceneRoot);
    TextRender->SetHorizontalAlignment(EHTA_Center);
    TextRender->SetWorldSize(54.0f);
    TextRender->SetTextRenderColor(FColor::Cyan);
    TextRender->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
    TextRender->SetHiddenInGame(true);

    MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
    MarkerMesh->SetupAttachment(SceneRoot);
    MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MarkerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
    MarkerMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.2f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MarkerMeshAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (MarkerMeshAsset.Succeeded())
    {
        MarkerMesh->SetStaticMesh(MarkerMeshAsset.Object);
    }

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ProgressBarMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (ProgressBarMeshAsset.Succeeded())
    {
        ProgressBarMesh = ProgressBarMeshAsset.Object;
    }

    MarkerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MarkerLight"));
    MarkerLight->SetupAttachment(SceneRoot);
    MarkerLight->SetRelativeLocation(FVector(0.0f, 0.0f, 85.0f));
    MarkerLight->SetIntensity(2200.0f);
    MarkerLight->SetAttenuationRadius(180.0f);

    ProgressBarBackground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProgressBarBackground"));
    ProgressBarBackground->SetupAttachment(SceneRoot);
    ProgressBarBackground->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProgressBarBackground->SetRelativeLocation(FVector(0.0f, 0.0f, ObjectiveBarVerticalOffset));
    ProgressBarBackground->SetRelativeScale3D(FVector(ObjectiveBarWidth, ObjectiveBarHeight, ObjectiveBarDepth));
    ProgressBarBackground->SetVisibility(false);
    if (ProgressBarMesh)
    {
        ProgressBarBackground->SetStaticMesh(ProgressBarMesh);
    }

    ProgressBarFill = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProgressBarFill"));
    ProgressBarFill->SetupAttachment(SceneRoot);
    ProgressBarFill->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProgressBarFill->SetRelativeLocation(FVector(0.0f, 0.0f, ObjectiveBarVerticalOffset + 1.0f));
    ProgressBarFill->SetRelativeScale3D(FVector(ObjectiveBarWidth, ObjectiveBarHeight * 0.7f, ObjectiveBarDepth * 0.7f));
    ProgressBarFill->SetVisibility(false);
    if (ProgressBarMesh)
    {
        ProgressBarFill->SetStaticMesh(ProgressBarMesh);
    }

    ProgressWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ProgressWidgetComponent"));
    ProgressWidgetComponent->SetupAttachment(SceneRoot);
    ProgressWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
    ProgressWidgetComponent->SetDrawAtDesiredSize(false);
    ProgressWidgetComponent->SetDrawSize(FVector2D(120.0f, 28.0f));
    ProgressWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
    ProgressWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 350.0f));
    ProgressWidgetComponent->SetRelativeRotation(FRotator::ZeroRotator);
    ProgressWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProgressWidgetComponent->SetGenerateOverlapEvents(false);
    ProgressWidgetComponent->SetHiddenInGame(false);
    ProgressWidgetComponent->SetVisibility(false);

    if (UClass* ObjectiveProgressWidgetClass = RoundGameDataRegistryLookup::ResolveHealthbarWidgetClass().Get())
    {
        ProgressWidgetComponent->SetWidgetClass(ObjectiveProgressWidgetClass);
    }
}

void ARoundObjectiveActor::PostNetReceive()
{
    Super::PostNetReceive();
    RefreshVisuals();
}

void ARoundObjectiveActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundObjectiveActor, ObjectiveId);
    DOREPLIFETIME(ARoundObjectiveActor, RewardId);
    DOREPLIFETIME(ARoundObjectiveActor, ObjectiveType);
    DOREPLIFETIME(ARoundObjectiveActor, ObjectiveState);
    DOREPLIFETIME(ARoundObjectiveActor, CenterTile);
    DOREPLIFETIME(ARoundObjectiveActor, ObjectiveTiles);
    DOREPLIFETIME(ARoundObjectiveActor, RewardDisplayName);
    DOREPLIFETIME(ARoundObjectiveActor, RewardDescription);
    DOREPLIFETIME(ARoundObjectiveActor, RewardType);
    DOREPLIFETIME(ARoundObjectiveActor, RewardValue);
    DOREPLIFETIME(ARoundObjectiveActor, RewardDurationInitiative);
    DOREPLIFETIME(ARoundObjectiveActor, bRewardPermanent);
    DOREPLIFETIME(ARoundObjectiveActor, SpawnAtGlobalInitiative);
    DOREPLIFETIME(ARoundObjectiveActor, ExpireAtGlobalInitiative);
    DOREPLIFETIME(ARoundObjectiveActor, RequiredProgressInitiative);
    DOREPLIFETIME(ARoundObjectiveActor, CurrentProgressInitiative);
    DOREPLIFETIME(ARoundObjectiveActor, CapturingTeam);
    DOREPLIFETIME(ARoundObjectiveActor, MaxHealth);
    DOREPLIFETIME(ARoundObjectiveActor, CurrentHealth);
    DOREPLIFETIME(ARoundObjectiveActor, ChannelingCharacter);
    DOREPLIFETIME(ARoundObjectiveActor, ChannelRemainingInitiative);
}

void ARoundObjectiveActor::InitializeObjective(
    const ERoundObjectiveType InType,
    ARoundGridTile* InCenterTile,
    const TArray<ARoundGridTile*>& InObjectiveTiles,
    URoundObjectiveRewardData* InRewardData,
    const int32 InSpawnAtGlobalInitiative,
    const int32 InExpireAtGlobalInitiative,
    const int32 InRequiredProgressInitiative,
    const int32 InMaxHealth)
{
    ObjectiveType = InType;
    CenterTile = InCenterTile;
    ApplyRewardData(InRewardData);
    SpawnAtGlobalInitiative = InSpawnAtGlobalInitiative;
    ExpireAtGlobalInitiative = InExpireAtGlobalInitiative;
    RequiredProgressInitiative = FMath::Max(1, InRequiredProgressInitiative);
    MaxHealth = FMath::Max(0, InMaxHealth);
    CurrentHealth = MaxHealth;
    ObjectiveTiles.Reset();
    for (ARoundGridTile* Tile : InObjectiveTiles)
    {
        if (Tile)
        {
            ObjectiveTiles.AddUnique(Tile);
        }
    }

    if (CenterTile)
    {
        SetActorLocation(CenterTile->GetActorLocation());
    }

    ObjectiveState = ERoundObjectiveState::Preview;
    ObjectiveId = FString::Printf(TEXT("%s_%d_%d"), *GetObjectiveTypeLabel(ObjectiveType), FMath::RoundToInt(CenterTile ? CenterTile->GridPosition.X : 0.0f), FMath::RoundToInt(CenterTile ? CenterTile->GridPosition.Y : 0.0f));
    RefreshVisuals();
}

bool ARoundObjectiveActor::AdvanceObjectiveTime(ARoundGameState* GameState, const int32 CurrentGlobalInitiative, const int32 InitiativeAmount)
{
    if (!GameState || InitiativeAmount <= 0)
    {
        return false;
    }

    if (ObjectiveState == ERoundObjectiveState::Preview && CurrentGlobalInitiative >= SpawnAtGlobalInitiative)
    {
        ObjectiveState = ERoundObjectiveState::Active;
        UE_LOG(LogTemp, Log, TEXT("Objective AdvanceObjectiveTime: Objective=%s transitioned Preview->Active at GlobalInitiative=%d CenterTile=%s CenterPos=%s Type=%d"),
            *GetNameSafe(this),
            CurrentGlobalInitiative,
            *GetNameSafe(CenterTile),
            CenterTile ? *CenterTile->GridPosition.ToString() : TEXT("None"),
            static_cast<int32>(ObjectiveType));
        RefreshVisuals();
    }

    if (ObjectiveState != ERoundObjectiveState::Active)
    {
        return false;
    }

    if (ExpireAtGlobalInitiative > 0 && CurrentGlobalInitiative >= ExpireAtGlobalInitiative)
    {
        ObjectiveState = ERoundObjectiveState::Completed;
        Destroy();
        return true;
    }

    if (ObjectiveType == ERoundObjectiveType::HoldZone)
    {
        TMap<ERoundTeam, int32> TeamPresence;
        for (ARoundCharacter* Character : GameState->RegisteredCharacters)
        {
            if (!Character || !Character->IsAlive() || !Character->CurrentTile)
            {
                continue;
            }

            if (Character->CharacterRowName.ToString().Contains(TEXT("Puppet"), ESearchCase::IgnoreCase))
            {
                continue;
            }

            if (ObjectiveTiles.Contains(Character->CurrentTile))
            {
                TeamPresence.FindOrAdd(Character->Team)++;
            }
        }

        const int32 TeamAPresence = TeamPresence.FindRef(ERoundTeam::TeamA);
        const int32 TeamBPresence = TeamPresence.FindRef(ERoundTeam::TeamB);
        const ERoundTeam SoleTeam = TeamAPresence > 0 && TeamBPresence == 0
            ? ERoundTeam::TeamA
            : (TeamBPresence > 0 && TeamAPresence == 0 ? ERoundTeam::TeamB : ERoundTeam::Neutral);

        if (SoleTeam != ERoundTeam::Neutral)
        {
            if (CapturingTeam != SoleTeam)
            {
                CapturingTeam = SoleTeam;
                CurrentProgressInitiative = 0;
                RefreshVisuals();
                return false;
            }

            CurrentProgressInitiative += InitiativeAmount;
            if (CurrentProgressInitiative >= RequiredProgressInitiative)
            {
                return DropReward(GameState);
            }
        }
        else if (CurrentProgressInitiative > 0 || CapturingTeam != ERoundTeam::Neutral)
        {
            CurrentProgressInitiative = 0;
            CapturingTeam = ERoundTeam::Neutral;
        }
    }
    else if (ObjectiveType == ERoundObjectiveType::Channel)
    {
        if (!ChannelingCharacter || !ChannelingCharacter->IsAlive() || ChannelingCharacter->CurrentTile != CenterTile)
        {
            CancelChannel();
            return false;
        }

        ChannelRemainingInitiative = FMath::Max(0, ChannelRemainingInitiative - InitiativeAmount);
        if (ChannelRemainingInitiative <= 0)
        {
            return DropReward(GameState);
        }
    }

    RefreshVisuals();
    return false;
}

void ARoundObjectiveActor::RefreshVisuals()
{
    if (!TextRender)
    {
        return;
    }

    if (ProgressWidgetComponent && !ProgressWidgetComponent->GetWidgetClass())
    {
        if (UClass* ObjectiveProgressWidgetClass = RoundGameDataRegistryLookup::ResolveHealthbarWidgetClass().Get())
        {
            ProgressWidgetComponent->SetWidgetClass(ObjectiveProgressWidgetClass);
        }
    }

    const FString RewardLabel = RewardData && !RewardData->DisplayName.IsEmpty()
        ? RewardData->DisplayName.ToString()
        : (!RewardDisplayName.IsEmpty() ? RewardDisplayName : TEXT("Reward"));

    FString Label;
    switch (ObjectiveState)
    {
    case ERoundObjectiveState::Preview:
        Label = FString::Printf(TEXT("%s -> %s"), *GetObjectiveTypeLabel(ObjectiveType), *RewardLabel);
        TextRender->SetTextRenderColor(FColor::Yellow);
        if (MarkerMesh)
        {
            MarkerMesh->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.12f));
        }
        if (MarkerLight)
        {
            MarkerLight->SetLightColor(FLinearColor(1.0f, 0.9f, 0.25f));
        }
        break;
    case ERoundObjectiveState::Active:
        if (ObjectiveType == ERoundObjectiveType::HoldZone)
        {
            Label = FString::Printf(TEXT("Hold %d/%d"), CurrentProgressInitiative, RequiredProgressInitiative);
        }
        else if (ObjectiveType == ERoundObjectiveType::DestroyNode)
        {
            Label = FString::Printf(TEXT("Destroy %d"), CurrentHealth);
        }
        else
        {
            Label = ChannelingCharacter
                ? FString::Printf(TEXT("Claim %d"), ChannelRemainingInitiative)
                : TEXT("Claim");
        }
        TextRender->SetTextRenderColor(FColor::Cyan);
        if (MarkerMesh)
        {
            MarkerMesh->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.2f));
        }
        if (MarkerLight)
        {
            switch (ObjectiveType)
            {
            case ERoundObjectiveType::HoldZone:
                MarkerLight->SetLightColor(FLinearColor(0.2f, 0.75f, 1.0f));
                break;
            case ERoundObjectiveType::DestroyNode:
                MarkerLight->SetLightColor(FLinearColor(1.0f, 0.3f, 0.2f));
                break;
            case ERoundObjectiveType::Channel:
                MarkerLight->SetLightColor(FLinearColor(0.65f, 0.35f, 1.0f));
                break;
            default:
                MarkerLight->SetLightColor(FLinearColor::White);
                break;
            }
        }
        break;
    default:
        Label = RewardLabel;
        break;
    }

    TextRender->SetText(FText::FromString(Label));
    RefreshProgressBarVisuals();
    RefreshProgressWidget();
}

void ARoundObjectiveActor::RefreshProgressBarVisuals()
{
    if (!ProgressBarBackground || !ProgressBarFill)
    {
        return;
    }

    float ProgressAlpha = 0.0f;
    FLinearColor FillColor = FLinearColor(0.3f, 0.8f, 1.0f, 1.0f);
    bool bShowBar = ObjectiveState == ERoundObjectiveState::Active;

    switch (ObjectiveType)
    {
    case ERoundObjectiveType::HoldZone:
        ProgressAlpha = RequiredProgressInitiative > 0
            ? static_cast<float>(CurrentProgressInitiative) / static_cast<float>(RequiredProgressInitiative)
            : 0.0f;
        FillColor = CapturingTeam == ERoundTeam::TeamA
            ? FLinearColor(0.25f, 0.65f, 1.0f, 1.0f)
            : (CapturingTeam == ERoundTeam::TeamB ? FLinearColor(1.0f, 0.35f, 0.35f, 1.0f) : FLinearColor(0.7f, 0.7f, 0.7f, 1.0f));
        break;
    case ERoundObjectiveType::DestroyNode:
        ProgressAlpha = MaxHealth > 0
            ? static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth)
            : 0.0f;
        FillColor = FLinearColor(1.0f, 0.3f, 0.2f, 1.0f);
        break;
    case ERoundObjectiveType::Channel:
        ProgressAlpha = RequiredProgressInitiative > 0
            ? 1.0f - (static_cast<float>(ChannelRemainingInitiative) / static_cast<float>(RequiredProgressInitiative))
            : 0.0f;
        FillColor = FLinearColor(0.7f, 0.35f, 1.0f, 1.0f);
        break;
    default:
        bShowBar = false;
        break;
    }

    ProgressAlpha = FMath::Clamp(ProgressAlpha, 0.0f, 1.0f);
    ProgressBarBackground->SetVisibility(false);
    ProgressBarFill->SetVisibility(false);
}

void ARoundObjectiveActor::RefreshProgressWidget()
{
    if (!ProgressWidgetComponent)
    {
        return;
    }

    float ProgressAlpha = 0.0f;
    bool bShowWidget = ObjectiveState == ERoundObjectiveState::Active;
    FString Label;

    switch (ObjectiveType)
    {
    case ERoundObjectiveType::HoldZone:
        ProgressAlpha = RequiredProgressInitiative > 0
            ? static_cast<float>(CurrentProgressInitiative) / static_cast<float>(RequiredProgressInitiative)
            : 0.0f;
        Label = FString::Printf(TEXT("%d/%d"), CurrentProgressInitiative, RequiredProgressInitiative);
        break;
    case ERoundObjectiveType::DestroyNode:
        ProgressAlpha = MaxHealth > 0
            ? static_cast<float>(CurrentHealth) / static_cast<float>(MaxHealth)
            : 0.0f;
        Label = FString::Printf(TEXT("%d/%d"), CurrentHealth, MaxHealth);
        break;
    case ERoundObjectiveType::Channel:
        ProgressAlpha = RequiredProgressInitiative > 0
            ? 1.0f - (static_cast<float>(ChannelRemainingInitiative) / static_cast<float>(RequiredProgressInitiative))
            : 0.0f;
        Label = FString::Printf(TEXT("%d/%d"), RequiredProgressInitiative - ChannelRemainingInitiative, RequiredProgressInitiative);
        break;
    default:
        bShowWidget = false;
        break;
    }

    ProgressAlpha = FMath::Clamp(ProgressAlpha, 0.0f, 1.0f);
    ProgressWidgetComponent->InitWidget();
    ProgressWidgetComponent->SetVisibility(bShowWidget);
    ProgressWidgetComponent->SetHiddenInGame(!bShowWidget);
    if (!bShowWidget)
    {
        return;
    }

    if (UUserWidget* Widget = ProgressWidgetComponent->GetUserWidgetObject())
    {
        if (UProgressBar* ProgressBar = Cast<UProgressBar>(Widget->GetWidgetFromName(TEXT("ProgressBar_Health"))))
        {
            ProgressBar->SetPercent(ProgressAlpha);
        }

        if (UTextBlock* NameText = Cast<UTextBlock>(Widget->GetWidgetFromName(TEXT("TextBlock_Name"))))
        {
            NameText->SetText(FText::FromString(Label));
        }
    }
}

bool ARoundObjectiveActor::DropReward(ARoundGameState* GameState)
{
    if (!GameState || !CenterTile || !GetWorld())
    {
        return false;
    }

    TSubclassOf<ARoundObjectiveRewardActor> RewardActorClass = GameState->ObjectiveRewardActorClass;
    if (!RewardActorClass)
    {
        RewardActorClass = ARoundObjectiveRewardActor::StaticClass();
    }
    ARoundObjectiveRewardActor* RewardActor = GetWorld()->SpawnActor<ARoundObjectiveRewardActor>(
        RewardActorClass,
        CenterTile->GetActorLocation(),
        FRotator::ZeroRotator);
    if (!RewardActor)
    {
        return false;
    }

    RewardActor->InitializeReward(CenterTile, RewardData);
    ObjectiveState = ERoundObjectiveState::RewardDropped;
    Destroy();
    return true;
}

void ARoundObjectiveActor::CancelChannel()
{
    ChannelingCharacter = nullptr;
    ChannelRemainingInitiative = 0;
    RefreshVisuals();
}

void ARoundObjectiveActor::ApplyRewardData(URoundObjectiveRewardData* InRewardData)
{
    RewardData = InRewardData;
    if (!InRewardData)
    {
        RewardDisplayName = TEXT("Reward");
        RewardDescription.Reset();
        RewardId = NAME_None;
        RewardType = ERoundObjectiveRewardType::Heal;
        RewardValue = 0.0;
        RewardDurationInitiative = 0;
        bRewardPermanent = false;
        return;
    }

    RewardId = InRewardData->RewardId;
    RewardDisplayName = InRewardData->DisplayName.IsEmpty()
        ? InRewardData->RewardId.ToString()
        : InRewardData->DisplayName.ToString();
    RewardDescription = InRewardData->Description.ToString();
    RewardType = InRewardData->RewardType;
    RewardValue = InRewardData->Value;
    RewardDurationInitiative = InRewardData->DurationInitiative;
    bRewardPermanent = InRewardData->bPermanent;
}
