#include "Actors/RoundGridTile.h"

#include "Actors/RoundCharacter.h"
#include "Actors/RoundGroveTerrainActor.h"
#include "Interfaces/RoundDeviceInterface.h"
#include "Actors/RoundFieldObjectActor.h"
#include "Actors/RoundFieldTriggerActor.h"
#include "Actors/RoundThornsTerrainActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Actors/RoundTerrainActor.h"
#include "Data/RoundTerrainData.h"
#include "Engine/Engine.h"
#include "Game/RoundGameState.h"
#include "Game/RoundPlayerPawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

namespace
{
UMaterialInterface* GDefaultVisibleMaterial = nullptr;
UMaterialInterface* GDefaultHiddenMaterial = nullptr;
UMaterialInterface* GHoveredMaterial = nullptr;
UMaterialInterface* GRangeMaterial = nullptr;
UMaterialInterface* GOkMaterial = nullptr;
UMaterialInterface* GNotOkMaterial = nullptr;
UMaterialInterface* GSelectedMaterial = nullptr;
}

ARoundGridTile::ARoundGridTile()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);

    TileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileMesh"));
    SetRootComponent(TileMesh);
    TileMesh->SetMobility(EComponentMobility::Static);
    TileMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TileMesh->SetCollisionObjectType(ECC_WorldDynamic);
    TileMesh->SetCollisionResponseToAllChannels(ECR_Block);
    TileMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    TileMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    TileMesh->SetGenerateOverlapEvents(false);

    DebugTerrainLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DebugTerrainLabel"));
    DebugTerrainLabel->SetupAttachment(RootComponent);
    DebugTerrainLabel->SetRelativeLocation(FVector(0.0, 0.0, 85.0));
    DebugTerrainLabel->SetHorizontalAlignment(EHTA_Center);
    DebugTerrainLabel->SetWorldSize(28.0f);
    DebugTerrainLabel->SetHiddenInGame(true);

    DebugRegionOverlay = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugRegionOverlay"));
    DebugRegionOverlay->SetupAttachment(RootComponent);
    DebugRegionOverlay->SetRelativeLocation(FVector(0.0, 0.0, 4.0));
    DebugRegionOverlay->SetRelativeScale3D(FVector(0.94f, 0.94f, 0.94f));
    DebugRegionOverlay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DebugRegionOverlay->SetCastShadow(false);
    DebugRegionOverlay->SetHiddenInGame(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> HexMeshFinder(TEXT("/Game/TopDown/Field/hexagon.hexagon"));
    if (HexMeshFinder.Succeeded())
    {
        TileMesh->SetStaticMesh(HexMeshFinder.Object);
        DebugRegionOverlay->SetStaticMesh(HexMeshFinder.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMaterialFinder(TEXT("/Game/TopDown/Field/Mat_Hexagon_Default.Mat_Hexagon_Default"));
    if (DefaultMaterialFinder.Succeeded())
    {
        GDefaultVisibleMaterial = DefaultMaterialFinder.Object;
        TileMesh->SetMaterial(0, DefaultMaterialFinder.Object);
    }
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultNoVisionMaterialFinder(TEXT("/Game/TopDown/Field/Mat_Hexagon_DefauntNoVision.Mat_Hexagon_DefauntNoVision"));
    if (DefaultNoVisionMaterialFinder.Succeeded())
    {
        GDefaultHiddenMaterial = DefaultNoVisionMaterialFinder.Object;
    }
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> HoveredMaterialFinder(TEXT("/Game/TopDown/Field/Mat_Hexagon_Highlighted.Mat_Hexagon_Highlighted"));
    if (HoveredMaterialFinder.Succeeded())
    {
        GHoveredMaterial = HoveredMaterialFinder.Object;
    }
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> RangeMaterialFinder(TEXT("/Game/TopDown/Field/Mat_Hexagon_Range.Mat_Hexagon_Range"));
    if (RangeMaterialFinder.Succeeded())
    {
        GRangeMaterial = RangeMaterialFinder.Object;
    }
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> OkMaterialFinder(TEXT("/Game/TopDown/Field/Mat_Hexagon_OK.Mat_Hexagon_OK"));
    if (OkMaterialFinder.Succeeded())
    {
        GOkMaterial = OkMaterialFinder.Object;
    }
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> NotOkMaterialFinder(TEXT("/Game/TopDown/Field/Mat_Hexagon_NotOK.Mat_Hexagon_NotOK"));
    if (NotOkMaterialFinder.Succeeded())
    {
        GNotOkMaterial = NotOkMaterialFinder.Object;
    }
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> SelectedMaterialFinder(TEXT("/Game/TopDown/Field/Mat_Hexagon_Selected.Mat_Hexagon_Selected"));
    if (SelectedMaterialFinder.Succeeded())
    {
        GSelectedMaterial = SelectedMaterialFinder.Object;
    }

    static ConstructorHelpers::FClassFinder<AActor> GroundClassFinder(TEXT("/Game/TopDown/Field/Terrain/BP_Ground"));
    static ConstructorHelpers::FClassFinder<AActor> ForestClassFinder(TEXT("/Game/TopDown/Field/Terrain/BP_Forest"));
    static ConstructorHelpers::FClassFinder<AActor> ThornsClassFinder(TEXT("/Game/TopDown/Field/Terrain/BP_Thorns"));
    static ConstructorHelpers::FClassFinder<AActor> GroveClassFinder(TEXT("/Game/TopDown/Field/Terrain/BP_Grove"));
    static ConstructorHelpers::FClassFinder<AActor> StreetClassFinder(TEXT("/Game/TopDown/Field/Terrain/BP_Street"));
    static ConstructorHelpers::FClassFinder<AActor> FireClassFinder(TEXT("/Game/TopDown/Field/Terrain/BP_Fire"));
    static ConstructorHelpers::FClassFinder<AActor> RocksClassFinder(TEXT("/Game/TopDown/Field/Terrain/BP_Rocks"));
    static ConstructorHelpers::FClassFinder<AActor> RubbleClassFinder(TEXT("/Game/TopDown/Field/Terrain/BP_Rubble"));

    if (GroundClassFinder.Succeeded()) { TerrainVisualClasses.Add(ERoundTerrainType::Ground, GroundClassFinder.Class); }
    if (ForestClassFinder.Succeeded()) { TerrainVisualClasses.Add(ERoundTerrainType::Forest, ForestClassFinder.Class); }
    const TSubclassOf<AActor> ThornsVisualClass = ThornsClassFinder.Succeeded()
        ? TSubclassOf<AActor>(ThornsClassFinder.Class)
        : TSubclassOf<AActor>(ARoundThornsTerrainActor::StaticClass());
    const TSubclassOf<AActor> GroveVisualClass = GroveClassFinder.Succeeded()
        ? TSubclassOf<AActor>(GroveClassFinder.Class)
        : TSubclassOf<AActor>(ARoundGroveTerrainActor::StaticClass());
    TerrainVisualClasses.Add(ERoundTerrainType::Thorns, ThornsVisualClass);
    TerrainVisualClasses.Add(ERoundTerrainType::Grove, GroveVisualClass);
    if (StreetClassFinder.Succeeded()) { TerrainVisualClasses.Add(ERoundTerrainType::Street, StreetClassFinder.Class); }
    if (FireClassFinder.Succeeded()) { TerrainVisualClasses.Add(ERoundTerrainType::Fire, FireClassFinder.Class); }
    if (RocksClassFinder.Succeeded()) { TerrainVisualClasses.Add(ERoundTerrainType::Rocks, RocksClassFinder.Class); }
    if (RubbleClassFinder.Succeeded()) { TerrainVisualClasses.Add(ERoundTerrainType::Rubble, RubbleClassFinder.Class); }

    TileMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
}

void ARoundGridTile::SetDebugTerrainLabelVisible(const bool bVisible)
{
    if (!DebugTerrainLabel) return;
    DebugTerrainLabel->SetText(FText::FromString(FString::Printf(TEXT("%s\n%d"), *UEnum::GetValueAsString(TerrainType), GetTerrainMoveCost())));
    DebugTerrainLabel->SetHiddenInGame(!bVisible);
}

void ARoundGridTile::SetDebugRegionOverlay(const bool bVisible, const FLinearColor Color, UMaterialInterface* OverlayMaterial)
{
    if (!DebugRegionOverlay) return;
    if (!bVisible || !OverlayMaterial)
    {
        DebugRegionOverlay->SetHiddenInGame(true);
        return;
    }
    if (!DebugRegionMaterial)
    {
        DebugRegionMaterial = UMaterialInstanceDynamic::Create(OverlayMaterial, this);
        DebugRegionOverlay->SetMaterial(0, DebugRegionMaterial);
    }
    DebugRegionMaterial->SetVectorParameterValue(TEXT("RegionColor"), Color);
    DebugRegionOverlay->SetHiddenInGame(false);
}

void ARoundGridTile::BeginPlay()
{
    Super::BeginPlay();

    if (TileMesh)
    {
        TileMesh->OnClicked.AddDynamic(this, &ARoundGridTile::HandleTileClicked);
        TileMesh->OnBeginCursorOver.AddDynamic(this, &ARoundGridTile::HandleTileBeginCursorOver);
        TileMesh->OnEndCursorOver.AddDynamic(this, &ARoundGridTile::HandleTileEndCursorOver);
    }

    ApplyCurrentMaterial();
}

void ARoundGridTile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ARoundGridTile, GridPosition);
    DOREPLIFETIME(ARoundGridTile, GridCoordinate);
    DOREPLIFETIME(ARoundGridTile, TerrainType);
    DOREPLIFETIME(ARoundGridTile, MoveCost);
    DOREPLIFETIME(ARoundGridTile, bBlocksVision);
    DOREPLIFETIME(ARoundGridTile, TerrainRegionId);
    DOREPLIFETIME(ARoundGridTile, TerrainRegionIndex);
    DOREPLIFETIME(ARoundGridTile, TerrainObject);
    DOREPLIFETIME(ARoundGridTile, OccupyingCharacter);
    DOREPLIFETIME(ARoundGridTile, OccupyingFieldObject);
}

