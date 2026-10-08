#include "POCWorld.h"
#include "POCCharacter.h"
#include "POCGameInstance.h"
#include "POCVisuals.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

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
    Mesh->SetCastShadow(false);
    const bool Platform = Kind == EPOCProp::Moving || Kind == EPOCProp::Lift || Kind == EPOCProp::EchoBridge || Kind == EPOCProp::Crumble;
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
    else if (Kind == EPOCProp::Crusher)
    {
        Color=FLinearColor(.27,.19,.12);Glow=0;
        Mesh->SetStaticMesh(POCVisuals::Shape(TEXT("Cube")));
        Mesh->SetRelativeScale3D(FVector(2.4,Size.Y/100,3.2));Mesh->SetRelativeLocation(FVector(0,0,850));
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        for(int32 Side:{-1,1})
            POCVisuals::Part(this,Visual,*FString::Printf(TEXT("PressRail%d"),Side),TEXT("Cylinder"),FVector(0,Side*(Size.Y*.5+60),520),FVector(.45,.45,10.4),FLinearColor(.45,.27,.1));
        WarningRing=POCVisuals::Part(this,Visual,TEXT("PressLandingMark"),TEXT("Cube"),FVector(0,0,2),FVector(3.4,Size.Y/100,.025),FLinearColor(1,.55,.06),.45);
        for(int32 Stripe=-3;Stripe<=3;++Stripe)
            POCVisuals::Part(this,Mesh,*FString::Printf(TEXT("PressStripe%d"),Stripe),TEXT("Cube"),FVector(-51,Stripe*12,-20),FVector(.02,.025,.15),FLinearColor(1,.66,.04),.3);
    }
    else if (Kind == EPOCProp::ArenaGate)
    {
        Color=FLinearColor(.07,.25,.18);Glow=.1;
        Mesh->SetStaticMesh(POCVisuals::Shape(TEXT("Cube")));
        Mesh->SetRelativeScale3D(FVector(.55,Size.Y/100,6));Mesh->SetRelativeLocation(FVector(0,0,300));
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        for(int32 I=-3;I<=3;++I)
            POCVisuals::Part(this,Mesh,*FString::Printf(TEXT("ArenaRune%d"),I),TEXT("Cube"),FVector(-52,I*12,0),FVector(.02,.025,.7),FLinearColor(.8,.45,.09),.4);
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
        Mesh->SetRelativeScale3D(FVector(1.1,Size.Y/100,.12));
        Color=FLinearColor(.16,.12,.08);
        for(int32 Jet=0;Jet<9;++Jet)
        {
            const float Y=(Jet-4)*Size.Y/9;
            auto* Flame=POCVisuals::Part(this,Visual,*FString::Printf(TEXT("Flame%d"),Jet),TEXT("Cone"),FVector(0,Y,160),FVector(1.3,1.3,3.2),FLinearColor(1,.18,.025),.85);
            Flame->SetVisibility(false);FlameJets.Add(Flame);
            auto* Core=POCVisuals::Part(this,Visual,*FString::Printf(TEXT("FlameCore%d"),Jet),TEXT("Cone"),FVector(0,Y,110),FVector(.85,.85,2.2),FLinearColor(1,.66,.1),1.2);
            Core->SetVisibility(false);FlameJets.Add(Core);
        }
    }
    else if (Kind == EPOCProp::Bomb)
    {
        Color = FLinearColor(.14,.17,.22); Glow = .05;
        Mesh->SetRelativeLocation(FVector(0,0,34)); Mesh->SetRelativeScale3D(FVector(.7));
        POCVisuals::Part(this,Visual,TEXT("BombBand"),TEXT("Cylinder"),FVector(0,0,34),FVector(.73,.73,.10),FLinearColor(1,.28,.07));
        POCVisuals::Part(this,Visual,TEXT("Fuse"),TEXT("Cylinder"),FVector(0,0,78),FVector(.075,.075,.26),FLinearColor(1,.7,.22),.6);
        WarningRing = POCVisuals::Part(this,Visual,TEXT("BlastFootprint"),TEXT("Cylinder"),FVector(0,0,3),FVector(7.2,7.2,.015),FLinearColor(.48,.10,.035),.15);
        WarningRing->SetVisibility(false);
    }
    else if (Kind == EPOCProp::Sweeper)
    {
        Color = FLinearColor(.94,.42,.08); Glow = .2;
        Mesh->SetStaticMesh(POCVisuals::Shape(TEXT("Cube")));
        Mesh->SetRelativeScale3D(FVector(.42,9.2,.42)); Mesh->SetRelativeLocation(FVector(0,0,65));
        POCVisuals::Part(this,Visual,TEXT("Axle"),TEXT("Cylinder"),FVector(0,0,40),FVector(.75,.75,.8),FLinearColor(.12,.16,.19));
        for(int32 Side:{-1,1}) POCVisuals::Part(this,Visual,*FString::Printf(TEXT("WarningCap%d"),Side),TEXT("Sphere"),FVector(0,Side*460,65),FVector(.65),Color,1);
    }
    else if (Kind==EPOCProp::PressurePlate)
    {
        Color=FLinearColor(.66,.38,.10); Glow=.12;
        Mesh->SetStaticMesh(POCVisuals::Shape(TEXT("Cube")));
        Mesh->SetRelativeScale3D(FVector(2,2,.08)); Mesh->SetRelativeLocation(FVector(0,0,4));
        POCVisuals::Part(this,Visual,TEXT("PlateMark"),TEXT("Cube"),FVector(0,0,9),FVector(.9,.15,.02),FLinearColor(.95,.72,.2),.4);
    }
    else if (Kind==EPOCProp::GateSwitch)
    {
        Color=FLinearColor(.34,.25,.12); Glow=.1;
        Mesh->SetStaticMesh(POCVisuals::Shape(TEXT("Cylinder")));
        Mesh->SetRelativeScale3D(FVector(1.2,1.2,.8)); Mesh->SetRelativeLocation(FVector(0,0,40));
        WarningRing=POCVisuals::Part(this,Visual,TEXT("SwitchButton"),TEXT("Sphere"),FVector(0,0,90),FVector(.65,.65,.25),FLinearColor(1,.5,.07),.5);
        for(int32 J=0;J<4;++J)
            POCVisuals::Part(this,Visual,*FString::Printf(TEXT("Socket%d"),J),TEXT("Cube"),FVector(0,(J-1.5)*24,112),FVector(.1,.1,.19),FLinearColor(.1,.7,.8),.5);
    }
    else if(Kind==EPOCProp::CakeDoor)
    {
        Color=FLinearColor(.23,.10,.08); Glow=.08;
        Mesh->SetStaticMesh(POCVisuals::Shape(TEXT("Cube")));
        Mesh->SetRelativeScale3D(FVector(.6,21,6.2)); Mesh->SetRelativeLocation(FVector(0,0,310));
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        for(int32 Side:{-1,1})
            POCVisuals::Part(this,Visual,*FString::Printf(TEXT("DoorSeal%d"),Side),TEXT("Cube"),FVector(-33,Side*300,310),FVector(.04,.55,5.5),FLinearColor(.95,.52,.10),.4);
    }
    else if (Kind == EPOCProp::Cake)
    {
        Mesh->SetStaticMesh(POCVisuals::Shape(TEXT("Cylinder")));
        Mesh->SetRelativeScale3D(FVector(.8, .8, .72));
        Mesh->SetRelativeLocation(FVector(0, 0, 36));
        Color = FLinearColor(.42, .31, .21); Glow = 0;
        POCVisuals::Part(this, Visual, TEXT("Plate"), TEXT("Cylinder"), FVector(0, 0, 73), FVector(.55, .55, .025), FLinearColor(.91, .88, .76));
        POCVisuals::Part(this, Visual, TEXT("Sponge"), TEXT("Cube"), FVector(0, 0, 82), FVector(.28, .22, .15), FLinearColor(1,.58,.12));
        POCVisuals::Part(this, Visual, TEXT("Jam"), TEXT("Cube"), FVector(0, 0, 82), FVector(.283, .223, .023), FLinearColor(.9,.035,.16));
        POCVisuals::Part(this, Visual, TEXT("Icing"), TEXT("Cube"), FVector(0, 0, 91), FVector(.29, .23, .045), FLinearColor(1,.88,.68));
        POCVisuals::Part(this, Visual, TEXT("Berry"), TEXT("Sphere"), FVector(0, 0, 96), FVector(.06), FLinearColor(1,.06,.14));
    }
    Surface = POCVisuals::Material(this, Color, Glow);
    Mesh->SetMaterial(0, Surface);
}

