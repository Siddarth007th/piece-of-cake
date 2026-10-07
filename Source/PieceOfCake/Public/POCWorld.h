#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "POCWorld.generated.h"

class APOCCharacter;
class APOCWorld;
class ACameraActor;
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UAudioComponent;
class ADirectionalLight;
class AExponentialHeightFog;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class EPOCProp : uint8 { Shard, Relic, Checkpoint, EchoNode, Bounce, Hazard, Cake, Moving, EchoBridge, Crumble, SlideGate };

USTRUCT()
struct FPOCRoutePoint
{
    GENERATED_BODY()
    FVector Position = FVector::ZeroVector;
    FVector Size = FVector(1260, 900, 200);
    float Yaw = 0;
    int32 Section = 0;
    int32 Index = 0;
    FString Kind;
    int32 Group = -1;
    int32 EchoNode = -1;
    int32 Enemy = -1;
    bool Checkpoint = false;
    bool Hazard = false;
    bool SlideGate = false;
    bool Bounce = false;
    bool Secret = false;
};

UCLASS(Blueprintable)
class PIECEOFCAKE_API APOCProp : public AActor
{
    GENERATED_BODY()
public:
    APOCProp();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void Configure(EPOCProp InKind, FVector InSize, int32 InId, int32 InGroup = -1);
    UFUNCTION(BlueprintCallable) void Activate(float Duration);
    UFUNCTION(BlueprintCallable) void ResetPlatform();
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EPOCProp Kind = EPOCProp::Shard;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Id = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Group = -1;
    UPROPERTY() TObjectPtr<APOCWorld> Journey;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY() TObjectPtr<USceneComponent> Visual;
    FVector Size = FVector(1260, 900, 200);
    FVector Origin;
    bool Taken = false;
    float ActiveRemaining = 0;
private:
    void Collect(APOCCharacter* Player);
    float Age = 0;
    float CollapseTime = -1;
    bool Touched = false;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Surface;
};

UCLASS(Blueprintable)
class PIECEOFCAKE_API APOCEnemy : public AActor
{
    GENERATED_BODY()
public:
    APOCEnemy();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintCallable) void Bonked(APOCCharacter* Player, bool Slam);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Species = 0;
    UPROPERTY() TObjectPtr<APOCWorld> Journey;
private:
    UPROPERTY() TObjectPtr<USceneComponent> Visual;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Shell;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> Surface;
    FVector Home;
    FVector Forward;
    float Age = 0;
    float Windup = -1;
    float Cooldown = 0;
    float DefeatTime = -1;
    float HitCooldown = 0;
    int32 Hits = 0;
};

UCLASS(Blueprintable)
class PIECEOFCAKE_API APOCWorld : public AActor
{
    GENERATED_BODY()
public:
    APOCWorld();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    UFUNCTION(BlueprintCallable) bool ActivateEcho(APOCCharacter* Player);
    UFUNCTION(BlueprintCallable) void ResetAtCheckpoint();
    UFUNCTION(BlueprintCallable) void Complete(APOCCharacter* Player, FVector CakePosition);
    UFUNCTION(BlueprintCallable) void ShowCaption(const FString& Text, float Duration = 4.f);
    void Sound(FName Name, FVector Position, float Pitch = 1.f);
    void Burst(FVector Position, FLinearColor Color, int32 Count = 10);
    const FPOCRoutePoint* Nearest(FVector Position) const;
    void TeleportSection(int32 Section);
    UPROPERTY(BlueprintReadOnly) FString SectionName;
    UPROPERTY(BlueprintReadOnly) FString Caption;
    UPROPERTY(BlueprintReadOnly) FString Prompt;
    UPROPERTY(BlueprintReadOnly) bool Ready = false;
    UPROPERTY(BlueprintReadOnly) FString LoadError;
    UPROPERTY() TObjectPtr<APOCCharacter> Player;
    UPROPERTY() TObjectPtr<ACameraActor> IntroCamera;
    UPROPERTY(EditAnywhere, Category="Journey") TSubclassOf<APOCProp> PropClass;
    UPROPERTY(EditAnywhere, Category="Journey") TSubclassOf<APOCEnemy> EnemyClass;
    TArray<FPOCRoutePoint> Route;
    int32 ActiveCheckpoint = -1;
private:
    bool LoadRoute();
    void BuildJourney();
    void AddScenery(const FPOCRoutePoint& Point);
    void AddInstance(const FString& Batch, const TCHAR* Shape, FVector Pos, FVector Scale, FRotator Rotation, FLinearColor Color, bool Collision = false, float Glow = 0.f);
    APOCProp* SpawnProp(EPOCProp Kind, FVector Pos, float Yaw, FVector Size, int32 Id, int32 Group = -1);
    void UpdateAtmosphere(float DeltaSeconds, int32 Section);
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Batches;
    TMap<FString, int32> BatchIds;
    UPROPERTY() TArray<TObjectPtr<APOCProp>> Props;
    UPROPERTY() TArray<TObjectPtr<APOCEnemy>> Enemies;
    UPROPERTY() TObjectPtr<ADirectionalLight> Sun;
    UPROPERTY() TObjectPtr<AExponentialHeightFog> Fog;
    UPROPERTY() TObjectPtr<UAudioComponent> Music;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Particles;
    TArray<FVector> ParticleVelocity;
    TArray<float> ParticleLife;
    TArray<FLinearColor> Palettes;
    float CaptionUntil = 0;
    float Age = 0;
    float SectionCheck = 0;
    int32 PreviousSection = -1;
};
