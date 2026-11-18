#include "Actors/RoundObjectiveRewardActor.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGridTile.h"
#include "Components/RoundAttributeSetComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Data/RoundObjectiveRewardData.h"
#include "Engine/StaticMesh.h"
#include "Effects/RoundObjectiveStatRewardEffect.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ARoundObjectiveRewardActor::ARoundObjectiveRewardActor()
{
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    TextRender = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRender"));
    TextRender->SetupAttachment(SceneRoot);
    TextRender->SetHorizontalAlignment(EHTA_Center);
    TextRender->SetWorldSize(48.0f);
    TextRender->SetTextRenderColor(FColor::Yellow);
    TextRender->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
    TextRender->SetHiddenInGame(true);

    MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
    MarkerMesh->SetupAttachment(SceneRoot);
    MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MarkerMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 35.0f));
    MarkerMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.22f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> RewardMeshAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (RewardMeshAsset.Succeeded())
    {
        MarkerMesh->SetStaticMesh(RewardMeshAsset.Object);
    }

    MarkerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MarkerLight"));
    MarkerLight->SetupAttachment(SceneRoot);
    MarkerLight->SetRelativeLocation(FVector(0.0f, 0.0f, 65.0f));
    MarkerLight->SetIntensity(1800.0f);
    MarkerLight->SetAttenuationRadius(140.0f);
    MarkerLight->SetLightColor(FLinearColor(0.1f, 1.0f, 0.4f));
}

void ARoundObjectiveRewardActor::PostNetReceive()
{
    Super::PostNetReceive();
    RefreshVisuals();
}

void ARoundObjectiveRewardActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundObjectiveRewardActor, RewardTile);
    DOREPLIFETIME(ARoundObjectiveRewardActor, RewardId);
    DOREPLIFETIME(ARoundObjectiveRewardActor, RewardDisplayName);
    DOREPLIFETIME(ARoundObjectiveRewardActor, RewardDescription);
    DOREPLIFETIME(ARoundObjectiveRewardActor, RewardType);
    DOREPLIFETIME(ARoundObjectiveRewardActor, RewardValue);
    DOREPLIFETIME(ARoundObjectiveRewardActor, RewardDurationInitiative);
    DOREPLIFETIME(ARoundObjectiveRewardActor, bRewardPermanent);
}

void ARoundObjectiveRewardActor::InitializeReward(ARoundGridTile* InTile, URoundObjectiveRewardData* InRewardData)
{
    RewardTile = InTile;
    RewardData = InRewardData;
    RewardId = InRewardData ? InRewardData->RewardId : NAME_None;
    RewardDisplayName = InRewardData && !InRewardData->DisplayName.IsEmpty()
        ? InRewardData->DisplayName.ToString()
        : (InRewardData ? InRewardData->RewardId.ToString() : TEXT("Reward"));
    RewardDescription = InRewardData ? InRewardData->Description.ToString() : FString();
    RewardType = InRewardData ? InRewardData->RewardType : ERoundObjectiveRewardType::Heal;
    RewardValue = InRewardData ? InRewardData->Value : 0.0;
    RewardDurationInitiative = InRewardData ? InRewardData->DurationInitiative : 0;
    bRewardPermanent = InRewardData && InRewardData->bPermanent;

    if (RewardTile)
    {
        SetActorLocation(RewardTile->GetActorLocation());
        InitializeTrigger(nullptr, {RewardTile});
    }

    RefreshVisuals();
}

bool ARoundObjectiveRewardActor::CanTriggerCharacter_Implementation(ARoundCharacter* EnteringCharacter, ARoundGridTile* EnteredTile, ARoundGridTile* PreviousTile) const
{
    return RewardTile && EnteringCharacter && EnteredTile == RewardTile;
}

void ARoundObjectiveRewardActor::ExecuteTrigger_Implementation(ARoundCharacter* EnteringCharacter, ARoundGridTile* EnteredTile, ARoundGridTile* PreviousTile)
{
    // Reward gameplay is sim-authoritative. The runtime actor mirrors visuals only.
}

void ARoundObjectiveRewardActor::RefreshVisuals()
{
    if (!TextRender)
    {
        return;
    }

    const FString Label = !RewardDisplayName.IsEmpty() ? RewardDisplayName : TEXT("Reward");
    TextRender->SetText(FText::FromString(Label));
}

bool ARoundObjectiveRewardActor::GrantReward(ARoundCharacter* Character)
{
    if (!Character || !Character->Attributes)
    {
        return false;
    }

    switch (RewardType)
    {
    case ERoundObjectiveRewardType::Heal:
        Character->Attributes->Heal(FMath::Max(1, FMath::RoundToInt(RewardValue)));
        break;
    case ERoundObjectiveRewardType::AttackAdd:
    case ERoundObjectiveRewardType::DefenseAdd:
    case ERoundObjectiveRewardType::MaxHealthAdd:
    case ERoundObjectiveRewardType::MaxInitiativeAdd:
        if (ARoundObjectiveStatRewardEffect* Effect = GetWorld() ? GetWorld()->SpawnActor<ARoundObjectiveStatRewardEffect>(ARoundObjectiveStatRewardEffect::StaticClass()) : nullptr)
        {
            Effect->EffectId = RewardData && !RewardData->RewardId.IsNone() ? RewardData->RewardId : TEXT("ObjectiveReward");
            Effect->DisplayName = RewardDisplayName.IsEmpty() ? TEXT("Objective Reward") : RewardDisplayName;
            Effect->Description = RewardDescription;
            Effect->SetDurationInitiative(bRewardPermanent ? 0 : FMath::Max(0, RewardDurationInitiative), !bRewardPermanent);

            const int32 RoundedValue = FMath::Max(1, FMath::RoundToInt(RewardValue));
            switch (RewardType)
            {
            case ERoundObjectiveRewardType::AttackAdd:
                Effect->AttackAdd = RoundedValue;
                break;
            case ERoundObjectiveRewardType::DefenseAdd:
                Effect->DefenseAdd = RoundedValue;
                break;
            case ERoundObjectiveRewardType::MaxHealthAdd:
                Effect->MaxHealthAdd = RoundedValue;
                break;
            case ERoundObjectiveRewardType::MaxInitiativeAdd:
                Effect->MaxInitiativeAdd = RoundedValue;
                break;
            default:
                break;
            }

            Character->AddEffect(Effect);
        }
        break;
    default:
        return false;
    }

    RemoveTrigger();
    return true;
}
