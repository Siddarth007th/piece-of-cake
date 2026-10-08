#include "POCCharacter.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "POCWorld.h"
#include "POCGameInstance.h"
#include "POCVisuals.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/World.h"

APOCCharacter::APOCCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(25.f, 44.f);
    GetMesh()->SetHiddenInGame(true);
    bUseControllerRotationYaw = false;
    auto* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = true;
    Movement->RotationRate = FRotator(0, 1100, 0);
    Movement->MaxWalkSpeed = RunSpeed;
    Movement->MaxAcceleration = 5200;
    Movement->BrakingDecelerationWalking = 2800;
    Movement->GroundFriction = 7;
    Movement->AirControl = .95f;
    Movement->JumpZVelocity = 650;
    Movement->GravityScale = 1.6f;
    Movement->MaxStepHeight = 45;
    Movement->SetWalkableFloorAngle(46.f);
    Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
    Movement->SetCrouchedHalfHeight(23.f);
    Movement->MaxWalkSpeedCrouched = 330;
    Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("FollowBoom"));
    Boom->SetupAttachment(RootComponent);
    Boom->TargetArmLength = 420;
    Boom->TargetOffset = FVector(0, 0, 75);
    Boom->bUsePawnControlRotation = true;
    Boom->bEnableCameraLag = true;
    Boom->CameraLagSpeed = 10;
    Boom->CameraLagMaxDistance = 45;
    Boom->bEnableCameraRotationLag = false;
    Boom->CameraRotationLagSpeed = 16;
    Boom->ProbeSize = 18;
    Boom->bDoCollisionTest = true;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("AdventureCamera"));
    Camera->SetupAttachment(Boom);
    Camera->FieldOfView = 82;
    VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Nori"));
    VisualRoot->SetupAttachment(RootComponent);
}

