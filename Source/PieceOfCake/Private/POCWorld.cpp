#include "POCWorld.h"
#include "POCCharacter.h"
#include "POCGameInstance.h"
#include "POCVisuals.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundWave.h"

namespace
{
    const TCHAR* SectionNames[] = { TEXT("SUNROOT VAULT"), TEXT("BELLOWS FOUNDRY"), TEXT("FERN ARCHIVES"), TEXT("ECHO SANCTUM"),
        TEXT("PRISM GROTTO"), TEXT("TIDAL GALLERY"), TEXT("CLOCKWORK DESCENT"), TEXT("THE CAKE CHAMBER") };
    const TCHAR* SectionCaptions[] = {
        TEXT("Small paws. Two jumps. SPACE again in the air; Q to dash."),
        TEXT("Heavy presses: amber warns, teal means cross. Send lit bombs back with J."),
        TEXT("The sentry courts are sealed. Defeat their guards to open the archive."),
        TEXT("E wakes the bridges. Double jump, then dash across."),
        TEXT("The path weaves across the grotto. Aim sideways between crystal islands."),
        TEXT("Ride the rising lifts. Wait for a low platform before you leap."),
        TEXT("The floor is on a deadline. So are you."),
        TEXT("One last gauntlet. The cake had better be worth it.") };
    FVector JsonVector(const TSharedPtr<FJsonObject>& Item, const TCHAR* Key)
    {
        const auto& Values = Item->GetArrayField(Key);
        return FVector(Values[0]->AsNumber(), Values[1]->AsNumber(), Values[2]->AsNumber());
    }
}

APOCWorld::APOCWorld()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Journey")));
    Music = CreateDefaultSubobject<UAudioComponent>(TEXT("Music"));
    Music->SetupAttachment(RootComponent);
    Music->bAutoActivate = false;
    Music->bAllowSpatialization = false;
    PropClass = APOCProp::StaticClass();
    EnemyClass = APOCEnemy::StaticClass();
    Palettes = {FLinearColor(.38,.29,.15), FLinearColor(.29,.16,.10), FLinearColor(.13,.30,.20), FLinearColor(.23,.20,.39),
        FLinearColor(.12,.27,.36), FLinearColor(.12,.32,.36), FLinearColor(.31,.22,.16), FLinearColor(.53,.34,.22)};
}

bool APOCWorld::LoadRoute()
{
    FString Text;
    const FString Path = FPaths::ProjectContentDir() / TEXT("Data/journey.json");
    TSharedPtr<FJsonObject> Document;
    if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Document) || !Document.IsValid())
    { LoadError = TEXT("Journey data is missing. Run Scripts/generate_journey.py and rebuild."); return false; }
    const TArray<TSharedPtr<FJsonValue>>* Points;
    if (!Document->TryGetArrayField(TEXT("points"), Points) || Points->Num() != 192)
    { LoadError = TEXT("Journey data is incomplete. Expected 192 route platforms."); return false; }
    for (const auto& Value : *Points)
    {
        const auto Item = Value->AsObject();
        if (!Item.IsValid()) return false;
        FPOCRoutePoint Point;
        Point.Index = Item->GetIntegerField(TEXT("index"));
        Point.Section = Item->GetIntegerField(TEXT("section"));
        Point.Position = JsonVector(Item, TEXT("position"));
        Point.Size = JsonVector(Item, TEXT("size"));
        Point.Yaw = Item->GetNumberField(TEXT("yaw"));
        Point.Kind = Item->GetStringField(TEXT("kind"));
        Point.Group = Item->GetIntegerField(TEXT("group"));
        Point.EchoNode = Item->GetIntegerField(TEXT("echo_node"));
        Point.Enemy = Item->GetIntegerField(TEXT("enemy"));
        Point.Checkpoint = Item->GetBoolField(TEXT("checkpoint"));
        Point.Hazard = Item->GetBoolField(TEXT("hazard"));
        Point.SlideGate = Item->GetBoolField(TEXT("slide_gate"));
        Point.Bounce = Item->GetBoolField(TEXT("bounce"));
        Point.Secret = Item->GetBoolField(TEXT("secret"));
        Item->TryGetBoolField(TEXT("bomb"), Point.Bomb);
        Item->TryGetBoolField(TEXT("sweeper"), Point.Sweeper);
        Item->TryGetStringField(TEXT("challenge"), Point.Challenge);
        if (Point.Section < 0 || Point.Section > 7 || Point.Index != Route.Num())
        { LoadError = TEXT("Journey data has invalid route indices."); return false; }
        Route.Add(Point);
    }
    return true;
}

void APOCWorld::BeginPlay()
{
    Super::BeginPlay();
    if (!LoadRoute()) { UE_LOG(LogTemp, Error, TEXT("%s"), *LoadError); return; }
    if (UClass* Class = LoadClass<APOCProp>(nullptr, TEXT("/Game/Gameplay/BP_EchoObject.BP_EchoObject_C"))) PropClass = Class;
    if (UClass* Class = LoadClass<APOCEnemy>(nullptr, TEXT("/Game/Enemies/BP_Guardian.BP_Guardian_C"))) EnemyClass = Class;
    // Keep short sounds and section music resident before the first input. Runtime
    // LoadObject calls had forced synchronous package flushes during traversal.
    const TCHAR* Effects[] = {TEXT("Bonk"), TEXT("Bounce"), TEXT("Cake"), TEXT("Checkpoint"), TEXT("Echo"),
        TEXT("Hurt"), TEXT("Jump"), TEXT("Land"), TEXT("Pop"), TEXT("Relic"), TEXT("Rumble"), TEXT("Shard"), TEXT("Slam"), TEXT("Slide"), TEXT("Warn")};
    for (const TCHAR* Name : Effects)
        Sounds.Add(FName(Name), LoadObject<USoundWave>(nullptr, *FString::Printf(TEXT("/Game/Audio/A_%s.A_%s"), Name, Name)));
    for (int32 I = 0; I < 8; ++I)
        Tracks.Add(LoadObject<USoundWave>(nullptr, *FString::Printf(TEXT("/Game/Audio/M_%d.M_%d"), I, I)));
    BuildJourney();
    Player = Cast<APOCCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (Player)
    {
        Player->Journey = this;
        Player->Checkpoint = FTransform(FRotator(0, Route[0].Yaw, 0), Route[0].Position + FVector(0, 0, 52));
        Player->Respawn();
    }
    Ready = true;
}