bool ARoundGridTile::IsWalkable() const
{
    const ARoundTerrainActor* TerrainActor = Cast<ARoundTerrainActor>(TerrainObject);
    return OccupyingCharacter == nullptr && !HasBlockingFieldObject() && (!TerrainActor || TerrainActor->IsMoveable());
}

ARoundCharacter* ARoundGridTile::GetResolvedOccupyingCharacter() const
{
    if (OccupyingCharacter)
    {
        return OccupyingCharacter;
    }

    if (const FObjectProperty* CharacterProperty = FindFProperty<FObjectProperty>(GetClass(), TEXT("Character")))
    {
        return Cast<ARoundCharacter>(CharacterProperty->GetObjectPropertyValue_InContainer(this));
    }

    return nullptr;
}

ARoundFieldObjectActor* ARoundGridTile::GetResolvedOccupyingFieldObject() const
{
    if (OccupyingFieldObject)
    {
        return OccupyingFieldObject;
    }

    return nullptr;
}

bool ARoundGridTile::HasBlockingFieldObject() const
{
    ARoundFieldObjectActor* FieldObject = GetResolvedOccupyingFieldObject();
    return FieldObject
        && FieldObject->GetClass()->ImplementsInterface(URoundDeviceInterface::StaticClass())
        && IRoundDeviceInterface::Execute_BlocksCharactersOnTile(FieldObject);
}

