#pragma once
#include "CoreMinimal.h"
class UStaticMesh;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class USceneComponent;
class AActor;

namespace POCVisuals
{
    UStaticMesh* Shape(const TCHAR* Name);
    UMaterialInstanceDynamic* Material(UObject* Owner, FLinearColor Color, float Glow = 0.f);
    UStaticMeshComponent* Part(AActor* Owner, USceneComponent* Parent, FName Name,
        const TCHAR* ShapeName, FVector Position, FVector Scale, FLinearColor Color, float Glow = 0.f);
}