void APOCWorld::AddInstance(const FString& Batch, const TCHAR* Shape, FVector Pos, FVector Scale, FRotator Rotation, FLinearColor Color, bool Collision, float Glow)
{
    int32 BatchIndex;
    if (const int32* Existing = BatchIds.Find(Batch)) BatchIndex = *Existing;
    else
    {
        auto* Instances = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        AddInstanceComponent(Instances);
        Instances->SetupAttachment(RootComponent);
        Instances->SetStaticMesh(POCVisuals::Shape(Shape));
        auto* Surface = POCVisuals::Material(this, Color, Glow);
        const bool Patterned = Batch.Contains(TEXT("Stone")) || Batch.Contains(TEXT("Top")) || Batch.Contains(TEXT("Wall")) || Batch.Contains(TEXT("Roof"));
        const int32 District = FCString::Atoi(*Batch);
        Surface->SetScalarParameterValue(TEXT("SurfaceStyle"), Patterned ? (District == 2 ? 2 : District == 4 ? 4 : District == 5 || District == 7 ? 3 : 1) : 0);
        Instances->SetMaterial(0, Surface);
        Instances->SetCollisionEnabled(Collision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        Instances->SetCollisionResponseToAllChannels(ECR_Block);
        const bool Detail = Batch.Contains(TEXT("Grass")) || Batch.Contains(TEXT("Flower"));
        Instances->SetCullDistances(Detail ? 6000 : 18000, Detail ? 9500 : 32000);
        Instances->SetCastShadow(false);
        Instances->RegisterComponent();
        BatchIndex = Batches.Add(Instances);
        BatchIds.Add(Batch, BatchIndex);
    }
    Batches[BatchIndex]->AddInstance(FTransform(Rotation, Pos, Scale), true);
}

APOCProp* APOCWorld::SpawnProp(EPOCProp Kind, FVector Pos, float Yaw, FVector Size, int32 Id, int32 Group)
{
    const FTransform Transform(FRotator(0, Yaw, 0), Pos);
    auto* Prop = GetWorld()->SpawnActorDeferred<APOCProp>(PropClass, Transform, this);
    Prop->Journey = this;
    Prop->Configure(Kind, Size, Id, Group);
    UGameplayStatics::FinishSpawningActor(Prop, Transform);
    Props.Add(Prop);
    return Prop;
}

void APOCWorld::BuildJourney()
{
    for (const auto& Point : Route)
    {
        const int32 Section = Point.Section, Local = Point.Index % 24;
        const FRotator Rotation(0, Point.Yaw, 0);
        const FVector Forward = Rotation.Vector(), Right = FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y);
        const FString Prefix = FString::FromInt(Section);
        if (Point.Kind == TEXT("ground"))
        {
            if(Point.Index>=185 && Point.Index<=190)
            {
                // Eight real 25cm risers make each stair flight traversable in both directions.
                for(int32 Step=0;Step<8;++Step)
                {
                    const float Along=-Point.Size.X*.5f+(Step+.5f)*Point.Size.X/8;
                    const float Rise=25*(Step+1);
                    AddInstance(Prefix+TEXT("StairStone"),TEXT("Cube"),Point.Position+Forward*Along+FVector(0,0,-200+Rise-20),
                        FVector(Point.Size.X/800+.04,Point.Size.Y/100,.4),Rotation,Palettes[Section]*1.15f,true);
                    AddInstance(Prefix+TEXT("StairNose"),TEXT("Cube"),Point.Position+Forward*(Along-Point.Size.X/16)+FVector(0,0,-200+Rise+1),
                        FVector(.08,Point.Size.Y/100,.025),Rotation,FLinearColor(.9,.57,.22),false,.15);
                }
            }
            else
            {
                AddInstance(Prefix + TEXT("Stone"), TEXT("Cube"), Point.Position - FVector(0, 0, 100), Point.Size / 100, Rotation, Palettes[Section], true);
                AddInstance(Prefix + TEXT("Top"), TEXT("Cube"), Point.Position - FVector(0, 0, 3), FVector(Point.Size.X / 100 - .2, Point.Size.Y / 100 - .2, .06), Rotation, Palettes[Section] * 1.25f);
            }
        }
        else SpawnProp(Point.Kind == TEXT("moving") ? EPOCProp::Moving : Point.Kind == TEXT("lift") ? EPOCProp::Lift : Point.Kind == TEXT("echo") ? EPOCProp::EchoBridge : EPOCProp::Crumble,
            Point.Position, Point.Yaw, Point.Size, 10000 + Point.Index, Point.Group);

        for (int32 J = -1; J <= 1; ++J)
            SpawnProp(EPOCProp::Shard, Point.Position + Forward * (J * 240) + FVector(0, 0, 80), 0, FVector(1), Point.Index * 3 + J + 1);
        if (Point.Checkpoint)
            SpawnProp(EPOCProp::Checkpoint, Point.Position - Forward * (Point.Size.X * .3f), Point.Yaw, FVector(1), Point.Index);
        if (Point.EchoNode >= 0)
            SpawnProp(EPOCProp::EchoNode, Point.Position + Forward * 390 + Right * 210, Point.Yaw, FVector(1), 30000 + Point.Index, Point.EchoNode);
        if(Point.Index>0 && Route[Point.Index-1].Kind==TEXT("echo") && Point.Kind!=TEXT("echo"))
            SpawnProp(EPOCProp::EchoNode,Point.Position-Forward*390+Right*210,Point.Yaw,FVector(1),31000+Point.Index,Route[Point.Index-1].Group);
        if (Point.Enemy >= 0)
        {
            const int32 Species = Point.Enemy;
            const FTransform Transform(Rotation, Point.Position + FVector(0, 0, Species == 2 ? 130 : 50));
            auto* Enemy = GetWorld()->SpawnActorDeferred<APOCEnemy>(EnemyClass, Transform, this);
            Enemy->Journey = this; Enemy->Species = Species;
            UGameplayStatics::FinishSpawningActor(Enemy, Transform); Enemies.Add(Enemy);
        }
        if(Section==2 && (Local==3 || Local==19))
        {
            SpawnProp(EPOCProp::ArenaGate,Point.Position+Forward*(Point.Size.X*.5f-80),Point.Yaw,Point.Size,46000+Point.Index,Point.Index);
            for(int32 Guard=0;Guard<3;++Guard)
            {
                const FVector Home=Point.Position+Right*(Guard-1)*330+Forward*(Guard==1?220:-80)+FVector(0,0,50);
                auto* Enemy=GetWorld()->SpawnActorDeferred<APOCEnemy>(EnemyClass,FTransform(Rotation,Home),this);
                Enemy->Journey=this;Enemy->ArenaGroup=Point.Index;Enemy->Species=Guard==1?1:0;UGameplayStatics::FinishSpawningActor(Enemy,FTransform(Rotation,Home));Enemies.Add(Enemy);
            }
            AddInstance(Prefix+TEXT("ArenaRug"),TEXT("Cube"),Point.Position+FVector(0,0,1),FVector(13,15,.025),Rotation,FLinearColor(.10,.24,.20));
        }
        if (Point.Hazard)
            SpawnProp(Section==1 ? EPOCProp::Crusher : EPOCProp::Hazard, Point.Position, Point.Yaw, Point.Size, 40000 + Point.Index);
        if (Point.Bomb)
        {
            const bool PlateTrap = Point.Section>0 && Point.Index%3==0 && Point.Index<185;
            SpawnProp(EPOCProp::Bomb, Point.Position + Forward * 120, Point.Yaw, FVector(1), 43000 + Point.Index, PlateTrap ? Point.Index : -1);
            if(PlateTrap) SpawnProp(EPOCProp::PressurePlate,Point.Position-Forward*340,Point.Yaw,FVector(1),45000+Point.Index,Point.Index);
        }
        if (Point.Sweeper)
            SpawnProp(EPOCProp::Sweeper, Point.Position, Point.Yaw, FVector(1), 44000 + Point.Index);
        if (Point.SlideGate)
            SpawnProp(EPOCProp::SlideGate, Point.Position, Point.Yaw, FVector(1), 41000 + Point.Index);
        if (Point.Bounce)
            SpawnProp(EPOCProp::Bounce, Point.Position + Forward * 300 + Right * 100, Point.Yaw, FVector(1), 42000 + Point.Index);
        if (Point.Secret)
        {
            const FVector Secret = Point.Position + Right * 830 + FVector(0, 0, 60);
            AddInstance(Prefix + TEXT("Secret"), TEXT("Cube"), Secret - FVector(0, 0, 75), FVector(5, 5, 1.5), Rotation, Palettes[Section], true);
            SpawnProp(EPOCProp::Relic, Secret + FVector(0, 0, 90), 0, FVector(1), 50000 + Section);
            AddInstance(TEXT("SecretMarker"), TEXT("Cone"), Point.Position + Right * 410 + FVector(0, 0, 40), FVector(.18, .18, .8), Rotation, FLinearColor(.85, .64, .22), false, .4);
        }
        if (Point.Kind == TEXT("ground"))
        {
            for (int32 Side : {-1, 1})
            {
                AddInstance(Prefix + TEXT("Border"), TEXT("Cube"), Point.Position + Right * Side * (Point.Size.Y*.5 - 20) + FVector(0,0,3),
                    FVector(Point.Size.X/100, .32, .06), Rotation, Palettes[Section]*1.7f);
                AddInstance(Prefix + TEXT("Foundation"), TEXT("Cube"), Point.Position + Right*Side*(Point.Size.Y*.5-8) - FVector(0,0,60),
                    FVector(Point.Size.X/100,.2,.18),Rotation,Palettes[Section]*.5f);
            }
        }
        if(Point.Kind==TEXT("lift"))
            for(int32 Side:{-1,1}) AddInstance(Prefix+TEXT("LiftGuide"),TEXT("Cylinder"),Point.Position+Right*Side*(Point.Size.Y*.5+70)+FVector(0,0,160),FVector(.28,.28,7),Rotation,FLinearColor(.3,.75,.8),false,.35);
        if(Point.Index>=3 && Point.Index<=6)
            AddInstance(TEXT("PracticeJumpLip"),TEXT("Cube"),Point.Position+Forward*(Point.Size.X*.5-65)+FVector(0,0,3),FVector(.3,Point.Size.Y*.007,.035),Rotation,FLinearColor(.22,.85,.78),false,.25);
        AddScenery(Point);
        // Inlays indicate forward travel without a floating waypoint over every jump.
        AddInstance(TEXT("Waymark"), TEXT("Cone"), Point.Position + Forward * (Point.Size.X*.5f-100) + FVector(0, 0, 6), FVector(.5, .5, .1), FRotator(90, Point.Yaw, 0), FLinearColor(.74, .58, .23), false, .2);
    }
    const auto& Last = Route.Last();
    const FRotator FinalFacing(0, Last.Yaw, 0);
    const FVector FinalForward = FinalFacing.Vector();
    const FVector FinalRight = FRotationMatrix(FinalFacing).GetUnitAxis(EAxis::Y);
    const FVector CakePosition = Last.Position + FinalForward * 780;
    SpawnProp(EPOCProp::CakeDoor,Last.Position+FinalForward*330,Last.Yaw,FVector(1),91000);
    SpawnProp(EPOCProp::GateSwitch,Last.Position-FinalForward*180-FinalRight*520,Last.Yaw,FVector(1),91001,0);
    SpawnProp(EPOCProp::GateSwitch,Last.Position-FinalForward*180+FinalRight*520,Last.Yaw,FVector(1),91002,1);
    AddInstance(TEXT("7RewardRug"),TEXT("Cube"),CakePosition-FVector(0,0,1),FVector(4,4,.03),FinalFacing,FLinearColor(.56,.045,.09));
    for(int32 Side:{-1,1})
    {
        AddInstance(TEXT("7CakePillar"),TEXT("DressedStone"),CakePosition+FinalRight*Side*230+FVector(0,0,190),FVector(.6,.6,3.8),FinalFacing,FLinearColor(.58,.36,.13));
        AddInstance(TEXT("7CakeLight"),TEXT("Sphere"),CakePosition+FinalRight*Side*230+FVector(0,0,410),FVector(.55),FinalFacing,FLinearColor(1,.6,.2),false,2);
    }
    SpawnProp(EPOCProp::Cake, CakePosition, Last.Yaw, FVector(1), 90000);
    // Nori and the title share the lit starting room, keeping the mascot and scenery in range.
    const FVector Start=Route[0].Position;
    const FVector SF=FRotator(0,Route[0].Yaw,0).Vector(), SR=FRotationMatrix(FRotator(0,Route[0].Yaw,0)).GetUnitAxis(EAxis::Y);
    const FVector IntroPosition=Start+SF*300+SR*310+FVector(0,0,148);
    const FVector ViewRight=FRotationMatrix((Start-IntroPosition).Rotation()).GetUnitAxis(EAxis::Y);
    const FVector IntroTarget=Start-ViewRight*74+FVector(0,0,65);
    IntroCamera=GetWorld()->SpawnActor<ACameraActor>(IntroPosition,(IntroTarget-IntroPosition).Rotation());
    IntroCamera->GetCameraComponent()->SetFieldOfView(48);
    const FVector DreamEye=Start+SF*390+SR*330+FVector(0,0,190);
    DreamCamera=GetWorld()->SpawnActor<ACameraActor>(DreamEye,(Start-SR*25+FVector(0,0,110)-DreamEye).Rotation());
    DreamCamera->GetCameraComponent()->SetFieldOfView(52);
    const FVector Bubble=Start-SR*75+SF*25+FVector(0,0,180);
    auto DreamPart=[this](const TCHAR* Name,const TCHAR* Shape,FVector P,FVector Scale,FLinearColor Color)
    { DreamParts.Add(POCVisuals::Part(this,RootComponent,Name,Shape,P,Scale,Color,.45f)); };
    for(int32 J=0;J<3;++J)
        DreamPart(*FString::Printf(TEXT("Thought%d"),J),TEXT("Sphere"),Start-SR*(30+J*13)+FVector(0,0,90+J*22),FVector(.12f+J*.055f),FLinearColor(.9,.86,.7));
    DreamPart(TEXT("DreamCloud"),TEXT("Sphere"),Bubble,FVector(1.25,1.05,.85),FLinearColor(.8,.88,.83));
    const FVector Slice=Bubble+(DreamEye-Bubble).GetSafeNormal()*65;
    DreamPart(TEXT("DreamSponge"),TEXT("Cube"),Slice,FVector(.65,.5,.3),FLinearColor(.9,.47,.08));
    DreamPart(TEXT("DreamJam"),TEXT("Cube"),Slice,FVector(.66,.51,.055),FLinearColor(.85,.035,.12));
    DreamPart(TEXT("DreamIcing"),TEXT("Cube"),Slice+FVector(0,0,17),FVector(.69,.54,.075),FLinearColor(1,.86,.66));
    DreamPart(TEXT("DreamCherry"),TEXT("Sphere"),Slice+FVector(0,0,26),FVector(.13),FLinearColor(1,.045,.12));
    for (const auto& Part : DreamParts) { DreamOrigins.Add(Part->GetRelativeLocation()); DreamScales.Add(Part->GetRelativeScale3D()); }
    ShowDream(false);
    Sun = GetWorld()->SpawnActor<ADirectionalLight>();
    Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sun->SetActorRotation(FRotator(-32, -30, 0));
    Sun->GetLightComponent()->SetIntensity(2.8f);
    Sun->GetLightComponent()->SetCastShadows(false);
    Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->bAtmosphereSunLight = false;
    Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetForwardShadingPriority(1);
    // Soft, shadowless sky fill preserves the warm key light without unreadable
    // black silhouettes, including on the non-Lumen Mac performance preset.
    auto* Fill = GetWorld()->SpawnActor<ADirectionalLight>();
    Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Fill->SetActorRotation(FRotator(-45, 150, 0));
    Fill->GetLightComponent()->SetIntensity(.6f);
    Fill->GetLightComponent()->SetLightColor(FLinearColor(.56, .75, 1));
    Fill->GetLightComponent()->SetCastShadows(false);
    Fog = GetWorld()->SpawnActor<AExponentialHeightFog>();
    Fog->GetComponent()->SetFogDensity(.002f);
    Fog->GetComponent()->SetFogHeightFalloff(.12f);
    Fog->GetComponent()->SetStartDistance(1200);
    Fog->GetComponent()->SetFogMaxOpacity(.7);
    auto* Post = GetWorld()->SpawnActor<APostProcessVolume>();
    Post->bUnbound = true;
    Post->Settings.bOverride_BloomIntensity = true; Post->Settings.BloomIntensity = .18;
    Post->Settings.bOverride_VignetteIntensity = true; Post->Settings.VignetteIntensity = .10;
    Post->Settings.bOverride_AutoExposureBias = true; Post->Settings.AutoExposureBias = 0;
    Post->Settings.bOverride_ColorSaturation = true; Post->Settings.ColorSaturation = FVector4(1.15f, 1.15f, 1.15f, 1.f);
    Post->Settings.bOverride_ColorContrast = true; Post->Settings.ColorContrast = FVector4(1.1f, 1.1f, 1.1f, 1.f);
    Post->Settings.bOverride_AmbientOcclusionIntensity = true; Post->Settings.AmbientOcclusionIntensity = 0;
    // Fixed-size particle pool: bursts recycle slots, never spawn unbounded actors.
    for (int32 Index = 0; Index < 72; ++Index)
    {
        auto* Particle = POCVisuals::Part(this, RootComponent, *FString::Printf(TEXT("Mote%d"), Index), TEXT("Sphere"), FVector::ZeroVector, FVector(.04), FLinearColor(.5, .9, 1), 1.5);
        Particle->SetVisibility(false); Particles.Add(Particle);
        ParticleVelocity.Add(FVector::ZeroVector); ParticleLife.Add(0);
    }
}