void APOCCharacter::BeginPlay()
{
    Super::BeginPlay();
    const FLinearColor Cream(.87f, .82f, .66f), Teal(.11f, .36f, .36f), Ink(.013f, .024f, .035f), Red(.65f, .095f, .065f);
    Body = POCVisuals::Part(this, VisualRoot, TEXT("Body"), TEXT("NoriBody"), FVector(0, 0, -5), FVector(.51, .44, .55), FLinearColor(.055,.095,.17));
    const FLinearColor Muzzle(.99, .89, .66), Amber(.92, .47, .055);
    HeadRoot = NewObject<USceneComponent>(this, TEXT("HeadRig"));
    AddInstanceComponent(HeadRoot); HeadRoot->SetupAttachment(VisualRoot);
    HeadRoot->SetRelativeLocation(FVector(7, 0, 25)); HeadRoot->RegisterComponent();
    Head = POCVisuals::Part(this, HeadRoot, TEXT("Head"), TEXT("NoriHead"), FVector::ZeroVector, FVector(.55, .52, .46), Cream);
    POCVisuals::Part(this, VisualRoot, TEXT("Belly"), TEXT("Sphere"), FVector(20, 0, -3), FVector(.11, .31, .29), Teal);
    POCVisuals::Part(this, HeadRoot, TEXT("Muzzle"), TEXT("Sphere"), FVector(23, 0, -4), FVector(.21, .3, .19), Muzzle);
    POCVisuals::Part(this, HeadRoot, TEXT("Nose"), TEXT("Sphere"), FVector(33, 0, -1), FVector(.07, .1, .065), Ink);
    // Light cloth ninja outfit keeps Nori's face, paws and rabbit ears readable.
    POCVisuals::Part(this, VisualRoot, TEXT("WaistSash"), TEXT("Sphere"), FVector(0,0,-15), FVector(.52,.46,.105), Red);
    auto* Scroll=POCVisuals::Part(this,VisualRoot,TEXT("CakeScroll"),TEXT("Cylinder"),FVector(-26,0,-1),FVector(.15,.15,.43),Muzzle);
    Scroll->SetRelativeRotation(FRotator(0,0,90));
    POCVisuals::Part(this,VisualRoot,TEXT("ScrollTie"),TEXT("Sphere"),FVector(-26,0,-1),FVector(.17,.08,.17),Red);
    POCVisuals::Part(this,HeadRoot,TEXT("NinjaHeadband"),TEXT("Sphere"),FVector(-1,0,16),FVector(.565,.535,.095),Red);
    auto* Crest=POCVisuals::Part(this,HeadRoot,TEXT("HeadbandCrest"),TEXT("Cube"),FVector(26,0,16),FVector(.025,.075,.075),Amber);
    Crest->SetRelativeRotation(FRotator(0,0,45));
    for(int32 I=0;I<2;++I)
        HeadbandTails.Add(POCVisuals::Part(this,HeadRoot,*FString::Printf(TEXT("HeadbandTail%d"),I),TEXT("NoriScarf"),FVector(-31-I*7,I?8:-8,15),FVector(.28,.075,.025),Red));
    POCVisuals::Part(this, VisualRoot, TEXT("ScarfCollar"), TEXT("Sphere"), FVector(2, 0, 10), FVector(.51, .48, .12), Red);
    for (int32 Side = -1; Side <= 1; Side += 2)
    {
        auto* EarRoot = NewObject<USceneComponent>(this, *FString::Printf(TEXT("EarRig%d"), Side));
        AddInstanceComponent(EarRoot); EarRoot->SetupAttachment(HeadRoot);
        EarRoot->SetRelativeLocation(FVector(-7, Side * 16, 35)); EarRoot->RegisterComponent();
        EarRoots.Add(EarRoot);
        auto* Ear = POCVisuals::Part(this, EarRoot, *FString::Printf(TEXT("Ear%d"), Side), TEXT("NoriEar"), FVector::ZeroVector, FVector(.16, .24, .55), Cream);
        Ear->SetMaterial(1, POCVisuals::Material(this, Teal));
        Ears.Add(Ear);
        POCVisuals::Part(this, HeadRoot, *FString::Printf(TEXT("EyeRim%d"), Side), TEXT("Sphere"), FVector(22, Side * 14, 5), FVector(.10, .185, .23), Muzzle);
        Eyes.Add(POCVisuals::Part(this, HeadRoot, *FString::Printf(TEXT("Eye%d"), Side), TEXT("Sphere"), FVector(26, Side * 14, 5), FVector(.07, .135, .175), Ink));
        EyeDetails.Add(POCVisuals::Part(this, HeadRoot, *FString::Printf(TEXT("Iris%d"), Side), TEXT("Sphere"), FVector(29, Side * 14, 5), FVector(.035, .082, .115), Amber));
        EyeDetails.Add(POCVisuals::Part(this, HeadRoot, *FString::Printf(TEXT("Pupil%d"), Side), TEXT("Sphere"), FVector(30, Side * 14, 5), FVector(.02, .044, .085), Ink));
        EyeDetails.Add(POCVisuals::Part(this, HeadRoot, *FString::Printf(TEXT("Glint%d"), Side), TEXT("Sphere"), FVector(31, Side * 14 - 2, 8), FVector(.015, .027, .033), FLinearColor::White, .1f));
        Feet.Add(POCVisuals::Part(this, VisualRoot, *FString::Printf(TEXT("Foot%d"), Side), TEXT("NoriPaw"), FVector(6, Side * 15, -34), FVector(.29, .20, .16), Teal));
        Paws.Add(POCVisuals::Part(this, VisualRoot, *FString::Printf(TEXT("Paw%d"), Side), TEXT("NoriPaw"), FVector(13, Side * 24, -3), FVector(.17, .14, .21), Cream));
        POCVisuals::Part(this,Paws.Last(),*FString::Printf(TEXT("WristWrap%d"),Side),TEXT("Sphere"),FVector(0,0,23),FVector(1.04,1.04,.3),Teal);
        POCVisuals::Part(this,Feet.Last(),*FString::Printf(TEXT("FootWrap%d"),Side),TEXT("Sphere"),FVector(0,0,28),FVector(.55,1.03,.25),Muzzle);
    }
    for (int32 Index = 0; Index < 6; ++Index)
        Scarf.Add(POCVisuals::Part(this, VisualRoot, *FString::Printf(TEXT("Scarf%d"), Index), TEXT("NoriScarf"), FVector(-22 - Index * 9, 5, 10), FVector(.14, .14 - Index * .009, .025), Red));
    // Soft fabric/body surfaces contrast with glossy eyes; the world remains matte.
    for (const auto& Eye : Eyes)
        if (auto* Material = Cast<UMaterialInstanceDynamic>(Eye->GetMaterial(0))) Material->SetScalarParameterValue(TEXT("Roughness"), .22f);
    for (const auto& Detail : EyeDetails)
        if (auto* Material = Cast<UMaterialInstanceDynamic>(Detail->GetMaterial(0))) Material->SetScalarParameterValue(TEXT("Roughness"), .28f);
    Checkpoint = GetActorTransform();
}

void APOCCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"), this, &APOCCharacter::MoveForward);
    Input->BindAxis(TEXT("MoveRight"), this, &APOCCharacter::MoveRight);
    Input->BindAction(TEXT("CameraToggle"), IE_Pressed, this, &APOCCharacter::ToggleCameraLook);
    Input->BindAction(TEXT("CameraCenter"), IE_Pressed, this, &APOCCharacter::CenterCamera);
    Input->BindAxis(TEXT("LookYawKeys"), this, &APOCCharacter::LookYawKeys);
    Input->BindAxis(TEXT("LookPitchKeys"), this, &APOCCharacter::LookPitchKeys);
    Input->BindAction(TEXT("CameraLook"), IE_Pressed, this, &APOCCharacter::CameraLookPressed);
    Input->BindAction(TEXT("CameraLook"), IE_Released, this, &APOCCharacter::CameraLookReleased);
    Input->BindAxis(TEXT("LookYaw"), this, &APOCCharacter::LookYaw);
    Input->BindAxis(TEXT("LookPitch"), this, &APOCCharacter::LookPitch);
    Input->BindAxis(TEXT("LookYawPad"), this, &APOCCharacter::LookYawPad);
    Input->BindAxis(TEXT("LookPitchPad"), this, &APOCCharacter::LookPitchPad);
    Input->BindAction(TEXT("Jump"), IE_Pressed, this, &APOCCharacter::JumpPressed);
    Input->BindAction(TEXT("Jump"), IE_Released, this, &APOCCharacter::JumpReleased);
    Input->BindAction(TEXT("Dash"), IE_Pressed, this, &APOCCharacter::Dash);
    Input->BindAction(TEXT("Bonk"), IE_Pressed, this, &APOCCharacter::Bonk);
    Input->BindAction(TEXT("Echo"), IE_Pressed, this, &APOCCharacter::Echo);
    Input->BindAction(TEXT("Sprint"), IE_Pressed, this, &APOCCharacter::SprintPressed);
    Input->BindAction(TEXT("Sprint"), IE_Released, this, &APOCCharacter::SprintReleased);
    Input->BindAction(TEXT("Slide"), IE_Pressed, this, &APOCCharacter::SlidePressed);
    Input->BindAction(TEXT("Slide"), IE_Released, this, &APOCCharacter::SlideReleased);
    Input->BindAction(TEXT("RestartCheckpoint"), IE_Pressed, this, &APOCCharacter::Respawn);
}

void APOCCharacter::MoveForward(float Value)
{
    if (!Controller || Eating || Dreaming || Dying || FMath::IsNearlyZero(Value)) return;
    AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X), Value);
}
void APOCCharacter::MoveRight(float Value)
{
    if (!Controller || Eating || Dreaming || Dying || FMath::IsNearlyZero(Value)) return;
    AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y), Value);
}
void APOCCharacter::CameraLookPressed() { CameraLookHeld=true; }
void APOCCharacter::CameraLookReleased() { CameraLookHeld=false; }
void APOCCharacter::ToggleCameraLook()
{
    if(Eating || Dreaming || Dying || UGameplayStatics::IsGamePaused(this)) return;
    CameraLookToggled=!CameraLookToggled;
    CameraRecentering=false;
    if(Journey) Journey->ShowCaption(CameraLookToggled ? TEXT("Mouse camera on. Move the mouse; tap F again to lock the view.") : TEXT("Camera locked. Arrow keys adjust it; X centers behind Nori."),3);
}
void APOCCharacter::CenterCamera()
{
    if(!Controller || Eating || Dreaming || Dying || UGameplayStatics::IsGamePaused(this)) return;
    CameraLookToggled=false;
    CameraCenterYaw=GetActorRotation().Yaw;
    CameraRecentering=true;
}
void APOCCharacter::ApplyCameraDelta(float Yaw,float Pitch)
{
    if(!Controller || Eating || Dreaming || Dying || UGameplayStatics::IsGamePaused(this) || (FMath::IsNearlyZero(Yaw) && FMath::IsNearlyZero(Pitch))) return;
    CameraRecentering=false;
    auto* GI=GetGameInstance<UPOCGameInstance>();
    const float Sensitivity=GI&&GI->Save ? GI->Save->Sensitivity : 1.f;
    const float Invert=GI&&GI->Save&&GI->Save->InvertY ? -1.f : 1.f;
    FRotator View=Controller->GetControlRotation();
    View.Yaw=FRotator::NormalizeAxis(View.Yaw+Yaw*Sensitivity);
    View.Pitch=FMath::Clamp(FRotator::NormalizeAxis(View.Pitch)+Pitch*Sensitivity*Invert,-65.f,28.f);
    View.Roll=0;
    // Explicit degrees avoid the legacy controller scale changing mouse/stick feel.
    Controller->SetControlRotation(View);
}
void APOCCharacter::LookYaw(float Value)
{
    if(IsMouseCameraActive()) ApplyCameraDelta(FMath::Clamp(Value,-20.f,20.f)*3.f,0);
}
void APOCCharacter::LookPitch(float Value)
{
    if(IsMouseCameraActive()) ApplyCameraDelta(0,FMath::Clamp(Value,-20.f,20.f)*3.f);
}
void APOCCharacter::LookYawPad(float Value) { ApplyCameraDelta(Value*120.f*FMath::Min(GetWorld()->GetDeltaSeconds(),.05f),0); }
void APOCCharacter::LookPitchPad(float Value) { ApplyCameraDelta(0,Value*85.f*FMath::Min(GetWorld()->GetDeltaSeconds(),.05f)); }
void APOCCharacter::LookYawKeys(float Value) { ApplyCameraDelta(Value*110.f*FMath::Min(GetWorld()->GetDeltaSeconds(),.05f),0); }
void APOCCharacter::LookPitchKeys(float Value)
{
    // Arrow up always looks up, regardless of the mouse/stick invert preference.
    auto* GI=GetGameInstance<UPOCGameInstance>();
    const float Invert=GI&&GI->Save&&GI->Save->InvertY ? -1.f : 1.f;
    ApplyCameraDelta(0,Value*75.f*FMath::Min(GetWorld()->GetDeltaSeconds(),.05f)*Invert);
}
void APOCCharacter::SetDeveloperFlight(bool Enabled)
{
    if (DeveloperFlight == Enabled || Eating || Dreaming || Dying) return;
    DeveloperFlight = Enabled;
    if(Enabled) GetGameInstance<UPOCGameInstance>()->AssistedRun=true;
    auto* Movement = GetCharacterMovement();
    ResetHeldInput(); UnCrouch();
    Movement->StopMovementImmediately(); Movement->ClearAccumulatedForces();
    ConsumeMovementInputVector();
    AttackRemaining = SlideRemaining = DashRemaining = DashCooldown = 0;
    Slamming = StrikePending = JumpUsed = DoubleJumpUsed = AirDashUsed = AirBonkUsed = AirJumpRequested = false;
    AirTime = 0;
    SetActorEnableCollision(!Enabled);
    Boom->bDoCollisionTest = !Enabled;
    Movement->GravityScale = Enabled ? 0.f : 1.6f;
    Movement->bCheatFlying = Enabled;
    Movement->MaxFlySpeed = 1400;
    Movement->BrakingDecelerationFlying = 8000;
    Movement->SetMovementMode(Enabled ? MOVE_Flying : MOVE_Walking);
    if (!Enabled) Respawn(); // Always exit on known safe ground, never inside a wall.
    if (Journey) Journey->ShowCaption(Enabled ? TEXT("Developer flight enabled. Progress collection is paused.") : TEXT("Developer flight off. Back at your checkpoint."),3);
    UE_LOG(LogTemp,Display,TEXT("POC_DEV_FLIGHT enabled=%d collision=%d xyz=%s"),Enabled,GetActorEnableCollision(),*GetActorLocation().ToCompactString());
}

