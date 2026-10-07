#include "POCVisuals.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Actor.h"

UStaticMesh* POCVisuals::Shape(const TCHAR* Name)
{
    return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"), Name, Name));
}

UMaterialInstanceDynamic* POCVisuals::Material(UObject* Owner, FLinearColor Color, float Glow)
{
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_World.M_World"));
    if (!Base) Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    UMaterialInstanceDynamic* Result = UMaterialInstanceDynamic::Create(Base, Owner);
    Result->SetVectorParameterValue(TEXT("Tint"), Color);
    Result->SetScalarParameterValue(TEXT("Glow"), Glow);
    return Result;
}

UStaticMeshComponent* POCVisuals::Part(AActor* Owner, USceneComponent* Parent, FName Name,
    const TCHAR* ShapeName, FVector Position, FVector Scale, FLinearColor Color, float Glow)
{
    UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Owner, Name);
    Owner->AddInstanceComponent(Mesh);
    Mesh->SetupAttachment(Parent);
    Mesh->SetStaticMesh(Shape(ShapeName));
    Mesh->SetMaterial(0, Material(Owner, Color, Glow));
    Mesh->SetRelativeLocation(Position);
    Mesh->SetRelativeScale3D(Scale);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCullDistance(14000);
    Mesh->SetCastShadow(Glow < 1.f);
    Mesh->RegisterComponent();
    return Mesh;
}