void APOCWorld::AddScenery(const FPOCRoutePoint& Point)
{
    const int32 Section = Point.Section, Local = Point.Index % 24;
    const FString Key = FString::FromInt(Section);
    const FRotator Rot(0, Point.Yaw, 0);
    const FVector Right = FRotationMatrix(Rot).GetUnitAxis(EAxis::Y), Forward = Rot.Vector();
    const FLinearColor Accent[] = {FLinearColor(.65,.85,.25), FLinearColor(1,.27,.055), FLinearColor(.25,.8,.48), FLinearColor(.62,.38,1),
        FLinearColor(.2,.8,1), FLinearColor(.18,.75,.85), FLinearColor(1,.58,.12), FLinearColor(1,.67,.35)};
    static const float RoofHeights[]={1100,1450,1700,1900,2100,1500,950,1600};
    const float Roof=RoofHeights[Section];
    // Continuous wall/ceiling shells span the gaps too. There is no open sky.
    const FVector End = Route.IsValidIndex(Point.Index+1) ? Route[Point.Index+1].Position : Point.Position+Forward*900;
    const FVector Start = Point.Index == 0 ? Point.Position-Forward*900 : Point.Position;
    const FVector Mid = (Start+End)*.5;
    const float Length = FVector::Dist2D(Start,End)+160;
    const FRotator SpanRot(0,(End-Start).Rotation().Yaw,0);
    const FVector SpanRight = FRotationMatrix(SpanRot).GetUnitAxis(EAxis::Y);
    const float CeilingZ = FMath::Max(Start.Z,End.Z)+Roof;
    for (int32 Side : {-1,1})
    {
        AddInstance(Key+TEXT("Wall"),TEXT("Cube"),Mid+SpanRight*Side*1190+FVector(0,0,(Roof-1300)*.5f),FVector(Length/100,2.4,(Roof+1700)/100),SpanRot,Palettes[Section]*.72f,true);
        const FVector Pillar = Point.Position+Right*Side*1050;
        AddInstance(Key+TEXT("Pier"),TEXT("DressedStone"),Pillar+FVector(0,0,Roof*.5),FVector(1.6,2,Roof/100),Rot,Palettes[Section]*1.1f);
        AddInstance(Key+TEXT("PierBase"),TEXT("Cube"),Pillar+FVector(0,0,60),FVector(2.4,2.6,1.2),Rot,Palettes[Section]*.65f);
        AddInstance(Key+TEXT("LightBracket"),TEXT("Cube"),Pillar-Right*Side*70+FVector(0,0,360),FVector(.7,.6,1.7),Rot,Palettes[Section]*.4f);
        AddInstance(Key+TEXT("Light"),TEXT("Cube"),Pillar-Right*Side*105+FVector(0,0,365),FVector(.35,.12,1.15),Rot,Accent[Section],false,1.5);
        AddInstance(Key+TEXT("WallBand"),TEXT("Cube"),Mid+SpanRight*Side*1058+FVector(0,0,700),FVector(Length/100,.12,.12),SpanRot,Accent[Section]*.6f,false,.25);
        if (Section==0 || Section==2)
        {
            for (int32 J=0;J<3;++J)
                AddInstance(Key+TEXT("Fern"),TEXT("Fern"),Pillar+Forward*(J*170-170)-Right*Side*100+FVector(0,0,110),FVector(2.6,2.6,3.1),Rot,FLinearColor(.12,.38,.15));
            AddInstance(Key+TEXT("Root"),TEXT("Cylinder"),Pillar+FVector(0,0,750),FVector(1.2,1.2,8),FRotator(20,Point.Yaw,Side*22),FLinearColor(.24,.16,.08));
            for (int32 J=0;J<3;++J)
                AddInstance(Key+TEXT("HangingLeaf"),TEXT("Fern"),Point.Position+Right*Side*(450+J*150)+FVector(0,0,Roof-70),FVector(3,3,2.5),FRotator(180,Point.Yaw,0),FLinearColor(.18,.39,.10));
        }
        if (Section==1 || Section==6)
        {
            AddInstance(Key+TEXT("Pipe"),TEXT("Cylinder"),Mid+SpanRight*Side*975+FVector(0,0,630),FVector(.9,.9,Length/100),FRotator(90,SpanRot.Yaw,0),FLinearColor(.42,.22,.09));
            AddInstance(Key+TEXT("VentFrame"),TEXT("Cube"),Pillar-Right*Side*120+FVector(0,0,180),FVector(2,.12,1.6),Rot,FLinearColor(.1,.12,.13));
            for(int32 J=0;J<4;++J)
                AddInstance(Key+TEXT("VentSlat"),TEXT("Cube"),Pillar-Right*Side*130+FVector(0,0,125+J*35),FVector(1.8,.15,.1),Rot,Accent[Section],false,.7);
        }
        if (Section==4)
        {
            for(int32 J=0;J<3;++J)
                AddInstance(Key+TEXT("Crystal"),TEXT("Cone"),Pillar-Right*Side*160+Forward*(J*100-100)+FVector(0,0,150),FVector(1,1,3.2+J),FRotator(Side*15,Point.Yaw,18),Accent[Section]*.75f,false,.6);
        }
        if (Section==3 || Section==5 || Section==7)
        {
            AddInstance(Key+TEXT("WallPanel"),TEXT("Cube"),Pillar+Forward*390-Right*Side*20+FVector(0,0,450),FVector(4,.1,4.8),Rot,Palettes[Section]*1.3f);
            for(int32 J=-1;J<=1;++J)
                AddInstance(Key+TEXT("PanelInlay"),TEXT("Cube"),Pillar+Forward*(390+J*90)-Right*Side*30+FVector(0,0,450),FVector(.12,.12,3.4-FMath::Abs(J)),Rot,Accent[Section],false,.3);
        }
    }
    if(Section==2)
    {
        // The archive reads as timber shelves and coloured book spines, not masonry.
        for(int32 Side:{-1,1}) for(int32 Shelf=0;Shelf<3;++Shelf)
        {
            const FVector Stack=Point.Position+Right*Side*970+Forward*300+FVector(0,0,230+Shelf*130);
            AddInstance(Key+TEXT("Shelf"),TEXT("Cube"),Stack,FVector(5.5,1.1,.16),Rot,FLinearColor(.32,.17,.07));
            for(int32 Book=0;Book<5;++Book)
                AddInstance(Key+FString::Printf(TEXT("Book%d"),Book),TEXT("Cube"),Stack+Forward*(Book*85-170)+FVector(0,0,42),FVector(.5,.6,.72+(Book%2)*.18),Rot,
                    Book%3==0?FLinearColor(.46,.14,.08):Book%3==1?FLinearColor(.12,.35,.34):FLinearColor(.55,.41,.12));
        }
    }
    if(Section==4)
    {
        for(int32 Side:{-1,1})
        {
            AddInstance(Key+TEXT("CavernLip"),TEXT("WeatheredRock"),Point.Position+Right*Side*870+FVector(0,0,1150),FVector(5,6,6),Rot,Palettes[Section]*.8f);
            AddInstance(Key+TEXT("Stalactite"),TEXT("Cone"),Point.Position+Right*Side*450+FVector(0,0,1190),FVector(1.2,1.2,3.3),FRotator(180,Point.Yaw,0),Palettes[Section]*1.3f);
        }
    }
    if(Section==3 || Section==7)
    {
        for(int32 Side:{-1,1})
        {
            const FVector Banner=Point.Position+Right*Side*925+Forward*360+FVector(0,0,750);
            AddInstance(Key+TEXT("Banner"),TEXT("Cube"),Banner,FVector(3.3,.08,3.7),Rot,Section==7?FLinearColor(.53,.10,.09):FLinearColor(.25,.09,.42));
            AddInstance(Key+TEXT("BannerMark"),TEXT("Cube"),Banner-Right*Side*6,FVector(1.1,.04,1.1),FRotator(0,Point.Yaw,45),Accent[Section],false,.12);
        }
    }
    AddInstance(Key+TEXT("Roof"),TEXT("Cube"),FVector(Mid.X,Mid.Y,CeilingZ+130),FVector(Length/100,26,2.6),SpanRot,Palettes[Section]*.6f,true);
    AddInstance(Key+TEXT("CrossBeam"),TEXT("Cube"),Point.Position+FVector(0,0,Roof-30),FVector(1.3,23,1.6),Rot,Palettes[Section]*.85f);
    // A visible pit bed closes the interior below the jumping route too.
    // It is deliberately non-colliding: falling still returns to a checkpoint.
    AddInstance(Key+TEXT("PitBed"),TEXT("Cube"),Mid-FVector(0,0,1300),FVector(Length/100,26,2),SpanRot,Palettes[Section]*.28f);
    if (Section==5)
        AddInstance(Key+TEXT("Canal"),TEXT("Cube"),Mid-FVector(0,0,1150),FVector(Length/100,23,.1),SpanRot,FLinearColor(.035,.28,.36),false,.2);
    if (Local==0)
    {
        for(int32 Side:{-1,1})
            AddInstance(Key+TEXT("PortalTrim"),TEXT("Cube"),Point.Position-Forward*550+Right*Side*900+FVector(0,0,440),FVector(.5,.35,8.8),Rot,Accent[Section],false,.55);
        AddInstance(Key+TEXT("PortalLintel"),TEXT("Cube"),Point.Position-Forward*550+FVector(0,0,880),FVector(.5,18.4,.4),Rot,Accent[Section],false,.55);
    }
    if (Point.Index==0 || Point.Index==Route.Num()-1)
        AddInstance(Key+TEXT("EndWall"),TEXT("Cube"),Point.Position+Forward*(Point.Index==0?-920:1500)+FVector(0,0,300),FVector(2.4,26,30),Rot,Palettes[Section]*.7f,true);
}

