#include "POCVisuals.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"

UStaticMesh* POCVisuals::Shape(const TCHAR* Name)
{
    if (FCString::Strcmp(Name, TEXT("Sphere")) && FCString::Strcmp(Name, TEXT("Cube"))
        && FCString::Strcmp(Name, TEXT("Cone")) && FCString::Strcmp(Name, TEXT("Cylinder")))
    {
        const FString Path = FString::Printf(TEXT("/Game/Art/Meshes/SM_%s.SM_%s"), Name, Name);
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!Mesh) UE_LOG(LogTemp, Error, TEXT("Required game mesh missing: %s. Run asset bootstrap."), *Path);
        return Mesh;
    }
    return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name));
}

UMaterialInstanceDynamic* POCVisuals::Material(UObject* Owner, FLinearColor Color, float Glow)
{
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_World.M_World"));
    if (!Base) Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    UMaterialInstanceDynamic* Result = UMaterialInstanceDynamic::Create(Base, Owner);
    Result->SetVectorParameterValue(TEXT("Tint"), Color);
    // A restrained ambient tint keeps silhouettes readable in the stylized shade.
    Result->SetScalarParameterValue(TEXT("Glow"), FMath::Max(Glow, .08f));
    return Result;
}

UStaticMeshComponent* POCVisuals::Part(AActor* Owner, USceneComponent* Parent, FName Name,
    const TCHAR* ShapeName, FVector Position, FVector Scale, FLinearColor Color, float Glow)
{
    UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner, Name);
    Owner->AddInstanceComponent(Mesh);
    Mesh->SetupAttachment(Parent);
    Mesh->SetStaticMesh(Shape(ShapeName));
    auto* Surface = Material(Owner, Color, Glow);
    for (int32 Slot = 0; Slot < FMath::Max(1, Mesh->GetNumMaterials()); ++Slot) Mesh->SetMaterial(Slot, Surface);
    Mesh->SetRelativeLocation(Position);
    Mesh->SetRelativeScale3D(Scale);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCullDistance(14000);
    Mesh->SetCastShadow(false);
    Mesh->RegisterComponent();
    return Mesh;
}