void APOCCharacter::ResetHeldInput() { SprintHeld = JumpHeld = CameraLookHeld = CameraLookToggled = CameraRecentering = FlightDownHeld = JumpCutPending = false; JumpQueuedUntil = -100; }
void APOCCharacter::SprintPressed() { SprintHeld = true; }
void APOCCharacter::SprintReleased() { SprintHeld = false; }
void APOCCharacter::JumpPressed()
{
    if (Eating || Dreaming || Dying || JumpHeld) return;
    if (DeveloperFlight) { JumpHeld = true; return; }
    AirJumpRequested = GetCharacterMovement()->IsFalling();
    JumpHeld = true;
    JumpQueuedUntil = GetWorld()->GetTimeSeconds() + BufferSeconds;
    TryBufferedJump();
}
void APOCCharacter::JumpReleased()
{
    JumpHeld = false;
    if (!DeveloperFlight) JumpCutPending = true;
}
void APOCCharacter::TryBufferedJump()
{
    const float Now = GetWorld()->GetTimeSeconds();
    if (DeveloperFlight || Eating || Dreaming || Dying || JumpQueuedUntil < Now) return;
    const bool FirstJump = !JumpUsed && (GetCharacterMovement()->IsMovingOnGround() || Now - LastGroundTime <= CoyoteSeconds);
    if (!FirstJump && (!AirJumpRequested || DoubleJumpUsed)) return;
    if (!FirstJump)
    {
        DoubleJumpUsed = true; ++DoubleJumpsPerformed;
        if (Journey) Journey->Burst(GetActorLocation() - FVector(0,0,30), FLinearColor(.25,1,.85), 14);
    }
    DashRemaining = 0;
    GetCharacterMovement()->GravityScale = 1.6f;
    Slamming = false;
    UnCrouch();
    SlideRemaining = 0;
    LastJumpTime = Now; JumpCutPending = !JumpHeld;
    LaunchCharacter(FVector(0, 0, 650), false, true);
    JumpUsed = true;
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("POCInputAudit")))
        UE_LOG(LogTemp, Display, TEXT("POC_INPUT JUMP accepted type=%s xyz=%s"), FirstJump?TEXT("ground"):TEXT("double"), *GetActorLocation().ToCompactString());
#endif
    JumpQueuedUntil = -100;
    LastGroundTime = -100;
    AirJumpRequested = false;
    if (Journey) Journey->Sound(TEXT("Jump"), GetActorLocation(), FMath::FRandRange(.95f, 1.08f));
}
void APOCCharacter::Dash()
{
    auto* Movement = GetCharacterMovement();
    if (DeveloperFlight || Eating || Dreaming || Dying || DashCooldown > 0 || (Movement->IsFalling() && AirDashUsed)) return;
    UnCrouch(); SlideRemaining = 0; Slamming = false;
    DashDirection = GetLastMovementInputVector().GetSafeNormal2D();
    if (DashDirection.IsNearlyZero()) DashDirection = GetActorForwardVector();
    DashRemaining = .20f; DashCooldown = .9f; DashTrail = 0;
    AirDashUsed = Movement->IsFalling(); ++DashesPerformed;
    Movement->GravityScale = 0;
    LaunchCharacter(DashDirection * 1550, true, true);
    if (Journey) Journey->Sound(TEXT("Slide"), GetActorLocation(), 1.35f);
}