const FPOCRoutePoint* APOCWorld::Nearest(FVector Position) const
{
    const FPOCRoutePoint* Result = nullptr;
    double Distance = TNumericLimits<double>::Max();
    for (const auto& Point : Route)
    {
        const double Current = FVector::DistSquared2D(Point.Position, Position);
        if (Current < Distance) { Distance = Current; Result = &Point; }
    }
    return Result;
}

void APOCWorld::ShowCaption(const FString& Text, float Duration)
{
    Caption = Text; CaptionUntil = Age + Duration;
}

void APOCWorld::Sound(FName Name, FVector Position, float Pitch)
{
    if (const auto* Wave = Sounds.Find(Name); Wave && *Wave)
        UGameplayStatics::PlaySoundAtLocation(this, *Wave, Position, GetGameInstance<UPOCGameInstance>()->Save->EffectsVolume * .6f, Pitch);
}

void APOCWorld::Burst(FVector Position, FLinearColor Color, int32 Count)
{
    for (int32 Index = 0; Index < Particles.Num() && Count > 0; ++Index)
        if (ParticleLife[Index] <= 0)
        {
            ParticleLife[Index] = FMath::FRandRange(.3, .65);
            ParticleVelocity[Index] = FMath::VRand() * FMath::FRandRange(80.f, 190.f) + FVector(0, 0, 85);
            Particles[Index]->SetWorldLocation(Position);
            Particles[Index]->SetVisibility(true);
            Cast<UMaterialInstanceDynamic>(Particles[Index]->GetMaterial(0))->SetVectorParameterValue(TEXT("Tint"), Color);
            --Count;
        }
}

