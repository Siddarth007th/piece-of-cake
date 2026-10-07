#include "POCCharacter.h"
#include "POCWorld.h"
#include "POCGameInstance.h"
#include "POCVisuals.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
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
    Movement->RotationRate = FRotator(0, 760, 0);
    Movement->MaxWalkSpeed = RunSpeed;
    Movement->MaxAcceleration = 3200;
    Movement->BrakingDecelerationWalking = 2800;
    Movement->GroundFriction = 7;
    Movement->AirControl = .7f;
    Movement->JumpZVelocity = 650;
    Movement->GravityScale = 1.6f;
    Movement->MaxStepHeight = 45;
    Movement->SetWalkableFloorAngle(46.f);
    Movement->GetNavAgentPropertiesRef().bCanCrouch = true;
    Movement->SetCrouchedHalfHeight(23.f);
    Movement->MaxWalkSpeedCrouched = 330;
    Boom = CreateDefaultSubobject<USpringArmComponent>(TEXT("FollowBoom"));
    Boom->SetupAttachment(RootComponent);
    Boom->TargetArmLength = 360;
    Boom->TargetOffset = FVector(0, 0, 75);
    Boom->bUsePawnControlRotation = true;
    Boom->bEnableCameraLag = true;
    Boom->CameraLagSpeed = 10;
    Boom->CameraLagMaxDistance = 85;
    Boom->bEnableCameraRotationLag = true;
    Boom->CameraRotationLagSpeed = 16;
    Boom->ProbeSize = 18;
    Boom->bDoCollisionTest = true;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("AdventureCamera"));
    Camera->SetupAttachment(Boom);
    Camera->FieldOfView = 78;
    VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Nori"));
    VisualRoot->SetupAttachment(RootComponent);
}

void APOCCharacter::BeginPlay()
{
    Super::BeginPlay();
    const FLinearColor Cream(.87f, .82f, .66f), Teal(.11f, .36f, .36f), Ink(.013f, .024f, .035f), Red(.65f, .095f, .065f);
    Body = POCVisuals::Part(this, VisualRoot, TEXT("Body"), TEXT("Sphere"), FVector(0, 0, -5), FVector(.51, .44, .55), Cream);
    Head = POCVisuals::Part(this, VisualRoot, TEXT("Head"), TEXT("Sphere"), FVector(7, 0, 25), FVector(.52, .49, .44), Cream);
    POCVisuals::Part(this, VisualRoot, TEXT("Nose"), TEXT("Sphere"), FVector(32, 0, 23), FVector(.075, .1, .07), Ink);
    POCVisuals::Part(this, VisualRoot, TEXT("Pack"), TEXT("Cube"), FVector(-23, 0, 1), FVector(.2, .33, .34), FLinearColor(.23, .19, .11));
    POCVisuals::Part(this, VisualRoot, TEXT("PackClasp"), TEXT("Cube"), FVector(-34, 0, 1), FVector(.025, .075, .12), FLinearColor(.88, .61, .22));
    POCVisuals::Part(this, VisualRoot, TEXT("ScarfCollar"), TEXT("Sphere"), FVector(2, 0, 10), FVector(.5, .47, .12), Red);
    for (int32 Side = -1; Side <= 1; Side += 2)
    {
        auto* Ear = POCVisuals::Part(this, VisualRoot, *FString::Printf(TEXT("Ear%d"), Side), TEXT("Cone"), FVector(0, Side * 16, 60), FVector(.15, .22, .47), Teal);
        Ear->SetRelativeRotation(FRotator(-12, 0, Side * 19));
        Ears.Add(Ear);
        Eyes.Add(POCVisuals::Part(this, VisualRoot, *FString::Printf(TEXT("Eye%d"), Side), TEXT("Sphere"), FVector(28, Side * 13, 29), FVector(.1, .16, .2), Ink));
        POCVisuals::Part(this, VisualRoot, *FString::Printf(TEXT("Glint%d"), Side), TEXT("Sphere"), FVector(32, Side * 13 - 2, 33), FVector(.035, .055, .055), FLinearColor::White, .1f);
        Feet.Add(POCVisuals::Part(this, VisualRoot, *FString::Printf(TEXT("Foot%d"), Side), TEXT("Sphere"), FVector(6, Side * 15, -34), FVector(.27, .19, .16), Teal));
        POCVisuals::Part(this, VisualRoot, *FString::Printf(TEXT("Paw%d"), Side), TEXT("Sphere"), FVector(13, Side * 25, -3), FVector(.17, .13, .19), Cream);
    }
    for (int32 Index = 0; Index < 6; ++Index)
        Scarf.Add(POCVisuals::Part(this, VisualRoot, *FString::Printf(TEXT("Scarf%d"), Index), TEXT("Cube"), FVector(-22 - Index * 11, 5, 10), FVector(.14, .14 - Index * .009, .025), Red));
    Checkpoint = GetActorTransform();
}

void APOCCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"), this, &APOCCharacter::MoveForward);
    Input->BindAxis(TEXT("MoveRight"), this, &APOCCharacter::MoveRight);
    Input->BindAxis(TEXT("LookYaw"), this, &APOCCharacter::LookYaw);
    Input->BindAxis(TEXT("LookPitch"), this, &APOCCharacter::LookPitch);
    Input->BindAxis(TEXT("LookYawPad"), this, &APOCCharacter::LookYawPad);
    Input->BindAxis(TEXT("LookPitchPad"), this, &APOCCharacter::LookPitchPad);
    Input->BindAction(TEXT("Jump"), IE_Pressed, this, &APOCCharacter::JumpPressed);
    Input->BindAction(TEXT("Jump"), IE_Released, this, &APOCCharacter::JumpReleased);
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
    if (!Controller || Eating || FMath::IsNearlyZero(Value)) return;
    AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X), Value);
}
void APOCCharacter::MoveRight(float Value)
{
    if (!Controller || Eating || FMath::IsNearlyZero(Value)) return;
    AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y), Value);
}
void APOCCharacter::LookYaw(float Value)
{
    auto* GI = GetGameInstance<UPOCGameInstance>();
    if (!Eating && GI && !UGameplayStatics::IsGamePaused(this)) AddControllerYawInput(Value * GI->Save->Sensitivity);
}
void APOCCharacter::LookPitch(float Value)
{
    auto* GI = GetGameInstance<UPOCGameInstance>();
    if (!Eating && GI && !UGameplayStatics::IsGamePaused(this)) AddControllerPitchInput(Value * GI->Save->Sensitivity * (GI->Save->InvertY ? -1 : 1));
}
void APOCCharacter::LookYawPad(float Value) { LookYaw(Value * 100.f * GetWorld()->GetDeltaSeconds()); }
void APOCCharacter::LookPitchPad(float Value) { LookPitch(Value * 85.f * GetWorld()->GetDeltaSeconds()); }
void APOCCharacter::SprintPressed() { SprintHeld = true; }
void APOCCharacter::SprintReleased() { SprintHeld = false; }
void APOCCharacter::JumpPressed()
{
    if (Eating) return;
    JumpHeld = true;
    JumpQueuedUntil = GetWorld()->GetTimeSeconds() + BufferSeconds;
    TryBufferedJump();
}
void APOCCharacter::JumpReleased()
{
    JumpHeld = false;
    if (GetVelocity().Z > 0 && !Slamming) GetCharacterMovement()->Velocity.Z *= .48f;
}
void APOCCharacter::TryBufferedJump()
{
    const float Now = GetWorld()->GetTimeSeconds();
    if (Eating || JumpQueuedUntil < Now || JumpUsed) return;
    if (!GetCharacterMovement()->IsMovingOnGround() && Now - LastGroundTime > CoyoteSeconds) return;
    UnCrouch();
    SlideRemaining = 0;
    LaunchCharacter(FVector(0, 0, 650), false, true);
    JumpUsed = true;
    JumpQueuedUntil = -100;
    LastGroundTime = -100;
    if (Journey) Journey->Sound(TEXT("Jump"), GetActorLocation(), FMath::FRandRange(.95f, 1.08f));
}
void APOCCharacter::SlidePressed()
{
    if (Eating) return;
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
void APOCCharacter::SlideReleased() { if (SlideRemaining <= 0) UnCrouch(); }

void APOCCharacter::Bonk()
{
    if (Eating || AttackRemaining > 0) return;
    AttackRemaining = .38f;
    if (GetCharacterMovement()->IsFalling())
        LaunchCharacter(GetActorForwardVector() * 620 + FVector(0, 0, FMath::Max(120.f, static_cast<float>(GetVelocity().Z))), true, true);
    else GetCharacterMovement()->AddImpulse(GetActorForwardVector() * 180, true);
    Strike(110, false);
    if (Journey) Journey->Sound(TEXT("Bonk"), GetActorLocation());
}

void APOCCharacter::Strike(float Radius, bool Radial)
{
    FVector Center = GetActorLocation() + (Radial ? FVector::ZeroVector : GetActorForwardVector() * 65);
    for (TActorIterator<APOCEnemy> It(GetWorld()); It; ++It)
        if (FVector::DistSquared(It->GetActorLocation(), Center) < FMath::Square(Radius + 45.f)) It->Bonked(this, Radial);
    if (Journey) Journey->Burst(Center, FLinearColor(.9, .7, .3), Radial ? 20 : 7);
}

void APOCCharacter::Echo() { if (Journey && !Eating) Journey->ActivateEcho(this); }

void APOCCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    LandSquash = FMath::Clamp(AirTime * .45f, .08f, .35f);
    AirTime = 0;
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
    Slamming = false;
    JumpUsed = true;
    LastGroundTime = -100;
    LaunchCharacter(FVector(0, 0, Strength), false, true);
    if (Journey) Journey->Sound(TEXT("Bounce"), GetActorLocation());
}

