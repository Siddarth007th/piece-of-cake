#include "POCWorld.h"
#include "POCCharacter.h"
#include "POCGameInstance.h"
#include "POCVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

APOCProp::APOCProp()
{
    PrimaryActorTick.bCanEverTick = true;
    Visual = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Visual);
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Visual);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetMobility(EComponentMobility::Movable);
}

void APOCProp::Configure(EPOCProp InKind, FVector InSize, int32 InId, int32 InGroup)
{
    Kind = InKind; Size = InSize; Id = InId; Group = InGroup;
}

void APOCProp::BeginPlay()
{
    Super::BeginPlay();
    Origin = GetActorLocation();
    Mesh->SetCullDistance(14000);
    const bool Platform = Kind == EPOCProp::Moving || Kind == EPOCProp::EchoBridge || Kind == EPOCProp::Crumble;
    Mesh->SetStaticMesh(POCVisuals::Shape(Platform || Kind == EPOCProp::Hazard || Kind == EPOCProp::SlideGate ? TEXT("Cube") : TEXT("Sphere")));
    FLinearColor Color(.24, .8, .84);
    float Glow = .5f;
    if (Platform)
    {
        Color = Kind == EPOCProp::EchoBridge ? FLinearColor(.12, .58, .72) : FLinearColor(.15, .23, .3);
        Mesh->SetRelativeScale3D(Size / 100);
        Mesh->SetRelativeLocation(FVector(0, 0, -Size.Z * .5));
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        Glow = Kind == EPOCProp::EchoBridge ? .55f : 0.f;
        POCVisuals::Part(this, Visual, TEXT("Inlay"), TEXT("Cube"), FVector(0, 0, 2), FVector(Size.X * .0085, .035, .025), FLinearColor(.25, .9, .9), .7);
        if (Kind == EPOCProp::EchoBridge) { Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetVisibility(false); }
    }
    else if (Kind == EPOCProp::SlideGate)
    {
        Color = FLinearColor(.2, .26, .33); Glow = 0;
        Mesh->SetRelativeScale3D(FVector(1.3, 9, 2.6));
        Mesh->SetRelativeLocation(FVector(0, 0, 195));
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        for (int32 Side : {-1, 1})
            POCVisuals::Part(this, Visual, *FString::Printf(TEXT("Gate%d"), Side), TEXT("Cube"), FVector(0, Side * 410, 60), FVector(1.3, .8, 1.2), Color);
    }
    else if (Kind == EPOCProp::Shard || Kind == EPOCProp::Relic)
    {
        const bool Relic = Kind == EPOCProp::Relic;
        Mesh->SetStaticMesh(POCVisuals::Shape(TEXT("Cube")));
        Mesh->SetRelativeScale3D(Relic ? FVector(.3, .3, .3) : FVector(.15, .15, .25));
        Mesh->SetRelativeRotation(FRotator(45, 0, 45));
        Color = Relic ? FLinearColor(1, .57, .12) : Color;
        Taken = GetGameInstance<UPOCGameInstance>()->Collected.Contains(Id);
        SetActorHiddenInGame(Taken);
    }
    else if (Kind == EPOCProp::Checkpoint)
    {
        Mesh->SetStaticMesh(POCVisuals::Shape(TEXT("Cylinder")));
        Mesh->SetRelativeScale3D(FVector(.65, .65, .12));
        Mesh->SetRelativeLocation(FVector(0, 0, 6));
        Color = FLinearColor(.95, .6, .24);
        POCVisuals::Part(this, Visual, TEXT("Lantern"), TEXT("Sphere"), FVector(0, 0, 70), FVector(.2), Color, 2);
    }
    else if (Kind == EPOCProp::EchoNode)
    {
        Mesh->SetStaticMesh(POCVisuals::Shape(TEXT("Cone")));
        Mesh->SetRelativeScale3D(FVector(.75, .75, 1.2));
        Mesh->SetRelativeLocation(FVector(0, 0, 60));
        POCVisuals::Part(this, Visual, TEXT("EchoOrb"), TEXT("Sphere"), FVector(0, 0, 130), FVector(.4), FLinearColor(.2, 1, 1), 2);
    }
    else if (Kind == EPOCProp::Bounce)
    {
        Mesh->SetRelativeScale3D(FVector(1.3, 1.3, .4));
        Mesh->SetRelativeLocation(FVector(0, 0, 17));
        Color = FLinearColor(.52, .33, .55);
        POCVisuals::Part(this, Visual, TEXT("BounceStem"), TEXT("Cylinder"), FVector(0, 0, -5), FVector(.2, .2, .4), FLinearColor(.26, .3, .32));
    }
    else if (Kind == EPOCProp::Hazard)
    {
        Mesh->SetRelativeScale3D(FVector(.9, 6, .18));
        Color = FLinearColor(.9, .24, .1);
    }
    else if (Kind == EPOCProp::Cake)
    {
        Mesh->SetStaticMesh(POCVisuals::Shape(TEXT("Cylinder")));
        Mesh->SetRelativeScale3D(FVector(.8, .8, .72));
        Mesh->SetRelativeLocation(FVector(0, 0, 36));
        Color = FLinearColor(.42, .31, .21); Glow = 0;
        POCVisuals::Part(this, Visual, TEXT("Plate"), TEXT("Cylinder"), FVector(0, 0, 73), FVector(.55, .55, .025), FLinearColor(.91, .88, .76));
        POCVisuals::Part(this, Visual, TEXT("Sponge"), TEXT("Cube"), FVector(0, 0, 82), FVector(.28, .22, .15), FLinearColor(.76, .53, .28));
        POCVisuals::Part(this, Visual, TEXT("Jam"), TEXT("Cube"), FVector(0, 0, 82), FVector(.283, .223, .023), FLinearColor(.52, .035, .08));
        POCVisuals::Part(this, Visual, TEXT("Icing"), TEXT("Cube"), FVector(0, 0, 91), FVector(.29, .23, .045), FLinearColor(.95, .9, .79));
        POCVisuals::Part(this, Visual, TEXT("Berry"), TEXT("Sphere"), FVector(0, 0, 96), FVector(.06), FLinearColor(.75, .05, .08));
    }
    Surface = POCVisuals::Material(this, Color, Glow);
    Mesh->SetMaterial(0, Surface);
}

