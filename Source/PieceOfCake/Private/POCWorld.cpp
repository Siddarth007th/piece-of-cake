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
#include "Engine/SkyAtmosphere.h"
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
    const TCHAR* SectionNames[] = { TEXT("THE MEADOW"), TEXT("FORGOTTEN RUINS"), TEXT("ANCIENT FOREST"), TEXT("THE LISTENING TEMPLE"),
        TEXT("UNDERGROUND CAVERNS"), TEXT("THE SUNKEN CITY"), TEXT("THE LONG WAY DOWN"), TEXT("ONE LAST CLIMB") };
    const TCHAR* SectionCaptions[] = {
        TEXT("There it is. Just up there. How hard could it be?"),
        TEXT("Apparently, the direct route is under renovation."),
        TEXT("Small feet. Very large trees."),
        TEXT("Some places remember the way. Press E near an Echo stone."),
        TEXT("Definitely not the kitchen."),
        TEXT("An entire civilization. Not one elevator."),
        TEXT("That sound is probably fine. Hold Shift / LB to sprint!"),
        TEXT("Almost there. For real, this time.") };
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
    Palettes = {FLinearColor(.36, .43, .27), FLinearColor(.22, .32, .37), FLinearColor(.11, .28, .23), FLinearColor(.17, .25, .34),
        FLinearColor(.085, .15, .26), FLinearColor(.12, .22, .38), FLinearColor(.23, .24, .34), FLinearColor(.43, .31, .22)};
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
        Instances->SetMaterial(0, POCVisuals::Material(this, Color, Glow));
        Instances->SetCollisionEnabled(Collision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        Instances->SetCollisionResponseToAllChannels(ECR_Block);
        Instances->SetCullDistances(35000, 65000);
        Instances->SetCastShadow(Glow < 1);
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
            AddInstance(Prefix + TEXT("Stone"), TEXT("Cube"), Point.Position - FVector(0, 0, 100), Point.Size / 100, Rotation, Palettes[Section], true);
            AddInstance(Prefix + TEXT("Top"), TEXT("Cube"), Point.Position - FVector(0, 0, 3), FVector(Point.Size.X / 100 - .2, Point.Size.Y / 100 - .2, .06), Rotation, Palettes[Section] * 1.25f);
        }
        else SpawnProp(Point.Kind == TEXT("moving") ? EPOCProp::Moving : Point.Kind == TEXT("echo") ? EPOCProp::EchoBridge : EPOCProp::Crumble,
            Point.Position, Point.Yaw, Point.Size, 10000 + Point.Index, Point.Group);

        for (int32 J = -1; J <= 1; ++J)
            SpawnProp(EPOCProp::Shard, Point.Position + Forward * (J * 240) + FVector(0, 0, 80), 0, FVector(1), Point.Index * 3 + J + 1);
        if (Point.Checkpoint)
            SpawnProp(EPOCProp::Checkpoint, Point.Position - Forward * 330, Point.Yaw, FVector(1), Point.Index);
        if (Point.EchoNode >= 0)
            SpawnProp(EPOCProp::EchoNode, Point.Position + Forward * 390 + Right * 210, Point.Yaw, FVector(1), 30000 + Point.Index, Point.EchoNode);
        if (Point.Enemy >= 0)
        {
            const int32 Species = Point.Enemy;
            const FTransform Transform(Rotation, Point.Position + FVector(0, 0, Species == 2 ? 130 : 50));
            auto* Enemy = GetWorld()->SpawnActorDeferred<APOCEnemy>(EnemyClass, Transform, this);
            Enemy->Journey = this; Enemy->Species = Species;
            UGameplayStatics::FinishSpawningActor(Enemy, Transform); Enemies.Add(Enemy);
        }
        if (Point.Hazard)
            SpawnProp(EPOCProp::Hazard, Point.Position, Point.Yaw, FVector(1), 40000 + Point.Index);
        if (Point.SlideGate)
            SpawnProp(EPOCProp::SlideGate, Point.Position, Point.Yaw, FVector(1), 41000 + Point.Index);
        if (Point.Bounce)
            SpawnProp(EPOCProp::Bounce, Point.Position + Forward * 300 + Right * 100, Point.Yaw, FVector(1), 42000 + Point.Index);
        if (Point.Secret)
        {
            const FVector Secret = Point.Position + Right * 900 + FVector(0, 0, 60);
            AddInstance(Prefix + TEXT("Secret"), TEXT("Cube"), Secret - FVector(0, 0, 75), FVector(5, 5, 1.5), Rotation, Palettes[Section], true);
            SpawnProp(EPOCProp::Relic, Secret + FVector(0, 0, 90), 0, FVector(1), 50000 + Section);
            AddInstance(TEXT("SecretMarker"), TEXT("Cone"), Point.Position + Right * 410 + FVector(0, 0, 40), FVector(.18, .18, .8), Rotation, FLinearColor(.85, .64, .22), false, .4);
        }
        AddScenery(Point);
        // Inlays indicate forward travel without a floating waypoint over every jump.
        AddInstance(TEXT("Waymark"), TEXT("Cone"), Point.Position + Forward * 510 + FVector(0, 0, 6), FVector(.5, .5, .1), FRotator(90, Point.Yaw, 0), FLinearColor(.74, .58, .23), false, .2);
    }
    const auto& Last = Route.Last();
    const FRotator FinalFacing(0, Last.Yaw, 0);
    const FVector FinalForward = FinalFacing.Vector();
    const FVector FinalRight = FRotationMatrix(FinalFacing).GetUnitAxis(EAxis::Y);
    const FVector CakePosition = Last.Position + FinalForward * 310;
    SpawnProp(EPOCProp::Cake, CakePosition, Last.Yaw, FVector(1), 90000);
    // The title camera establishes the ordinary reward before play starts. It
    // blends back to Nori when Begin is selected; traversal stays player-controlled.
    const FVector IntroPosition = CakePosition - FinalForward * 520 + FinalRight * 310 + FVector(0, 0, 205);
    IntroCamera = GetWorld()->SpawnActor<ACameraActor>(IntroPosition, (CakePosition - FinalRight * 120 + FVector(0, 0, 70) - IntroPosition).Rotation());
    IntroCamera->GetCameraComponent()->SetFieldOfView(48);
    Sun = GetWorld()->SpawnActor<ADirectionalLight>();
    Sun->SetActorRotation(FRotator(-32, -30, 0));
    Sun->GetLightComponent()->SetIntensity(3.8f);
    Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->bAtmosphereSunLight = true;
    auto* Sky = GetWorld()->SpawnActor<ASkyLight>();
    Sky->GetLightComponent()->SetIntensity(.7f);
    Sky->GetLightComponent()->SetRealTimeCaptureEnabled(true);
    GetWorld()->SpawnActor<ASkyAtmosphere>();
    Fog = GetWorld()->SpawnActor<AExponentialHeightFog>();
    Fog->GetComponent()->SetFogDensity(.009f);
    Fog->GetComponent()->SetFogHeightFalloff(.12f);
    Fog->GetComponent()->SetStartDistance(1200);
    Fog->GetComponent()->SetFogMaxOpacity(.9);
    auto* Post = GetWorld()->SpawnActor<APostProcessVolume>();
    Post->bUnbound = true;
    Post->Settings.bOverride_BloomIntensity = true; Post->Settings.BloomIntensity = .4;
    Post->Settings.bOverride_VignetteIntensity = true; Post->Settings.VignetteIntensity = .23;
    Post->Settings.bOverride_AutoExposureBias = true; Post->Settings.AutoExposureBias = .4;
    AddInstance(TEXT("BasinWater"), TEXT("Cylinder"), FVector(-48000, 0, -2500), FVector(1000, 1000, 1), FRotator::ZeroRotator, FLinearColor(.08, .2, .28), false, .05);
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
    FRandomStream Random(2718 + Point.Index * 71);
    const FRotator Rot(0, Point.Yaw, 0);
    const FVector Right = FRotationMatrix(Rot).GetUnitAxis(EAxis::Y), Forward = Rot.Vector();
    const FString Key = FString::FromInt(Section);
    for (int32 Side : {-1, 1})
    {
        const FVector Edge = Point.Position + Right * Side * (Point.Size.Y * .5 + 300);
        if (Point.Kind == TEXT("ground"))
            AddInstance(Key + TEXT("Cliff"), TEXT("Cone"), Edge - FVector(0, 0, 1100), FVector(14, 13, 26), FRotator(180, Point.Yaw, 0), Palettes[Section] * .65f);
        if (Section == 0 || Section == 2)
        {
            const float Height = Section == 2 ? Random.FRandRange(2200, 4300) : Random.FRandRange(650, 1400);
            const FVector Tree = Edge + Right * Side * Random.FRandRange(150, 800);
            AddInstance(Key + TEXT("Trunk"), TEXT("Cylinder"), Tree + FVector(0, 0, Height * .4), FVector(Section == 2 ? 2.5 : .8, Section == 2 ? 2.5 : .8, Height / 100), FRotator(0, 0, Side * 5), FLinearColor(.18, .23, .19));
            for (int32 Crown = 0; Crown < 3; ++Crown)
                AddInstance(Key + TEXT("Canopy"), TEXT("Sphere"), Tree + FVector(Random.FRandRange(-220, 220), Random.FRandRange(-220, 220), Height * .75 + Crown * 160),
                    FVector(10 + Crown * 2, 12, 6) * (Section == 2 ? 1.8 : .8), Rot, Section == 2 ? FLinearColor(.09, .25, .22) : FLinearColor(.22, .37, .2));
        }
        if (Section == 1 || Section == 3 || Section == 5 || Section == 7)
        {
            const float Height = Section == 5 ? Random.FRandRange(5000, 15000) : Random.FRandRange(1500, 3300);
            AddInstance(Key + TEXT("Pillar"), TEXT("Cube"), Edge + FVector(0, 0, Height * .5), FVector(2, 2.5, Height / 100), Rot, Palettes[Section] * .8f);
            AddInstance(Key + TEXT("Capital"), TEXT("Cube"), Edge + FVector(0, 0, Height), FVector(3.5, 4, .7), Rot, Palettes[Section] * 1.3f);
            AddInstance(Key + TEXT("Lantern"), TEXT("Sphere"), Edge + FVector(0, 0, 240), FVector(.4), Rot, FLinearColor(1, .55, .16), false, 3);
            if (Local % 4 == 0)
                AddInstance(Key + TEXT("Arch"), TEXT("Cube"), Point.Position + FVector(0, 0, 1250), FVector(3, 20, 1.8), Rot, Palettes[Section]);
        }
        if (Section == 4)
        {
            AddInstance(TEXT("CaveRibs"), TEXT("Sphere"), Edge + FVector(0, 0, 1300), FVector(12, 12, 30), Rot, Palettes[4] * .6f);
            AddInstance(TEXT("CaveRoof"), TEXT("Sphere"), Point.Position + FVector(0, 0, 2800), FVector(20, 28, 10), Rot, Palettes[4] * .55f);
            for (int32 J = 0; J < 4; ++J)
                AddInstance(TEXT("Crystal"), TEXT("Cone"), Edge + Forward * (J * 140 - 200) + FVector(0, 0, 100), FVector(.7, .9, Random.FRandRange(2, 5)), FRotator(Side * 12, 0, 20), FLinearColor(.05, .45, .7), false, .8);
        }
        for (int32 J = 0; J < 8; ++J)
        {
            const FVector P = Point.Position + Forward * Random.FRandRange(-570, 570) + Right * Side * Random.FRandRange(330, Point.Size.Y * .5 - 25);
            if (Point.Kind == TEXT("ground"))
                AddInstance(Key + TEXT("Grass"), TEXT("Cone"), P + FVector(0, 0, 18), FVector(.11, .11, Random.FRandRange(.3, .7)), Rot, Section == 4 ? FLinearColor(.1, .5, .55) : FLinearColor(.22, .36, .26), false, Section == 4 ? .5 : 0);
        }
    }
    if (Local % 6 == 0)
    {
        const FVector Mountain = Point.Position + Right * 14000;
        AddInstance(TEXT("DistantMass"), TEXT("Cone"), Mountain + FVector(0, 0, -2000), FVector(190, 210, 220), Rot, FLinearColor(.16, .22, .3));
        AddInstance(TEXT("Falls"), TEXT("Cube"), Point.Position + Right * -1800 + FVector(0, 0, -1200), FVector(1.5, .4, 35), Rot, FLinearColor(.26, .57, .63), false, .25);
    }
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
    const FString Asset = FString::Printf(TEXT("/Game/Audio/A_%s.A_%s"), *Name.ToString(), *Name.ToString());
    if (auto* Wave = LoadObject<USoundWave>(nullptr, *Asset))
        UGameplayStatics::PlaySoundAtLocation(this, Wave, Position, GetGameInstance<UPOCGameInstance>()->Save->EffectsVolume * .6f, Pitch);
}