bool ARoundGridTile::IsMoveable() const
{
    const ARoundTerrainActor* TerrainActor = Cast<ARoundTerrainActor>(TerrainObject);
    return !TerrainActor || TerrainActor->IsMoveable();
}

int32 ARoundGridTile::GetTerrainMoveCost(const int32 BaseCost) const
{
    if (const ARoundTerrainActor* TerrainActor = Cast<ARoundTerrainActor>(TerrainObject))
    {
        return TerrainActor->GetMoveCost(BaseCost);
    }

    return MoveCost;
}

int32 ARoundGridTile::GetTerrainVisionCost() const
{
    if (const ARoundTerrainActor* TerrainActor = Cast<ARoundTerrainActor>(TerrainObject))
    {
        return TerrainActor->GetVisionCost();
    }
    return 1;
}

int32 ARoundGridTile::GetTerrainVisionRangeModifier() const
{
    if (const ARoundTerrainActor* TerrainActor = Cast<ARoundTerrainActor>(TerrainObject))
    {
        return TerrainActor->GetVisionRangeModifier();
    }
    return 0;
}

bool ARoundGridTile::TerrainStopsMovement() const
{
    if (const ARoundTerrainActor* TerrainActor = Cast<ARoundTerrainActor>(TerrainObject))
    {
        return TerrainActor->StopsMovement();
    }
    return false;
}

