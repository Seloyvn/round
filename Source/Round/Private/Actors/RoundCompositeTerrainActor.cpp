#include "Actors/RoundCompositeTerrainActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ARoundCompositeTerrainActor::ARoundCompositeTerrainActor()
{
    MeshComponent->SetVisibility(false, true);
    MeshComponent->SetHiddenInGame(true, true);
}

void ARoundCompositeTerrainActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RebuildVisualParts();
}

void ARoundCompositeTerrainActor::RebuildVisualParts()
{
    for (UStaticMeshComponent* Component : GeneratedVisualComponents)
    {
        if (IsValid(Component))
        {
            Component->DestroyComponent();
        }
    }
    GeneratedVisualComponents.Reset();

    for (int32 PartIndex = 0; PartIndex < VisualParts.Num(); ++PartIndex)
    {
        const FRoundTerrainVisualPart& Part = VisualParts[PartIndex];
        UStaticMesh* Mesh = Part.Mesh.LoadSynchronous();
        if (!Mesh)
        {
            continue;
        }

        const FName ComponentName(*FString::Printf(TEXT("TerrainVisual_%02d"), PartIndex));
        UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, ComponentName, RF_Transactional);
        Component->SetupAttachment(SceneRoot);
        Component->SetStaticMesh(Mesh);
        Component->SetRelativeLocation(Part.Location);
        Component->SetRelativeRotation(Part.Rotation);
        Component->SetRelativeScale3D(Part.Scale);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCastShadow(Part.bCastShadow);
        if (UMaterialInterface* Material = Part.Material.LoadSynchronous())
        {
            Component->SetMaterial(0, Material);
        }
        AddInstanceComponent(Component);
        Component->RegisterComponent();
        GeneratedVisualComponents.Add(Component);
    }
}