void APOCCharacter::SlidePressed()
{
    if (Eating || Dreaming || Dying) return;
    if (DeveloperFlight) { FlightDownHeld = true; return; }
    if (GetCharacterMovement()->IsFalling())
    {
        Slamming = true;
        LaunchCharacter(FVector(0, 0, -1700), false, true);
    }
    else
    {
        Crouch();
        SlideRemaining = .65f;
        GetCharacterMovement()->Velocity = GetActorForwardVector() * 1000;
        if (Journey) Journey->Sound(TEXT("Slide"), GetActorLocation());
    }
}
void APOCCharacter::SlideReleased() { FlightDownHeld = false; if (SlideRemaining <= 0) UnCrouch(); }

void APOCCharacter::Bonk()
{
    const bool Airborne = GetCharacterMovement()->IsFalling();
    if (DeveloperFlight || Eating || Dreaming || Dying || AttackRemaining > 0) return;
    AttackRemaining = .48f;
    StrikePending = true;
    if (Airborne && !AirBonkUsed)
    {
        AirBonkUsed = true;
        LaunchCharacter(GetActorForwardVector() * 620 + FVector(0, 0, FMath::Max(120.f, static_cast<float>(GetVelocity().Z))), true, true);
    }
    else GetCharacterMovement()->AddImpulse(GetActorForwardVector() * 180, true);
    if (Journey) Journey->Sound(TEXT("Bonk"), GetActorLocation());
}

void APOCCharacter::Strike(float Radius, bool Radial)
{
    FVector Center = GetActorLocation() + (Radial ? FVector::ZeroVector : GetActorForwardVector() * 65);
    for (TActorIterator<APOCEnemy> It(GetWorld()); It; ++It)
        if (FVector::DistSquared(It->GetActorLocation(), Center) < FMath::Square(Radius + 45.f)) It->Bonked(this, Radial);
    for (TActorIterator<APOCProp> It(GetWorld()); It; ++It)
        if (It->Kind == EPOCProp::Bomb && FVector::DistSquared(It->GetActorLocation(), Center) < FMath::Square(Radius + 70.f)) It->Kick(this);
    if (Journey) Journey->Burst(Center, FLinearColor(.9, .7, .3), Radial ? 20 : 7);
}

void APOCCharacter::Echo() { if (Journey && !DeveloperFlight && !Eating && !Dreaming && !Dying) Journey->ActivateEcho(this); }

void APOCCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("POCInputAudit")))
        UE_LOG(LogTemp, Display, TEXT("POC_INPUT LANDED airtime=%.3f xyz=%s"), AirTime, *GetActorLocation().ToCompactString());
#endif
    LandSquash = FMath::Clamp(AirTime * .45f, .08f, .35f);
    AirTime = 0;
    AirBonkUsed = DoubleJumpUsed = AirDashUsed = false;
    JumpUsed = false;
    LastGroundTime = GetWorld()->GetTimeSeconds();
    if (Slamming)
    {
        Strike(240, true);
        if (Journey) Journey->Sound(TEXT("Slam"), GetActorLocation());
        Slamming = false;
    }
    else if (Journey) Journey->Sound(TEXT("Land"), GetActorLocation(), .95f);
    TryBufferedJump();
}

void APOCCharacter::Bounce(float Strength)
{
    if (DeveloperFlight) return;
    Slamming = false;
    DoubleJumpUsed = AirDashUsed = false;
    JumpUsed = true;
    LastGroundTime = -100;
    LaunchCharacter(FVector(0, 0, Strength), false, true);
    if (Journey) Journey->Sound(TEXT("Bounce"), GetActorLocation());
}

void APOCCharacter::Hurt(FVector Source, float UpForce, float PushForce, int32 Damage)
{
    if (DeveloperFlight || Eating || Dreaming || Dying || InvulnerableRemaining > 0) return;
    DashRemaining = 0;
    GetCharacterMovement()->GravityScale = 1.6f;
    Hearts = FMath::Max(0, Hearts - FMath::Max(0, Damage));
    ++DamageEvents; LastDamageSource = Source;
    InvulnerableRemaining = 1.15f;
    if (Journey) { Journey->Burst(GetActorLocation(), FLinearColor(1, .3, .15), 8); Journey->Sound(TEXT("Hurt"), GetActorLocation()); }
    FVector Away = (GetActorLocation() - Source).GetSafeNormal2D();
    if (Away.IsNearlyZero()) Away = -GetActorForwardVector();
    LaunchCharacter(Away * PushForce + FVector(0, 0, UpForce), true, true);
    UE_LOG(LogTemp, Display, TEXT("POC_HIT hearts=%d damage=%d push=%.0f up=%.0f"), Hearts, Damage, PushForce, UpForce);
    if (Hearts == 0) BeginDeath(false);
}