bool APOCWorld::ActivateEcho(APOCCharacter* Character)
{
    for (const auto& Prop : Props)
    {
        const float Distance = FVector::Dist(Prop->GetActorLocation(), Character->GetActorLocation());
        if(Prop->Kind==EPOCProp::GateSwitch && Distance<200)
        {
            bool& Powered = Prop->Group==0 ? LeftSwitch : RightSwitch;
            auto* GI=GetGameInstance<UPOCGameInstance>();
            if(Powered) { ShowCaption(TEXT("This switch is already powered."),2); return true; }
            const int32 Available=GI->Shards-GI->ShardsSpent;
            if(!GI->TrySpendShards(SwitchPrice))
            { ShowCaption(FString::Printf(TEXT("Need %d more shards. Backtrack and collect them; your progress is safe."),SwitchPrice-Available),5); return false; }
            Powered=true;
            Burst(Prop->GetActorLocation()+FVector(0,0,70),FLinearColor(1,.6,.1),28); Sound(TEXT("Relic"),Prop->GetActorLocation());
            ShowCaption(GateIsOpen()?TEXT("Both seals are lit. The cake room is open!"):TEXT("One seal lit. Power the switch on the other side."),4);
            return true;
        }
        if (Prop->Kind == EPOCProp::Cake && Distance < 200)
        { Complete(Character, Prop->GetActorLocation()); return true; }
        if (Prop->Kind == EPOCProp::EchoNode && Distance < 450)
        {
            for (const auto& Bridge : Props) if (Bridge->Kind == EPOCProp::EchoBridge && Bridge->Group == Prop->Group) Bridge->Activate(18);
            Burst(Prop->GetActorLocation() + FVector(0, 0, 120), FLinearColor(.15, .9, 1), 30);
            Sound(TEXT("Echo"), Prop->GetActorLocation());
            ShowCaption(TEXT("The old road remembers. 18 seconds. Go!"), 3);
            return true;
        }
    }
    ShowCaption(TEXT("Echo stones hum when you are close."), 2);
    return false;
}