bool ARoundGridTile::MatchesGridPosition(const FVector2D& InPosition) const
{
    return GridPosition.Equals(InPosition);
}

int32 ARoundGridTile::GetHexDistanceTo(const ARoundGridTile* OtherTile) const
{
    if (!OtherTile)
    {
        return MAX_int32;
    }

    const int32 Q1 = FMath::RoundToInt(GridPosition.X);
    const int32 R1 = FMath::RoundToInt(GridPosition.Y);
    const int32 Q2 = FMath::RoundToInt(OtherTile->GridPosition.X);
    const int32 R2 = FMath::RoundToInt(OtherTile->GridPosition.Y);

    const int32 DQ = Q1 - Q2;
    const int32 DR = R1 - R2;
    return (FMath::Abs(DQ) + FMath::Abs(DR) + FMath::Abs(DQ + DR)) / 2;
}

int32 ARoundGridTile::GetDistance(const ARoundGridTile* OtherTile) const
{
    if (!OtherTile)
    {
        return MAX_int32;
    }

    const float DeltaX = OtherTile->GridPosition.X - GridPosition.X;
    const float DeltaY = OtherTile->GridPosition.Y - GridPosition.Y;
    const float DeltaZ = DeltaX + DeltaY;

    const float MaxAxisDistance = FMath::Max3(FMath::Abs(DeltaX), FMath::Abs(DeltaY), FMath::Abs(DeltaZ));
    return FMath::TruncToInt(MaxAxisDistance);
}

TArray<ARoundGridTile*> ARoundGridTile::GetDistanceFields(const int32 Distance) const
{
    TArray<ARoundGridTile*> Result;

    const ARoundGameState* RoundGameState = GetWorld() ? GetWorld()->GetGameState<ARoundGameState>() : nullptr;
    if (!RoundGameState)
    {
        return Result;
    }

    for (ARoundGridTile* Field : RoundGameState->GetFields())
    {
        if (Field && GetDistance(Field) <= Distance)
        {
            Result.AddUnique(Field);
        }
    }

    return Result;
}

void ARoundGridTile::SetTerrainType(const ERoundTerrainType NewTerrainType)
{
    TerrainType = NewTerrainType;
    MoveCost = 1;
    bBlocksVision = false;

    if (TerrainObject)
    {
        TerrainObject->Destroy();
        TerrainObject = nullptr;
    }

    const ARoundGameState* RoundGameState = GetWorld() ? GetWorld()->GetGameState<ARoundGameState>() : nullptr;
    const URoundTerrainData* TerrainData = RoundGameState ? RoundGameState->GetTerrainData(NewTerrainType) : nullptr;

    TSubclassOf<AActor> TerrainActorClass = TerrainData ? TerrainData->TerrainActorClass : nullptr;
    if (!TerrainActorClass)
    {
        if (const TSubclassOf<AActor>* VisualClass = TerrainVisualClasses.Find(NewTerrainType))
        {
            TerrainActorClass = *VisualClass;
        }
    }

    if (TerrainActorClass)
    {
        if (GetWorld())
        {
            FActorSpawnParameters SpawnParameters;
            SpawnParameters.Owner = this;
            SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            TerrainObject = GetWorld()->SpawnActor<AActor>(TerrainActorClass, GetActorLocation(), GetActorRotation(), SpawnParameters);
            if (ARoundTerrainActor* TerrainActor = Cast<ARoundTerrainActor>(TerrainObject))
            {
                TerrainActor->TerrainType = NewTerrainType;
                TerrainActor->SetTerrainData(const_cast<URoundTerrainData*>(TerrainData));
                TerrainActor->SetField(this);
                MoveCost = TerrainActor->GetTerrainBaseMoveCost();
                bBlocksVision = TerrainActor->BlocksVision();
            }
        }
    }

    ApplyCurrentMaterial();
}

