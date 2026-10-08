#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "POCCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class APOCWorld;

UCLASS(Blueprintable)
class PIECEOFCAKE_API APOCCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    APOCCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual void Landed(const FHitResult& Hit) override;
    void ResetHeldInput();
    bool IsMouseCameraActive() const { return CameraLookHeld || CameraLookToggled; }
    void SetDeveloperFlight(bool Enabled);
    UPROPERTY(BlueprintReadOnly) bool DeveloperFlight = false;
    UFUNCTION(BlueprintCallable) void Bonk();
    UFUNCTION(BlueprintCallable) void Echo();
    UFUNCTION(BlueprintCallable) void Dash();
    UPROPERTY(BlueprintReadOnly) float DashCooldown = 0;
    UPROPERTY(BlueprintReadOnly) float DashRemaining = 0;
    int32 DoubleJumpsPerformed = 0;
    int32 DashesPerformed = 0;
    UFUNCTION(BlueprintCallable) void Respawn();
    UFUNCTION(BlueprintCallable) void Hurt(FVector Source, float UpForce = 420.f, float PushForce = 650.f, int32 Damage = 1);
    UFUNCTION(BlueprintCallable) void Bounce(float Strength = 820.f);
    UFUNCTION(BlueprintCallable) void EatCake(FVector CakeLocation);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement") float RunSpeed = 820.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement") float SprintSpeed = 1100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement") float CoyoteSeconds = .16f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement") float BufferSeconds = .18f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USpringArmComponent> Boom;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(BlueprintReadOnly) bool Slamming = false;
    UPROPERTY(BlueprintReadOnly) bool Eating = false;
    bool Dreaming = false;
    bool IsPresentationVisible() const;
    float ThoughtBlend = 0;
    UPROPERTY(BlueprintReadOnly) bool Dying = false;
    float DeathRemaining = 0;
    int32 DamageEvents = 0;
    int32 FallDeaths = 0;
    FVector LastDamageSource;
    void BeginDeath(bool Fell);
    UPROPERTY(BlueprintReadOnly) float AttackRemaining = 0;
    UPROPERTY(BlueprintReadOnly) float SlideRemaining = 0;
    UPROPERTY(BlueprintReadOnly) int32 Hearts = 3;
    UPROPERTY() TObjectPtr<APOCWorld> Journey;
    FTransform Checkpoint;
    int32 RespawnCount = 0;
    float InvulnerableRemaining = 0;
private:
    friend class APOCJourneyTest;
    void MoveForward(float Value);
    void MoveRight(float Value);
    void CameraLookPressed();
    void CameraLookReleased();
    bool CameraLookHeld = false;
    bool CameraLookToggled = false;
    bool CameraRecentering = false;
    float CameraCenterYaw = 0;
    void ToggleCameraLook();
    void CenterCamera();
    void ApplyCameraDelta(float Yaw, float Pitch);
    void LookYawKeys(float Value);
    void LookPitchKeys(float Value);
    void LookYaw(float Value);
    void LookPitch(float Value);
    void LookYawPad(float Value);
    void LookPitchPad(float Value);
    void JumpPressed();
    void JumpReleased();
    void SprintPressed();
    void SprintReleased();
    void SlidePressed();
    void SlideReleased();
    void TryBufferedJump();
    void Strike(float Radius, bool Radial);
    void Animate(float DeltaSeconds);
    void TryLedgeStep();
    float LastGroundTime = -100.f;
    float JumpQueuedUntil = -100.f;
    float AirTime = 0;
    float AnimationTime = 0;
    float InputAuditClock = 0;
    float LandSquash = 0;
    float EatTime = 0;
    bool SprintHeld = false;
    bool JumpHeld = false;
    bool FlightDownHeld = false;
    bool JumpCutPending = false;
    float LastJumpTime = -100.f;
    bool JumpUsed = false;
    bool AirBonkUsed = false;
    bool AirJumpRequested = false;
    bool DoubleJumpUsed = false;
    bool AirDashUsed = false;
    FVector DashDirection;
    float DashTrail = 0;
    bool StrikePending = false;
    float GaitPhase = 0;
    float BodyLean = 0;
    float PreviousFacing = 0;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> HeadbandTails;
    UPROPERTY() TObjectPtr<USceneComponent> HeadRoot;
    UPROPERTY() TArray<TObjectPtr<USceneComponent>> EarRoots;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Paws;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> EyeDetails;
    UPROPERTY() TObjectPtr<USceneComponent> VisualRoot;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Head;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Feet;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Ears;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Eyes;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Scarf;
};