void APOCCharacter::Hurt(FVector Source)
{
    if (Eating || InvulnerableRemaining > 0) return;
    Hearts--;
    InvulnerableRemaining = 1.6f;
    if (Journey) { Journey->Burst(GetActorLocation(), FLinearColor(1, .3, .15), 8); Journey->Sound(TEXT("Hurt"), GetActorLocation()); }
    if (Hearts <= 0) { Respawn(); return; }
    FVector Away = (GetActorLocation() - Source).GetSafeNormal2D();
    LaunchCharacter(Away * 390 + FVector(0, 0, 350), true, true);
}

void APOCCharacter::Respawn()
{
    if (Eating) return;
    UnCrouch();
    GetCharacterMovement()->StopMovementImmediately();
    if (Journey) Journey->ResetAtCheckpoint();
    SetActorLocationAndRotation(Checkpoint.GetLocation(), Checkpoint.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    if (Controller) Controller->SetControlRotation(FRotator(-14, Checkpoint.Rotator().Yaw, 0));
    Hearts = 3;
    Slamming = JumpUsed = SprintHeld = JumpHeld = false;
    SlideRemaining = AttackRemaining = 0;
    JumpQueuedUntil = LastGroundTime = -100;
    AirTime = 0;
    InvulnerableRemaining = 1;
    if (Journey) Journey->ShowCaption(TEXT("Still worth it."), 2.5);
}

void APOCCharacter::EatCake(FVector CakeLocation)
{
    if (Eating) return;
    Eating = true;
    EatTime = 0;
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->DisableMovement();
    SetActorRotation((CakeLocation - GetActorLocation()).Rotation());
    if (Controller) Controller->SetControlRotation(FRotator(-12, GetActorRotation().Yaw + 130, 0));
}

void APOCCharacter::TryLedgeStep()
{
    if (Eating || Slamming || AirTime < .2f || GetVelocity().Z > 0 || GetVelocity().Size2D() < 100) return;
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
            JumpUsed = false;
            LandSquash = .18;
        }
    }
}

void APOCCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AttackRemaining = FMath::Max(0.f, AttackRemaining - DeltaSeconds);
    InvulnerableRemaining = FMath::Max(0.f, InvulnerableRemaining - DeltaSeconds);
    if (SlideRemaining > 0)
    {
        SlideRemaining -= DeltaSeconds;
        GetCharacterMovement()->GroundFriction = .4;
        GetCharacterMovement()->MaxWalkSpeedCrouched = 1000;
        if (SlideRemaining <= 0) UnCrouch();
    }
    else { GetCharacterMovement()->GroundFriction = 7; GetCharacterMovement()->MaxWalkSpeedCrouched = 330; }
    GetCharacterMovement()->MaxWalkSpeed = SprintHeld ? SprintSpeed : RunSpeed;
    if (GetCharacterMovement()->IsMovingOnGround())
    {
        LastGroundTime = GetWorld()->GetTimeSeconds();
        if (GetVelocity().Z <= 0) JumpUsed = false;
    }
    else AirTime += DeltaSeconds;
    TryBufferedJump();
    TryLedgeStep();
    if (!Eating && Journey)
    {
        const auto* Point = Journey->Nearest(GetActorLocation());
        if (Point && GetActorLocation().Z < Point->Position.Z - 1700) Respawn();
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
    const float TargetDistance = Eating ? 260.f : (GI->Section == 5 ? 520.f : 360.f) + (SprintHeld ? 70 : 0);
    Boom->TargetArmLength = FMath::FInterpTo(Boom->TargetArmLength, TargetDistance, DeltaSeconds, 2.f);
    Camera->FieldOfView = FMath::FInterpTo(Camera->FieldOfView, SprintHeld ? 86.f : 78.f, DeltaSeconds, 3.f);
    Animate(DeltaSeconds);
}

void APOCCharacter::Animate(float DeltaSeconds)
{
    AnimationTime += DeltaSeconds;
    LandSquash = FMath::FInterpTo(LandSquash, 0, DeltaSeconds, 12);
    const float Speed = FMath::Clamp(static_cast<float>(GetVelocity().Size2D()) / RunSpeed, 0.f, 1.5f);
    const float Gait = AnimationTime * (SprintHeld ? 22 : 17);
    const float BonkOffset = AttackRemaining > .15f ? FMath::Sin((.38f - AttackRemaining) / .23f * PI) : 0;
    Body->SetRelativeScale3D(FVector(.51 * (1 + LandSquash), .44 * (1 + LandSquash), .55 * (1 - LandSquash)));
    const float Bob = FMath::Sin(Gait * 2) * Speed * 2 + FMath::Sin(AnimationTime * 2.2f) * 1.5;
    VisualRoot->SetRelativeLocation(FVector(BonkOffset * 12, 0, Bob - (bIsCrouched ? 15 : 0)));
    VisualRoot->SetRelativeRotation(FRotator(bIsCrouched ? -55 : Slamming ? -30 : Speed * -5, 0, Eating ? FMath::Sin(EatTime * 15) * 6 : 0));
    Head->SetRelativeLocation(FVector(7 + BonkOffset * 13 + (Eating ? FMath::Sin(EatTime * 13) * 4 : 0), 0, 25));
    for (int32 Index = 0; Index < Feet.Num(); ++Index)
    {
        const float Phase = Gait + Index * PI;
        Feet[Index]->SetRelativeLocation(FVector(6 + FMath::Sin(Phase) * Speed * 13, (Index == 0 ? -15 : 15), -34 + FMath::Max(0.f, FMath::Cos(Phase)) * Speed * 8));
        Ears[Index]->SetRelativeRotation(FRotator(-12 - Speed * 16 + FMath::Sin(Gait - Index) * Speed * 6, 0, (Index == 0 ? -19 : 19)));
        const bool Blink = FMath::Fmod(AnimationTime + .5f, 4.6f) < .12f;
        Eyes[Index]->SetRelativeScale3D(FVector(.1, .16, Blink ? .018 : .2));
    }
    for (int32 Index = 0; Index < Scarf.Num(); ++Index)
    {
        const float Wind = FMath::Sin(AnimationTime * (6 + Speed * 3) - Index * .7f);
        Scarf[Index]->SetRelativeLocation(FVector(-22 - Index * (8 + Speed * 3), 5 + Wind * Index * 1.8,
            10 - Index * 5 * (1 - FMath::Min(Speed, 1.f)) + Wind * Index * 1.2 + (Slamming ? Index * 7 : 0)));
        Scarf[Index]->SetRelativeRotation(FRotator(Wind * 10, Wind * 12, 0));
    }
    VisualRoot->SetVisibility(InvulnerableRemaining <= 0 || FMath::Fmod(AnimationTime, .16f) < .1f, true);
}
