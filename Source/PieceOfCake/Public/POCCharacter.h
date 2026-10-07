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
    UFUNCTION(BlueprintCallable) void Bonk();
    UFUNCTION(BlueprintCallable) void Echo();
    UFUNCTION(BlueprintCallable) void Respawn();
    UFUNCTION(BlueprintCallable) void Hurt(FVector Source);
    UFUNCTION(BlueprintCallable) void Bounce(float Strength = 820.f);
    UFUNCTION(BlueprintCallable) void EatCake(FVector CakeLocation);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement") float RunSpeed = 620.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement") float SprintSpeed = 840.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement") float CoyoteSeconds = .12f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement") float BufferSeconds = .14f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USpringArmComponent> Boom;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(BlueprintReadOnly) bool Slamming = false;
    UPROPERTY(BlueprintReadOnly) bool Eating = false;
    UPROPERTY(BlueprintReadOnly) float AttackRemaining = 0;
    UPROPERTY(BlueprintReadOnly) float SlideRemaining = 0;
    UPROPERTY(BlueprintReadOnly) int32 Hearts = 3;
    UPROPERTY() TObjectPtr<APOCWorld> Journey;
    FTransform Checkpoint;
    float InvulnerableRemaining = 0;
private:
    void MoveForward(float Value);
    void MoveRight(float Value);
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
    float LandSquash = 0;
    float EatTime = 0;
    bool SprintHeld = false;
    bool JumpHeld = false;
    bool JumpUsed = false;
    bool AirBonkUsed = false;
    bool StrikePending = false;
    float GaitPhase = 0;
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