void APOCWorld::DropBomb(FVector Position)
{
    // Reuse a fixed enemy-bomb pool after detonation; no unbounded actor growth.
    for (const auto& Prop : Props)
        if (Prop->Kind == EPOCProp::Bomb && Prop->Group == -2 && Prop->IsHidden())
        { Prop->Origin = Position; Prop->ResetPlatform(); Prop->SetActorHiddenInGame(false); Prop->FuseRemaining = 1.5f; return; }
    int32 Count = 0;
    for (const auto& Prop : Props) if (Prop->Kind == EPOCProp::Bomb && Prop->Group == -2) ++Count;
    if (Count < 12) SpawnProp(EPOCProp::Bomb, Position, 0, FVector(1), 60000+Count, -2)->FuseRemaining = 1.5f;
}

void APOCWorld::ShowDream(bool Visible)
{
    for(const auto& Part:DreamParts) Part->SetVisibility(Visible);
}

void APOCWorld::AnimateDream(float Time)
{
    for (int32 I=0; I<DreamParts.Num(); ++I)
    {
        const float Delay=I<3 ? I*.14f : .5f;
        const float T=FMath::Clamp((Time-Delay)/.65f,0.f,1.f);
        const float Smooth=T*T*(3-2*T);
        DreamParts[I]->SetRelativeScale3D(DreamScales[I]*FMath::Max(.001f,Smooth));
        DreamParts[I]->SetRelativeLocation(DreamOrigins[I]+FVector(0,0,FMath::Sin(Time*1.6f)*2.f));
    }
}