void ARoundGridTile::SetHoverState(const ERoundHoverFieldState NewState)
{
    HoverState = NewState;
    ApplyCurrentMaterial();
}

void ARoundGridTile::SetSelectedState(const bool bInSelected)
{
    bSelected = bInSelected;
    if (bSelected)
    {
        HoverState = ERoundHoverFieldState::Selected;
    }
    else if (HoverState == ERoundHoverFieldState::Selected)
    {
        HoverState = ERoundHoverFieldState::None;
    }

    ApplyCurrentMaterial();
}

void ARoundGridTile::SetHasVision(const bool bInHasVision)
{
    bHasVision = bInHasVision;
    ApplyCurrentMaterial();
}

void ARoundGridTile::AddFieldTrigger(ARoundFieldTriggerActor* TriggerActor)
{
    if (!TriggerActor)
    {
        return;
    }

    FieldTriggers.AddUnique(TriggerActor);
}

void ARoundGridTile::RemoveFieldTrigger(ARoundFieldTriggerActor* TriggerActor)
{
    if (!TriggerActor)
    {
        return;
    }

    FieldTriggers.RemoveSingleSwap(TriggerActor);
}

void ARoundGridTile::NotifyCharacterEntered(ARoundCharacter* EnteringCharacter, ARoundGridTile* PreviousTile)
{
    if (!HasAuthority() || !EnteringCharacter)
    {
        return;
    }

    if (PreviousTile)
    {
        if (ARoundFieldObjectActor* PreviousFieldObject = PreviousTile->GetResolvedOccupyingFieldObject())
        {
            if (PreviousFieldObject->GetClass()->ImplementsInterface(URoundDeviceInterface::StaticClass()))
            {
                FRoundDeviceTriggerContext TriggerContext;
                TriggerContext.TriggerType = ERoundDeviceTriggerType::OnLeave;
                TriggerContext.TriggeringCharacter = EnteringCharacter;
                TriggerContext.TriggerTile = PreviousTile;

                if (IRoundDeviceInterface::Execute_CanTriggerDevice(PreviousFieldObject, TriggerContext))
                {
                    IRoundDeviceInterface::Execute_TriggerDevice(PreviousFieldObject, TriggerContext);
                }
            }
        }
    }

    if (ARoundFieldObjectActor* CurrentFieldObject = GetResolvedOccupyingFieldObject())
    {
        if (CurrentFieldObject->GetClass()->ImplementsInterface(URoundDeviceInterface::StaticClass()))
        {
            FRoundDeviceTriggerContext TriggerContext;
            TriggerContext.TriggerType = ERoundDeviceTriggerType::OnEnter;
            TriggerContext.TriggeringCharacter = EnteringCharacter;
            TriggerContext.TriggerTile = this;

            if (IRoundDeviceInterface::Execute_CanTriggerDevice(CurrentFieldObject, TriggerContext))
            {
                IRoundDeviceInterface::Execute_TriggerDevice(CurrentFieldObject, TriggerContext);
            }
        }
    }

    if (ARoundTerrainActor* TerrainActor = Cast<ARoundTerrainActor>(TerrainObject))
    {
        TerrainActor->OnCharacterEntered(EnteringCharacter, PreviousTile);
    }

    const TArray<TObjectPtr<ARoundFieldTriggerActor>> TriggersSnapshot = FieldTriggers;
    for (ARoundFieldTriggerActor* TriggerActor : TriggersSnapshot)
    {
        if (TriggerActor)
        {
            TriggerActor->NotifyCharacterEntered(EnteringCharacter, this, PreviousTile);
        }
    }
}