void APOCCharacter::BeginDeath(bool Fell)
{
    if (DeveloperFlight || Dying || Eating || Dreaming) return;
    Hearts = 0; Dying = true; DeathRemaining = .75f;
    if (Fell) ++FallDeaths;
    ResetHeldInput(); UnCrouch();
    AttackRemaining = SlideRemaining = DashRemaining = 0;
    Slamming = StrikePending = false;
    GetCharacterMovement()->GravityScale = 1.6f;
    if (Journey) Journey->ShowCaption(Fell ? TEXT("One fall. Back to the checkpoint.") : TEXT("Out of hearts. Try again."), .75f);
    UE_LOG(LogTemp, Display, TEXT("POC_DEATH cause=%s hearts=0"), Fell ? TEXT("fall") : TEXT("damage"));
}

void APOCCharacter::Respawn()
{
    if (Eating || Dreaming) return;
    if (DeveloperFlight) { SetDeveloperFlight(false); return; }
    ResetHeldInput();
    Dying = false; DeathRemaining = 0;
    ++RespawnCount;
    UnCrouch();
    GetCharacterMovement()->StopMovementImmediately();
    if (Journey) Journey->ResetAtCheckpoint();
    SetActorLocationAndRotation(Checkpoint.GetLocation(), Checkpoint.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    if (Controller) Controller->SetControlRotation(FRotator(-14, Checkpoint.Rotator().Yaw, 0));
    Hearts = 3;
    Slamming = JumpUsed = SprintHeld = JumpHeld = AirBonkUsed = StrikePending = false;
    SlideRemaining = AttackRemaining = DashRemaining = DashCooldown = 0;
    DoubleJumpUsed = AirDashUsed = AirJumpRequested = false;
    GetCharacterMovement()->GravityScale = 1.6f;
    JumpQueuedUntil = LastGroundTime = -100;
    AirTime = 0;
    InvulnerableRemaining = 1;
    if (Journey) Journey->ShowCaption(TEXT("Still worth it."), 2.5);
}

void APOCCharacter::EatCake(FVector CakeLocation)
{
    if (DeveloperFlight || Eating) return;
    Eating = true;
    EatTime = 0;
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    SetActorRotation(FRotator(0, (CakeLocation - GetActorLocation()).Rotation().Yaw, 0));
    // Keep the player's camera angle through the cake animation.
}

void APOCCharacter::TryLedgeStep()
{
    if (!GetCharacterMovement()->IsFalling() || Eating || Slamming || AirTime < .2f || GetVelocity().Z > 0 || GetVelocity().Size2D() < 100) return;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(LedgeStep), false, this);
    FHitResult Hit;
    FVector Forward = GetVelocity().GetSafeNormal2D();
    FVector Start = GetActorLocation() + Forward * 65 + FVector(0, 0, 95);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0, 0, 135), ECC_Visibility, Params)
        && Hit.ImpactNormal.Z > .8 && Hit.ImpactPoint.Z > GetActorLocation().Z - 35)
    {
        const FVector Destination = Hit.ImpactPoint + FVector(0, 0, 48);
        if (!GetWorld()->OverlapBlockingTestByChannel(Destination, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeCapsule(25, 44), Params))
        {
            SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);
            GetCharacterMovement()->Velocity.Z = 0;
            JumpUsed = DoubleJumpUsed = AirDashUsed = AirBonkUsed = false;
            LandSquash = .18;
        }
    }
}

void APOCCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Dying)
    {
        DeathRemaining -= DeltaSeconds;
        Animate(DeltaSeconds);
        if (DeathRemaining <= 0) Respawn();
        return;
    }
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("POCInputAudit")))
    {
        InputAuditClock-=DeltaSeconds;
        if(InputAuditClock<=0)
        {
            InputAuditClock=.5f;
            UE_LOG(LogTemp,Display,TEXT("POC_INPUT POSITION xyz=%s camera=%s fov=%.1f arm=%.1f dash=%.2f spin=%.2f flight=%d hearts=%d collision=%d"),*GetActorLocation().ToCompactString(),Controller?*Controller->GetControlRotation().ToCompactString():TEXT("none"),Camera->FieldOfView,Boom->TargetArmLength,DashRemaining,AttackRemaining,DeveloperFlight,Hearts,GetActorEnableCollision());
        }
    }