void APOCWorld::Complete(APOCCharacter* Character, FVector CakePosition)
{
    if(!GateIsOpen()) { ShowCaption(TEXT("Two switches. 180 shards each. Dessert has a cover charge."),4); return; }
    Burst(CakePosition+FVector(0,0,150),FLinearColor(1,.45,.12),48);
    Character->EatCake(CakePosition);
    ShowCaption(TEXT("No ancient prophecy. Just vanilla."), 4);
    Sound(TEXT("Cake"), CakePosition);
}

void APOCWorld::ResetAtCheckpoint()
{
    for (const auto& Prop : Props) Prop->ResetPlatform();
    for (const auto& Enemy : Enemies) Enemy->ResetEnemy();
}

void APOCWorld::TeleportSection(int32 Section)
{
#if !UE_BUILD_SHIPPING
    if (!Player || !Route.IsValidIndex(Section * 24)) return;
    const auto& Point = Route[Section * 24];
    Player->Checkpoint = FTransform(FRotator(0, Point.Yaw, 0), Point.Position + FVector(0, 0, 52));
    Player->Respawn();
#endif
}

void APOCWorld::UpdateAtmosphere(float DeltaSeconds, int32 Section)
{
    static const FLinearColor FogColors[] = {FLinearColor(.4, .58, .62), FLinearColor(.22, .38, .48), FLinearColor(.1, .28, .3), FLinearColor(.13, .22, .35),
        FLinearColor(.035, .1, .22), FLinearColor(.11, .24, .42), FLinearColor(.24, .25, .38), FLinearColor(.62, .39, .27)};
    const auto Color = FMath::Lerp(Fog->GetComponent()->FogInscatteringLuminance, FogColors[Section], FMath::Clamp(DeltaSeconds * .4f, 0.f, 1.f));
    Fog->GetComponent()->SetFogInscatteringColor(Color);
    const float Intensity = Section == 4 ? 2.2f : 2.8f;
    Sun->GetLightComponent()->SetIntensity(FMath::FInterpTo(Sun->GetLightComponent()->Intensity, Intensity, DeltaSeconds, .5));
    Sun->GetLightComponent()->SetLightColor(Section == 1 || Section == 7 ? FLinearColor(1,.79,.6) : Section == 4 || Section == 5 ? FLinearColor(.7,.88,1) : FLinearColor(1,.95,.83));
}