void APOCProp::Activate(float Duration)
{
    ActiveRemaining = Duration;
    if (Kind == EPOCProp::EchoBridge)
    {
        Mesh->SetVisibility(true);
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
}

void APOCProp::ResetPlatform()
{
    CollapseTime = -1; Touched = false; ActiveRemaining = 0;
    if (Kind == EPOCProp::Crumble || Kind == EPOCProp::Moving || Kind == EPOCProp::EchoBridge)
    {
        SetActorLocation(Origin);
        SetActorRotation(FRotator(0, GetActorRotation().Yaw, 0));
        SetActorHiddenInGame(false);
        Mesh->SetVisibility(Kind != EPOCProp::EchoBridge);
        Mesh->SetCollisionEnabled(Kind == EPOCProp::EchoBridge ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
    }
}

void APOCProp::Collect(APOCCharacter* Player)
{
    auto* GI = GetGameInstance<UPOCGameInstance>();
    if (Taken || GI->Collected.Contains(Id)) return;
    Taken = true;
    GI->Collected.Add(Id);
    if (Kind == EPOCProp::Relic) { ++GI->Relics; Journey->ShowCaption(TEXT("A tiny piece of a very big story.")); }
    else ++GI->Shards;
    SetActorHiddenInGame(true);
    Journey->Sound(Kind == EPOCProp::Relic ? TEXT("Relic") : TEXT("Shard"), GetActorLocation(), 1 + (GI->Shards % 5) * .06f);
    Journey->Burst(GetActorLocation(), Kind == EPOCProp::Relic ? FLinearColor(1, .6, .1) : FLinearColor(.2, .8, .9), 6);
}

void APOCProp::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds;
    auto* Player = Journey ? Journey->Player.Get() : nullptr;
    if (!Player || Taken) return;
    const FVector Delta = Player->GetActorLocation() - GetActorLocation();
    const float FlatDistance = Delta.Size2D();
    const FVector Local = GetActorTransform().InverseTransformPosition(Player->GetActorLocation());
    const bool OnTop = FMath::Abs(Local.X) < Size.X * .5 + 15 && FMath::Abs(Local.Y) < Size.Y * .5 + 15
        && Local.Z > 10 && Local.Z < 100;
    if (ActiveRemaining > 0) ActiveRemaining = FMath::Max(0.f, ActiveRemaining - DeltaSeconds);
    switch (Kind)
    {
    case EPOCProp::Shard:
    case EPOCProp::Relic:
        Mesh->AddLocalRotation(FRotator(0, DeltaSeconds * 80, 0));
        Mesh->SetRelativeLocation(FVector(0, 0, FMath::Sin(Age * 3 + Id) * 9));
        if (Delta.SizeSquared() < FMath::Square(Kind == EPOCProp::Relic ? 85.f : 75.f)) Collect(Player);
        break;
    case EPOCProp::Checkpoint:
        if (FlatDistance < 135 && FMath::Abs(Delta.Z) < 130 && Journey->ActiveCheckpoint != Id)
        {
            Journey->ActiveCheckpoint = Id;
            Player->Checkpoint = FTransform(GetActorRotation(), Origin + FVector(0, 0, 52));
            Player->Hearts = 3;
            Journey->ShowCaption(TEXT("A moment to catch your breath."), 2.4);
            Journey->Sound(TEXT("Checkpoint"), Origin);
            GetGameInstance<UPOCGameInstance>()->SaveProgress();
        }
        Surface->SetScalarParameterValue(TEXT("Glow"), Journey->ActiveCheckpoint == Id ? 1.5 : .2);
        break;
    case EPOCProp::EchoNode:
        if (FlatDistance < 400 && FMath::Abs(Delta.Z) < 220) Journey->Prompt = TEXT("E / Y    Remember the way");
        break;
    case EPOCProp::Bounce:
        if (ActiveRemaining <= 0 && FlatDistance < 75 && Delta.Z > 25 && Delta.Z < 85 && Player->GetVelocity().Z <= 0)
        {
            ActiveRemaining = .35;
            Player->Bounce(); Journey->Burst(Origin + FVector(0, 0, 35), FLinearColor(.8, .4, .7), 12);
        }
        break;
    case EPOCProp::Moving:
        SetActorLocation(Origin + GetActorRightVector() * FMath::Sin(Age * .75) * 150);
        break;
    case EPOCProp::EchoBridge:
        if (ActiveRemaining <= 0)
        {
            // Never remove a supporting surface from directly under a player.
            if (!OnTop) { Mesh->SetVisibility(false); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
        }
        else Surface->SetScalarParameterValue(TEXT("Glow"), ActiveRemaining < 4 ? .6 + FMath::Abs(FMath::Sin(Age * 9)) * 1.5 : .7);
        break;
    case EPOCProp::Crumble:
        if (OnTop && CollapseTime < 0) { CollapseTime = Age + 2.4f; Journey->Sound(TEXT("Rumble"), Origin); }
        if (CollapseTime > 0)
        {
            const float Falling = Age - CollapseTime;
            if (Falling < 0) SetActorLocation(Origin + FVector(0, FMath::Sin(Age * 58) * 3, 0));
            else
            {
                Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                SetActorLocation(Origin - FVector(0, 0, Falling * Falling * 800));
                if (Falling > 2) SetActorHiddenInGame(true);
            }
        }
        break;
    case EPOCProp::Hazard:
    {
        const float Cycle = FMath::Fmod(Age + Id * .13f, 3.4f);
        const bool Active = Cycle > 1.3f && Cycle < 2.4f;
        Mesh->SetRelativeScale3D(FVector(.9, 6, Active ? 1.2 : .12));
        Mesh->SetRelativeLocation(FVector(0, 0, Active ? 60 : 6));
        Surface->SetScalarParameterValue(TEXT("Glow"), Cycle > .6f && Cycle < 2.4f ? 1.8f : .05f);
        if (Active && FMath::Abs(Local.X) < 70 && FMath::Abs(Local.Y) < 320 && Local.Z < 165 && Local.Z > -10) Player->Hurt(Origin);
        break;
    }
    case EPOCProp::Cake:
        if (FlatDistance < 170 && FMath::Abs(Delta.Z) < 160 && !Player->Eating) Journey->Prompt = TEXT("E / Y    Finally. Cake.");
        if (Player->Eating)
        {
            if (!Touched) { Touched = true; CollapseTime = Age + 1.4f; }
            if (Age > CollapseTime)
                for (UActorComponent* Component : GetComponents())
                    if (auto* Part = Cast<UStaticMeshComponent>(Component))
                        if (Part != Mesh && Part->GetFName() != TEXT("Plate")) Part->SetVisibility(false);
        }
        break;
    default: break;
    }
}

APOCEnemy::APOCEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    Visual = CreateDefaultSubobject<USceneComponent>(TEXT("EnemyRoot"));
    SetRootComponent(Visual);
}

void APOCEnemy::BeginPlay()
{
    Super::BeginPlay();
    Home = GetActorLocation(); Forward = GetActorForwardVector();
    const FLinearColor Color = Species == 1 ? FLinearColor(.22, .31, .35) : Species == 2 ? FLinearColor(.42, .31, .52) : FLinearColor(.43, .51, .28);
    Shell = POCVisuals::Part(this, Visual, TEXT("Shell"), Species == 1 ? TEXT("Cube") : TEXT("Sphere"), FVector::ZeroVector,
        Species == 1 ? FVector(.9, .9, 1.1) : FVector(.7, .7, .5), Color);
    Surface = POCVisuals::Material(this, Color);
    Shell->SetMaterial(0, Surface);
    for (int32 Side : {-1, 1})
    {
        POCVisuals::Part(this, Visual, *FString::Printf(TEXT("Eye%d"), Side), TEXT("Sphere"), FVector(32, Side * 17, 7), FVector(.06, .09, .08), FLinearColor(.95, .68, .26), .8);
        POCVisuals::Part(this, Visual, *FString::Printf(TEXT("Limb%d"), Side), Species == 2 ? TEXT("Cone") : TEXT("Sphere"), FVector(0, Side * 39, -15),
            Species == 2 ? FVector(.7, .3, .08) : FVector(.24, .22, .2), Color);
    }
}

void APOCEnemy::Bonked(APOCCharacter* Player, bool Slam)
{
    if (DefeatTime >= 0 || HitCooldown > 0) return;
    HitCooldown = .45;
    ++Hits;
    if (Species != 1 || Hits >= 2 || Slam)
    {
        DefeatTime = 0;
        Journey->Burst(GetActorLocation(), FLinearColor(.7, .8, .65), 14);
        Journey->Sound(TEXT("Pop"), GetActorLocation());
    }
    else { Windup = -1; Cooldown = 1.2; Journey->Sound(TEXT("Bonk"), GetActorLocation(), .6); }
}

void APOCEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds; Cooldown -= DeltaSeconds; HitCooldown -= DeltaSeconds;
    auto* Player = Journey ? Journey->Player.Get() : nullptr;
    if (!Player) return;
    if (DefeatTime >= 0)
    {
        DefeatTime += DeltaSeconds;
        const float Amount = FMath::Max(.001f, 1 - DefeatTime * 2.5f);
        SetActorScale3D(FVector(1 + DefeatTime, 1 + DefeatTime, Amount));
        AddActorWorldRotation(FRotator(0, DeltaSeconds * 330, 0));
        if (DefeatTime > .4f) { SetActorHiddenInGame(true); SetActorTickEnabled(false); }
        return;
    }
    FVector Delta = Player->GetActorLocation() - GetActorLocation();
    if (Delta.Size2D() < 70 && Delta.Z > 35 && Delta.Z < 105 && Player->GetVelocity().Z < -60)
    { Bonked(Player, Player->Slamming); Player->Bounce(750); return; }
    const float Range = Species == 1 ? 240.f : 135.f;
    if (Windup >= 0)
    {
        Windup += DeltaSeconds;
        Surface->SetScalarParameterValue(TEXT("Glow"), 1 + FMath::Sin(Windup * 24) * .5);
        Shell->SetRelativeScale3D(Species == 1 ? FVector(.9, .9, 1.1 - Windup * .2) : FVector(.8, .8, .4));
        if (Windup > .65)
        {
            Journey->Burst(GetActorLocation(), FLinearColor(1, .55, .2), 9);
            if (Delta.Size2D() < Range && FMath::Abs(Delta.Z) < 115) Player->Hurt(GetActorLocation());
            Windup = -1; Cooldown = 1.5;
        }
    }
    else
    {
        Surface->SetScalarParameterValue(TEXT("Glow"), HitCooldown > 0 ? 1 : 0);
        Shell->SetRelativeScale3D(Species == 1 ? FVector(.9, .9, 1.1) : FVector(.7, .7, .5));
        if (Species != 1)
        {
            SetActorLocation(Home + Forward * FMath::Sin(Age * .8) * 180 + FVector(0, 0, Species == 2 ? FMath::Sin(Age * 2) * 32 : FMath::Abs(FMath::Sin(Age * 5)) * 4));
            SetActorRotation(FRotator(0, Forward.Rotation().Yaw + (FMath::Cos(Age * .8) < 0 ? 180 : 0), 0));
        }
        if (Cooldown <= 0 && Delta.Size2D() < Range + 50 && FMath::Abs(Delta.Z) < 160 && !Player->Eating)
        { Windup = 0; Journey->Sound(TEXT("Warn"), GetActorLocation()); }
    }
}