void ARoundGridTile::OnHover()
{
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    ARoundPlayerPawn* PlayerPawn = PlayerController ? Cast<ARoundPlayerPawn>(PlayerController->GetPawn()) : nullptr;
    if (!PlayerPawn)
    {
        SetHoverState(bSelected ? ERoundHoverFieldState::Selected : ERoundHoverFieldState::Hovered);
        return;
    }

    ARoundCharacter* SelectedCharacter = PlayerPawn->SelectedCharacter();
    if (!SelectedCharacter)
    {
        SetHoverState(bSelected ? ERoundHoverFieldState::Selected : ERoundHoverFieldState::Hovered);
        return;
    }

    switch (PlayerPawn->SelectedAction)
    {
    case ERoundActionType::Move:
        SetHoverState(SelectedCharacter->CanMoveTo(this) ? ERoundHoverFieldState::Ok : ERoundHoverFieldState::NotOk);
        break;
    case ERoundActionType::Attack:
    {
        const bool bInRange = SelectedCharacter->CurrentTile && SelectedCharacter->CurrentTile->GetDistance(this) <= SelectedCharacter->GetAttackRange();
        const bool bCanAttack = bInRange && SelectedCharacter->EnemyField(this);
        SetHoverState(bCanAttack ? ERoundHoverFieldState::Ok : ERoundHoverFieldState::NotOk);
        break;
    }
    case ERoundActionType::Ability:
        SetHoverState(ERoundHoverFieldState::Hovered);
        break;
    default:
        SetHoverState(bSelected ? ERoundHoverFieldState::Selected : ERoundHoverFieldState::Hovered);
        break;
    }
}

void ARoundGridTile::OnEndHover()
{
    if (bSelected)
    {
        SetHoverState(ERoundHoverFieldState::Selected);
    }
    else
    {
        SetHoverState(ERoundHoverFieldState::None);
    }
}

void ARoundGridTile::SetDefaultColor()
{
    if (!bSelected)
    {
        HoverState = ERoundHoverFieldState::None;
    }

    ApplyCurrentMaterial();
}

void ARoundGridTile::ApplyCurrentMaterial()
{
    if (!TileMesh)
    {
        return;
    }

    UMaterialInterface* MaterialToApply = nullptr;
    switch (HoverState)
    {
    case ERoundHoverFieldState::Hovered:
        MaterialToApply = GHoveredMaterial;
        break;
    case ERoundHoverFieldState::Range:
        MaterialToApply = GRangeMaterial ? GRangeMaterial : GHoveredMaterial;
        break;
    case ERoundHoverFieldState::Ok:
        MaterialToApply = GOkMaterial;
        break;
    case ERoundHoverFieldState::NotOk:
        MaterialToApply = GNotOkMaterial;
        break;
    case ERoundHoverFieldState::Selected:
        MaterialToApply = GSelectedMaterial;
        break;
    case ERoundHoverFieldState::None:
    default:
        MaterialToApply = bHasVision ? GDefaultVisibleMaterial : GDefaultHiddenMaterial;
        break;
    }

    if (MaterialToApply)
    {
        TileMesh->SetMaterial(0, MaterialToApply);
    }
}

void ARoundGridTile::HandleTileClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
    if (ButtonPressed != EKeys::LeftMouseButton)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("RoundGridTile Clicked - %s Pos=%s Button=%s"),
        *GetName(),
        *GridPosition.ToString(),
        *ButtonPressed.ToString());

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("RoundGridTile Clicked - no local PlayerController for %s"), *GetName());
        return;
    }

    if (ARoundPlayerPawn* PlayerPawn = Cast<ARoundPlayerPawn>(PlayerController->GetPawn()))
    {
        UE_LOG(LogTemp, Log, TEXT("RoundGridTile Clicked - forwarding to PlayerPawn %s"), *GetNameSafe(PlayerPawn));
        PlayerPawn->Klick(this);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("RoundGridTile Clicked - local controller has no ARoundPlayerPawn"));
    }
}

void ARoundGridTile::HandleTileBeginCursorOver(UPrimitiveComponent* TouchedComponent)
{
    OnHover();

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!PlayerController)
    {
        return;
    }

    if (ARoundPlayerPawn* PlayerPawn = Cast<ARoundPlayerPawn>(PlayerController->GetPawn()))
    {
        PlayerPawn->HoverField(this);
    }
}

void ARoundGridTile::HandleTileEndCursorOver(UPrimitiveComponent* TouchedComponent)
{
    OnEndHover();

    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!PlayerController)
    {
        return;
    }

    if (ARoundPlayerPawn* PlayerPawn = Cast<ARoundPlayerPawn>(PlayerController->GetPawn()))
    {
        PlayerPawn->EndHoverField(this);
    }
}