void APOCWorld::Burst(FVector Position, FLinearColor Color, int32 Count)
{
    for (int32 Index = 0; Index < Particles.Num() && Count > 0; ++Index)
        if (ParticleLife[Index] <= 0)
        {
            ParticleLife[Index] = FMath::FRandRange(.3, .65);
            ParticleVelocity[Index] = FMath::VRand() * FMath::FRandRange(80, 190) + FVector(0, 0, 85);
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

void APOCWorld::Complete(APOCCharacter* Character, FVector CakePosition)
{
    Character->EatCake(CakePosition);
    ShowCaption(TEXT("No ancient prophecy. Just vanilla."), 4);
    Sound(TEXT("Cake"), CakePosition);
}

void APOCWorld::ResetAtCheckpoint()
{
    for (const auto& Prop : Props) Prop->ResetPlatform();
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
    const auto Color = FMath::Lerp(Fog->GetComponent()->FogInscatteringColor, FogColors[Section], FMath::Clamp(DeltaSeconds * .4f, 0.f, 1.f));
    Fog->GetComponent()->SetFogInscatteringColor(Color);
    const float Intensity = Section == 4 ? .65 : Section == 2 || Section == 3 ? 1.7 : 3.8;
    Sun->GetLightComponent()->SetIntensity(FMath::FInterpTo(Sun->GetLightComponent()->Intensity, Intensity, DeltaSeconds, .5));
    Sun->GetLightComponent()->SetLightColor(Section == 7 ? FLinearColor(1, .62, .37) : FLinearColor(1, .9, .72));
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
    if (!Player->Eating) GI->RunSeconds += DeltaSeconds;
    if (Age > CaptionUntil) Caption.Empty();
    SectionCheck -= DeltaSeconds;
    if (SectionCheck <= 0)
    {
        SectionCheck = .15f;
        Prompt.Empty();
        const auto* Point = Nearest(Player->GetActorLocation());
        GI->Section = Point ? Point->Section : 0;
        SectionName = SectionNames[GI->Section];
        for (const auto& Prop : Props)
            Prop->SetActorTickEnabled(!Prop->Taken && (Prop->ActiveRemaining > 0 || FVector::DistSquared2D(Prop->Origin, Player->GetActorLocation()) < FMath::Square(8000.f)));
        for (const auto& Enemy : Enemies)
            if (!Enemy->IsHidden()) Enemy->SetActorTickEnabled(FVector::DistSquared2D(Enemy->GetActorLocation(), Player->GetActorLocation()) < FMath::Square(6000.f));
        if (PreviousSection != GI->Section)
        {
            PreviousSection = GI->Section;
            ShowCaption(SectionCaptions[GI->Section], 6);
            const FString Asset = FString::Printf(TEXT("/Game/Audio/M_%d.M_%d"), GI->Section, GI->Section);
            if (auto* Wave = LoadObject<USoundWave>(nullptr, *Asset))
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