#endif
    if (Dreaming) { AirTime = 0; Animate(DeltaSeconds); return; }
    if(CameraRecentering && Controller && !Eating)
    {
        const FRotator Target(-14.f,CameraCenterYaw,0);
        const FRotator Next=FMath::RInterpTo(Controller->GetControlRotation(),Target,DeltaSeconds,16.f);
        Controller->SetControlRotation(Next);
        if(Next.Equals(Target,.15f)) { Controller->SetControlRotation(Target);CameraRecentering=false; }
    }
    if (DeveloperFlight)
    {
        GetCharacterMovement()->MaxFlySpeed = SprintHeld ? 3000.f : 1400.f;
        AddMovementInput(FVector::UpVector, float(JumpHeld) - float(FlightDownHeld));
        Animate(DeltaSeconds);
        return;
    }
    // A tap gets at least 120 ms of lift; it must survive a key down/up arriving
    // in one streaming frame. Held jumps retain their full height.
    if (JumpCutPending && GetWorld()->GetTimeSeconds() - LastJumpTime >= .12f)
    {
        if (GetVelocity().Z > 0 && !Slamming) GetCharacterMovement()->Velocity.Z *= .72f;
        JumpCutPending = false;
    }
    AttackRemaining = FMath::Max(0.f, AttackRemaining - DeltaSeconds);
    // A short wind-up makes the contact coincide with the animated lunge.
    if (StrikePending && AttackRemaining <= .37f)
    {
        StrikePending = false;
        if (!Eating) Strike(185, true);
    }
    InvulnerableRemaining = FMath::Max(0.f, InvulnerableRemaining - DeltaSeconds);
    if (SlideRemaining > 0)
    {
        SlideRemaining -= DeltaSeconds;
        GetCharacterMovement()->GroundFriction = .4;
        GetCharacterMovement()->BrakingDecelerationWalking = 250;
        GetCharacterMovement()->MaxWalkSpeedCrouched = 1000;
        if (SlideRemaining <= 0) UnCrouch();
    }
    else { GetCharacterMovement()->GroundFriction = 7; GetCharacterMovement()->BrakingDecelerationWalking = 2800; GetCharacterMovement()->MaxWalkSpeedCrouched = 330; }
    auto* Movement = GetCharacterMovement();
    DashCooldown = FMath::Max(0.f, DashCooldown - DeltaSeconds);
    if (DashRemaining > 0)
    {
        DashRemaining -= DeltaSeconds;
        Movement->Velocity = DashDirection * 1550;
        Movement->GroundFriction = 0;
        Movement->BrakingDecelerationWalking = 0;
        Movement->MaxWalkSpeed = 1550;
        DashTrail -= DeltaSeconds;
        if (DashTrail <= 0) { DashTrail = .035f; if (Journey) Journey->Burst(GetActorLocation(), FLinearColor(.2,.9,1), 2); Strike(95, false); }
        if (DashRemaining <= 0) { Movement->GravityScale = 1.6f; Movement->Velocity = DashDirection * (SprintHeld ? SprintSpeed : RunSpeed); }
    }
    else Movement->MaxWalkSpeed = SprintHeld ? SprintSpeed : RunSpeed;
    if (GetCharacterMovement()->IsMovingOnGround())
    {
        LastGroundTime = GetWorld()->GetTimeSeconds();
        // Landing resets jump resources. Do not clear a queued launch while the
        // movement component still reports ground contact in this same frame.
    }
    else AirTime += DeltaSeconds;
    TryBufferedJump();
    TryLedgeStep();
    if (!Eating && Journey)
    {
        const auto* Point = Journey->Nearest(GetActorLocation());
        if (Point && GetActorLocation().Z < Point->Position.Z - 1700) BeginDeath(true);
    }
    if (Eating)
    {
        EatTime += DeltaSeconds;
        if (EatTime > 3.2f)
        {
            auto* GI = GetGameInstance<UPOCGameInstance>();
            if (!GI->Won) { GI->Won = true; GI->SaveProgress(); }
        }
    }
    auto* GI = GetGameInstance<UPOCGameInstance>();
    // A steady follow camera: movement, speed and district never rotate or zoom it.
    // Only deliberate look input changes the angle; the spring arm avoids walls.
    Boom->TargetArmLength = 420.f;
    Camera->FieldOfView = 82.f;

    Animate(DeltaSeconds);
}

bool APOCCharacter::IsPresentationVisible() const { return VisualRoot && VisualRoot->IsVisible(); }