void APOCProp::Activate(float Duration)
{
    ActiveRemaining = Duration;
    if(Kind==EPOCProp::Bomb && BombCooldown<=0 && !IsHidden()) FuseRemaining = FuseRemaining<0 ? Duration : FMath::Min(FuseRemaining,Duration);
    if (Kind == EPOCProp::EchoBridge)
    {
        Mesh->SetVisibility(true);
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }
}

bool APOCProp::IsDangerous() const
{
    const float Phase = FMath::Fmod(GetWorld()->GetTimeSeconds() + (Id%24)/8 * .35f, 3.6f);
    return Kind == EPOCProp::Hazard && Phase >= 1.05f && Phase < 2.05f;
}

void APOCProp::Kick(APOCCharacter* Player)
{
    if (Kind != EPOCProp::Bomb || BombCooldown > 0 || IsHidden() || !BombVelocity.IsNearlyZero()) return;
    BombVelocity = Player->GetActorForwardVector()*1150 + FVector(0,0,330);
    FuseRemaining = .85f; ++Kicks;
    if (WarningRing) WarningRing->SetVisibility(false);
    Journey->Sound(TEXT("Bonk"),GetActorLocation(),.7);
    Journey->ShowCaption(TEXT("Return to sender."),1.2);
}

void APOCProp::ResetPlatform()
{
    if (Kind == EPOCProp::Bomb)
    {
        FuseRemaining = -1; BombCooldown = 0; BombVelocity = FVector::ZeroVector;
        SetActorLocation(Origin); SetActorHiddenInGame(Group == -2);
        if (WarningRing) WarningRing->SetVisibility(false);
    }
    CollapseTime = -1; Touched = false; EncounterCleared = false; ActiveRemaining = 0;
    if (Kind == EPOCProp::Crumble || Kind == EPOCProp::Moving || Kind == EPOCProp::Lift || Kind == EPOCProp::EchoBridge)
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
    if (!Player || Player->Dying || Player->DeveloperFlight || Taken) return;
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
    case EPOCProp::Lift:
        SetActorLocation(Origin+FVector(0,0,110+110*FMath::Sin(GetWorld()->GetTimeSeconds()*1.05f+Id)));
        if(FlatDistance<700 && Delta.Z<450 && Delta.Z>-100) Journey->Prompt=TEXT("RISING LIFT  -  board when low; ride, then jump");
        break;
    case EPOCProp::Crusher:
    {
        const float Phase=FMath::Fmod(GetWorld()->GetTimeSeconds()+Id*.31f,4.f);
        const float Lift=Phase<.85f?1.f:Phase<1.10f?1.f-(Phase-.85f)/.25f:Phase<2.2f?0.f:Phase<2.55f?(Phase-2.2f)/.35f:1.f;
        Mesh->SetRelativeLocation(FVector(0,0,160+690*Lift));
        Surface->SetVectorParameterValue(TEXT("Tint"),Phase<.85f?FLinearColor(.9,.44,.035):Lift<.7f?FLinearColor(.55,.11,.035):FLinearColor(.12,.46,.35));
        if(Lift<.18f && FMath::Abs(Local.X)<145 && FMath::Abs(Local.Y)<Size.Y*.5 && Local.Z<330 && Local.Z>0) Player->Hurt(Origin-GetActorForwardVector()*60,350,800,1);
        if(FlatDistance<650 && FMath::Abs(Delta.Z)<250) Journey->Prompt=Lift>.95f && Phase>2.55f ? TEXT("PRESS OPEN  -  cross now") : TEXT("HEAVY PRESS  -  wait for teal, then cross");
        break;
    }
    case EPOCProp::ArenaGate:
    {
        int32 Remaining=0;
        if(!EncounterCleared && Journey->Route.IsValidIndex(Group))
            for(TActorIterator<APOCEnemy> It(GetWorld());It;++It)
                if(!It->IsHidden() && It->ArenaGroup==Group) ++Remaining;
        if(!EncounterCleared && Remaining==0)
        { EncounterCleared=true;Journey->ShowCaption(TEXT("Sentry court cleared. The archive opens."),3);Journey->Sound(TEXT("Echo"),Origin); }
        Mesh->SetRelativeLocation(FVector(0,0,FMath::FInterpTo(Mesh->GetRelativeLocation().Z,EncounterCleared?1050.f:300.f,DeltaSeconds,2.5f)));
        if(!EncounterCleared && FlatDistance<2000) Journey->Prompt=FString::Printf(TEXT("SENTRY COURT  -  %d guards remain  /  J spin; jump to dodge"),Remaining);
        break;
    }
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
                if(Falling>5 && (FlatDistance>Size.X*.6f || Delta.Z>150 || Delta.Z < -120))
                { CollapseTime=-1;SetActorLocation(Origin);SetActorHiddenInGame(false);Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); }
            }
        }
        break;
    case EPOCProp::Hazard:
    {
        // All vents in a room share a clock: amber warning, red blast, safe blue.
        const float Cycle = FMath::Fmod(GetWorld()->GetTimeSeconds() + (Id%24)/8 * .35f, 3.6f);
        const bool Active = IsDangerous();
        const bool Warning = Cycle < 1.05f;
        Mesh->SetRelativeScale3D(FVector(1.1,Size.Y/100,.10));
        Mesh->SetRelativeLocation(FVector(0,0,5));
        for(int32 J=0;J<FlameJets.Num();++J)
        {
            FlameJets[J]->SetVisibility(Active);
            // Keep the fuse/damage clock exact; animate only visible nearby jets.
            if(!Active || FlatDistance>3500) continue;
            const bool Core=J%2==1;
            const float Height=(Core?200.f:310.f)+FMath::Sin(Age*17+J*1.7f)*(Core?25:45);
            FlameJets[J]->SetRelativeScale3D(FVector(Core?.85:1.3,Core?.85:1.3,Height/100));
            FlameJets[J]->SetRelativeLocation(FVector(FMath::Sin(Age*9+J)*6,(J/2-4)*Size.Y/9,Height*.5f+8));
        }
        Surface->SetVectorParameterValue(TEXT("Tint"), Active ? FLinearColor(1,.16,.035) : Warning ? FLinearColor(1,.58,.045) : FLinearColor(.08,.52,.48));
        Surface->SetScalarParameterValue(TEXT("Glow"), Active ? .8 : Warning ? .3f + .4f*FMath::Abs(FMath::Sin(Cycle*14)) : .12f);
        if (Active && FMath::Abs(Local.X) < 82 && FMath::Abs(Local.Y) < Size.Y*.5+25 && Local.Z < 395 && Local.Z > -10) Player->Hurt(Origin);
        if (FMath::Abs(Local.X)<420 && Warning && Local.Z<180) Journey->Prompt = TEXT("AMBER: wait    TEAL: cross    Q: dash");
        break;
    }
    case EPOCProp::Sweeper:
    {
        const float Angle = GetWorld()->GetTimeSeconds()*72 + Id*23;
        SetActorRotation(FRotator(0,Angle,0));
        const FVector SweeperLocal = GetActorTransform().InverseTransformPosition(Player->GetActorLocation());
        if(FMath::Abs(SweeperLocal.X)<48 && FMath::Abs(SweeperLocal.Y)<490 && SweeperLocal.Z>12 && SweeperLocal.Z<133) Player->Hurt(Origin);
        break;
    }
    case EPOCProp::Bomb:
    {
        if (BombCooldown > 0)
        {
            BombCooldown -= DeltaSeconds;
            if(BombCooldown<=0 && Group != -2) { SetActorLocation(Origin); SetActorHiddenInGame(false); FuseRemaining=-1;Surface->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.14,.17,.22));Surface->SetScalarParameterValue(TEXT("Glow"),.05); }
            break;
        }
        if (IsHidden()) break;
        if (Group<0 && FuseRemaining < 0 && FlatDistance < 460 && FMath::Abs(Delta.Z)<250)
        { FuseRemaining=1.6f; Journey->Sound(TEXT("Warn"),GetActorLocation(),1.2); }
        if (!BombVelocity.IsNearlyZero())
        {
            BombVelocity.Z -= 980*DeltaSeconds;
            SetActorLocation(GetActorLocation()+BombVelocity*DeltaSeconds);
        }
        if (FuseRemaining >= 0)
        {
            FuseRemaining -= DeltaSeconds;
            Surface->SetVectorParameterValue(TEXT("Tint"),FLinearColor(1,.16,.035));
            Surface->SetScalarParameterValue(TEXT("Glow"),.2f+.6f*FMath::Abs(FMath::Sin(FuseRemaining*22)));
            WarningRing->SetVisibility(BombVelocity.IsNearlyZero());
            Mesh->SetRelativeScale3D(FVector(.7f+.06f*FMath::Sin(FuseRemaining*22)));
            if (FlatDistance<350) Journey->Prompt=TEXT("LIT BOMB!  J: kick it    Q: dash clear");
            if (FuseRemaining<=0)
            {
                ++Explosions;
                const FVector Blast=GetActorLocation()+FVector(0,0,40);
                Journey->Burst(Blast,FLinearColor(1,.35,.045),36); Journey->Sound(TEXT("Slam"),Blast,.6);
                const float BlastDistance=FVector::Dist(Player->GetActorLocation(),Blast);
                if(BlastDistance<360)
                {
                    const float Strength=1-FMath::Clamp(BlastDistance/360.f,0.f,1.f);
                    Player->Hurt(Blast,650+Strength*600,850+Strength*950,BlastDistance<120?2:1);
                }
                for(TActorIterator<APOCProp> It(GetWorld());It;++It)
                    if(*It!=this && It->Kind==EPOCProp::Bomb && FVector::Dist(It->GetActorLocation(),Blast)<460) It->Activate(.4f);
                for (TActorIterator<APOCEnemy> It(GetWorld());It;++It)
                    if(FVector::DistSquared(It->GetActorLocation(),Blast)<FMath::Square(340.f)) It->Bonked(Player,true);
                BombVelocity=FVector::ZeroVector; FuseRemaining=-1; BombCooldown=4.2f;
                WarningRing->SetVisibility(false); SetActorHiddenInGame(true);
            }
        }
        break;
    }
    case EPOCProp::PressurePlate:
        if(Touched && FlatDistance>160) { Touched=false;Mesh->SetRelativeLocation(FVector(0,0,4));Surface->SetScalarParameterValue(TEXT("Glow"),.12); }
        if(!Touched && FlatDistance<115 && Delta.Z>15 && Delta.Z<85)
        {
            Touched=true; Mesh->SetRelativeLocation(FVector(0,0,1)); Surface->SetScalarParameterValue(TEXT("Glow"),1);
            for(TActorIterator<APOCProp> It(GetWorld());It;++It) if(It->Kind==EPOCProp::Bomb && It->Group==Group) It->Activate(1.25f);
            Journey->ShowCaption(TEXT("Click. That was a pressure plate."),2); Journey->Sound(TEXT("Warn"),Origin,.7);
        }
        break;
    case EPOCProp::GateSwitch:
    {
        const bool Powered=Group==0?Journey->LeftSwitch:Journey->RightSwitch;
        auto* GI=GetGameInstance<UPOCGameInstance>();
        Cast<UMaterialInstanceDynamic>(WarningRing->GetMaterial(0))->SetVectorParameterValue(TEXT("Tint"),Powered?FLinearColor(.15,1,.65):FLinearColor(1,.5,.07));
        Surface->SetScalarParameterValue(TEXT("Glow"),Powered?.6:.1);
        if(FlatDistance<230 && FMath::Abs(Delta.Z)<160)
            Journey->Prompt=Powered?TEXT("SWITCH POWERED"):FString::Printf(TEXT("E  POWER SWITCH    %d / %d SHARDS"),GI->Shards-GI->ShardsSpent,APOCWorld::SwitchPrice);
        break;
    }
    case EPOCProp::CakeDoor:
        if(Journey->GateIsOpen())
        {
            if(!Touched) { Touched=true;Journey->Sound(TEXT("Rumble"),Origin,.7); }
            const FVector Target=Origin+FVector(0,0,750);
            SetActorLocation(FMath::VInterpConstantTo(GetActorLocation(),Target,DeltaSeconds,330));
        }
        else if(FlatDistance<700) Journey->Prompt=TEXT("POWER BOTH SIDE SWITCHES  ·  180 SHARDS EACH");
        break;
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
    Shell = POCVisuals::Part(this, Visual, TEXT("Shell"), Species == 1 ? TEXT("WeatheredRock") : TEXT("NoriBody"), FVector::ZeroVector,
        Species == 1 ? FVector(.9, .9, 1.1) : FVector(.7, .7, .5), Color);
    Surface = POCVisuals::Material(this, Color);
    Shell->SetMaterial(0, Surface);
    for (int32 Side : {-1, 1})
    {
        POCVisuals::Part(this, Visual, *FString::Printf(TEXT("Eye%d"), Side), TEXT("Sphere"), FVector(32, Side * 17, 7), FVector(.06, .09, .08), FLinearColor(.95, .68, .26), .8);
        POCVisuals::Part(this, Visual, *FString::Printf(TEXT("Limb%d"), Side), Species == 2 ? TEXT("NoriEar") : TEXT("NoriPaw"), FVector(0, Side * 39, -15),
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

void APOCEnemy::ResetEnemy()
{
    SetActorLocation(Home); SetActorScale3D(FVector(1)); SetActorHiddenInGame(false);
    Hits=0; DefeatTime=-1; Windup=-1; Cooldown=1; HitCooldown=0; ChargeRemaining=0;
}

void APOCEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Age += DeltaSeconds; Cooldown -= DeltaSeconds; HitCooldown -= DeltaSeconds;
    auto* Player = Journey ? Journey->Player.Get() : nullptr;
    if (!Player || Player->Dying || Player->DeveloperFlight) return;
    if (DefeatTime >= 0)
    {
        DefeatTime += DeltaSeconds;
        SetActorScale3D(FVector(1+DefeatTime,1+DefeatTime,FMath::Max(.001f,1-DefeatTime*2.5f)));
        AddActorWorldRotation(FRotator(0,DeltaSeconds*330,0));
        if (DefeatTime>.4f) { SetActorHiddenInGame(true); SetActorTickEnabled(false); }
        return;
    }
    const FVector Delta=Player->GetActorLocation()-GetActorLocation();
    if(Delta.Size2D()<80 && Delta.Z>35 && Delta.Z<125 && Player->GetVelocity().Z < -60)
    { Bonked(Player,Player->Slamming); Player->Bounce(750); return; }
    if (ChargeRemaining>0)
    {
        ChargeRemaining-=DeltaSeconds;
        FVector Position=GetActorLocation()+AttackDirection*950*DeltaSeconds;
        // The guard cannot charge off its own platform or chase across a gap.
        const FVector Offset=Position-Home;
        if(Offset.Size2D()<420) SetActorLocation(Position);
        if(Delta.Size2D()<100 && FMath::Abs(Delta.Z)<105) Player->Hurt(GetActorLocation(),480,900);
        if(ChargeRemaining<=0) Cooldown=1.25;
        return;
    }
    if(Windup>=0)
    {
        Windup+=DeltaSeconds;
        Surface->SetScalarParameterValue(TEXT("Glow"),.4+.6*FMath::Abs(FMath::Sin(Windup*20)));
        if(Windup>=.7)
        {
            if(Species==0) ChargeRemaining=.42;
            else if(Species==1)
            {
                Journey->Burst(GetActorLocation(),FLinearColor(1,.55,.15),24);
                if(Delta.Size2D()<290 && Delta.Z<130 && Delta.Z>-100) Player->Hurt(GetActorLocation(),480,900);
                Cooldown=1.6;
            }
            else { Journey->DropBomb(BombTarget); Cooldown=2.8; }
            Windup=-1;
        }
        return;
    }
    Surface->SetScalarParameterValue(TEXT("Glow"),HitCooldown>0 ? .6f : .08f);
    FVector Idle=Home+Forward*FMath::Sin(Age*.9)*110+FVector(0,0,Species==2?FMath::Sin(Age*2)*25:0);
    if(Species!=2)
    {
        FHitResult Floor; FCollisionQueryParams Query(SCENE_QUERY_STAT(EnemyFeet),false,this);Query.AddIgnoredActor(Player);
        if(GetWorld()->LineTraceSingleByChannel(Floor,Idle+FVector(0,0,350),Idle-FVector(0,0,500),ECC_Visibility,Query)) Idle.Z=Floor.ImpactPoint.Z+50;
    }
    SetActorLocation(FMath::VInterpTo(GetActorLocation(),Idle,DeltaSeconds,2));
    const float Range=Species==2?900:Species==0?420:330;
    if(Cooldown<=0 && Delta.Size2D()<Range && FMath::Abs(Delta.Z)<220 && !Player->Eating)
    {
        Windup=0; AttackDirection=Delta.GetSafeNormal2D();
        SetActorRotation(FRotator(0,AttackDirection.Rotation().Yaw,0));
        const auto* Ground=Journey->Nearest(Player->GetActorLocation());
        BombTarget=Player->GetActorLocation(); BombTarget.Z=Ground?Ground->Position.Z:Home.Z-50;
        Journey->Sound(TEXT("Warn"),GetActorLocation(),Species==2?1.4:.8);
    }
}