void APOCWorld::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Ready) return;
    Age += DeltaSeconds;
    if (!Player)
    {
        Player = Cast<APOCCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
        if (!Player) return;
        Player->Journey = this;
        Player->Checkpoint = FTransform(FRotator(0, Route[0].Yaw, 0), Route[0].Position + FVector(0, 0, 52));
        Player->Respawn();
    }
    auto* GI = GetGameInstance<UPOCGameInstance>();
    if (!Player->Eating && !Player->Dreaming) GI->RunSeconds += DeltaSeconds;
    if (Age > CaptionUntil) Caption.Empty();
    SectionCheck -= DeltaSeconds;
    if (SectionCheck <= 0)
    {
        SectionCheck = .15f;
        Prompt.Empty();
        const auto* Point = Nearest(Player->GetActorLocation());
        GI->Section = Point ? Point->Section : 0;
        SectionName = SectionNames[GI->Section];
        if(Point && !Player->DeveloperFlight && Point->Index>=3 && Point->Index<=7)
            Prompt=Point->Index<5?TEXT("At the teal edge: move forward + SPACE to jump") : TEXT("Hold SPACE for height. Release, then press again for a double jump.");
        for (const auto& Prop : Props)
            Prop->SetActorTickEnabled(!Prop->Taken && (Prop->ActiveRemaining > 0 || FVector::DistSquared2D(Prop->Origin, Player->GetActorLocation()) < FMath::Square(8000.f)));
        for (const auto& Enemy : Enemies)
            if (!Enemy->IsHidden()) Enemy->SetActorTickEnabled(FVector::DistSquared2D(Enemy->GetActorLocation(), Player->GetActorLocation()) < FMath::Square(6000.f));
        if (PreviousSection != GI->Section)
        {
            PreviousSection = GI->Section;
            ShowCaption(SectionCaptions[GI->Section], 6);
            if (auto* Wave = Tracks.IsValidIndex(GI->Section) ? Tracks[GI->Section].Get() : nullptr)
            { Music->SetSound(Wave); Music->FadeIn(1.8, GI->Save->MusicVolume * .45f); }
        }
    }
    Music->SetVolumeMultiplier(GI->Save->MusicVolume * .45f);
    UpdateAtmosphere(DeltaSeconds, GI->Section);
    for (int32 Index = 0; Index < Particles.Num(); ++Index)
    {
        if (ParticleLife[Index] <= 0) continue;
        ParticleLife[Index] -= DeltaSeconds;
        ParticleVelocity[Index].Z -= 350 * DeltaSeconds;
        Particles[Index]->AddWorldOffset(ParticleVelocity[Index] * DeltaSeconds);
        Particles[Index]->SetWorldScale3D(FVector(FMath::Max(.001f, ParticleLife[Index] * .09f)));
        if (ParticleLife[Index] <= 0) Particles[Index]->SetVisibility(false);
    }
}