void APOCCharacter::Animate(float DeltaSeconds)
{
    AnimationTime += DeltaSeconds;
    ThoughtBlend = FMath::FInterpTo(ThoughtBlend, Dreaming ? 1.f : 0.f, DeltaSeconds, 3.f);
    VisualRoot->SetVisibility(Dreaming || InvulnerableRemaining <= 0 || FMath::Fmod(AnimationTime, .16f) < .1f, true);
    LandSquash = FMath::FInterpTo(LandSquash, 0, DeltaSeconds, 12);
    const float Speed = FMath::Clamp(static_cast<float>(GetVelocity().Size2D()) / RunSpeed, 0.f, 1.5f);
    const bool Airborne = GetCharacterMovement()->IsFalling();
    GaitPhase += DeltaSeconds * (SprintHeld ? 25.f : 20.f) * FMath::Min(Speed, 1.25f);
    const float Gait = GaitPhase;
    const float GroundSpeed = Airborne ? 0.f : Speed;
    const float SpinProgress = AttackRemaining > 0 ? (.48f-AttackRemaining)/.48f : 0;
    const float BonkOffset = AttackRemaining > 0 ? FMath::Sin(SpinProgress*PI) : 0;
    const float TurnRate = FMath::FindDeltaAngleDegrees(PreviousFacing,GetActorRotation().Yaw)/FMath::Max(DeltaSeconds,.001f);
    PreviousFacing = GetActorRotation().Yaw;
    BodyLean = FMath::FInterpTo(BodyLean,FMath::Clamp(-TurnRate*.028f,-19.f,19.f)*FMath::Min(Speed,1.f),DeltaSeconds,10);
    const float Stretch = Airborne && !Slamming ? FMath::Clamp(static_cast<float>(GetVelocity().Z)/1800.f,-.13f,.18f) : 0;
    Body->SetRelativeScale3D(FVector(.51 * (1 + LandSquash - Stretch*.35f), .44 * (1 + LandSquash - Stretch*.35f), .55 * (1 - LandSquash + Stretch)));
    const float Bob = FMath::Sin(Gait * 2) * GroundSpeed * 2 + FMath::Sin(AnimationTime * 2.2f) * 1.5;
    VisualRoot->SetRelativeLocation(FVector(BonkOffset * 12, 0, Bob - (bIsCrouched ? 15 : 0)));
    VisualRoot->SetRelativeRotation(FRotator(FMath::Lerp(bIsCrouched ? -55.f : Slamming ? -30.f : Speed * -9.f, -3.f, ThoughtBlend), SpinProgress*720, Eating ? FMath::Sin(EatTime * 15) * 6 : FMath::Lerp(BodyLean, 2.f, ThoughtBlend)));
    HeadRoot->SetRelativeLocation(FVector(7 + BonkOffset * 13 + (Eating ? FMath::Sin(EatTime * 13) * 4 : 0), 0, 25));
    HeadRoot->SetRelativeRotation(FRotator(ThoughtBlend * (5.f + FMath::Sin(AnimationTime * 1.3f)), ThoughtBlend * -9.f, ThoughtBlend * -7.f));
    for (int32 Index = 0; Index < Feet.Num(); ++Index)
    {
        const float Phase = Gait + Index * PI;
        Feet[Index]->SetRelativeLocation(FVector(6 + FMath::Sin(Phase) * GroundSpeed * 20, (Index == 0 ? -15 : 15), -34 + FMath::Max(0.f, FMath::Cos(Phase)) * GroundSpeed * 8 + (Airborne ? 6 : 0)));
        EarRoots[Index]->SetRelativeRotation(FRotator(-12 - Speed * 16 + FMath::Sin(Gait - Index) * Speed * 6, 0, (Index == 0 ? -19 : 19)));
        const bool Blink = FMath::Fmod(AnimationTime + .5f, 4.6f) < .12f;
        Eyes[Index]->SetRelativeScale3D(FVector(.07, .135, Blink ? .018 : .175));
        const float Side = Index == 0 ? -1.f : 1.f;
        Paws[Index]->SetRelativeLocation(FVector(13 - FMath::Sin(Phase) * GroundSpeed * 17 + BonkOffset * 12,
            Side * (24 + (Airborne ? 10 : 0) + BonkOffset*20), -3 + (Airborne ? 9 : 0) + (Eating ? FMath::Sin(EatTime * 13) * 5 + 13 : 0)));
        Paws[Index]->SetRelativeRotation(FRotator(BonkOffset * 60 - FMath::Sin(Phase)*GroundSpeed*24, 0, Side * (Airborne ? -40 : -BonkOffset*60)));
        if (ThoughtBlend > .001f)
        {
            // One paw beneath the muzzle, the other at the hip: an unambiguous thinking pose.
            const FVector Thinking = Index == 0 ? FVector(28,-13,15) : FVector(4,25,-7);
            Paws[Index]->SetRelativeLocation(FMath::Lerp(Paws[Index]->GetRelativeLocation(), Thinking, ThoughtBlend));
            Paws[Index]->SetRelativeRotation(FMath::Lerp(Paws[Index]->GetRelativeRotation(), FRotator(Index==0 ? -28.f : 8.f,0,0), ThoughtBlend));
        }
        for (int32 Detail = Index * 3; Detail < Index * 3 + 3; ++Detail) EyeDetails[Detail]->SetVisibility(!Blink && VisualRoot->IsVisible());
    }
    for(int32 I=0; I<HeadbandTails.Num(); ++I)
    {
        const float Flutter=FMath::Sin(AnimationTime*(Dreaming ? 2.f : 11.f)-I*1.4f);
        HeadbandTails[I]->SetRelativeLocation(FVector(-33-Speed*7,I?9:-9,12+Flutter*3-I*4));
        HeadbandTails[I]->SetRelativeRotation(FRotator(-18+Speed*18+Flutter*10,I?16:-16,0));
    }
    for (int32 Index = 0; Index < Scarf.Num(); ++Index)
    {
        const float Wind = FMath::Sin(AnimationTime * (Dreaming ? 2.f : 6 + Speed * 3) - Index * .7f);
        Scarf[Index]->SetRelativeLocation(FVector(-22 - Index * (8 + Speed * 3), 5 + Wind * Index * 1.8,
            10 - Index * 5 * (1 - FMath::Min(Speed, 1.f)) + Wind * Index * 1.2 + (Slamming ? Index * 7 : 0)));
        Scarf[Index]->SetRelativeRotation(FRotator(-24 * (1 - FMath::Min(Speed, 1.f)) + Wind * 10, Wind * 12, 0));
    }
}
