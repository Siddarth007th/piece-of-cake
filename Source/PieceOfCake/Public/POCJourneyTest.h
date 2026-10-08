#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "POCJourneyTest.generated.h"

// Development-only route runner: uses the character's ordinary movement/actions.
// No teleporting, collision overrides, god mode, or accelerated world time.
UCLASS(NotBlueprintable, Transient)
class PIECEOFCAKE_API APOCJourneyTest : public AActor
{
    GENERATED_BODY()
public:
    APOCJourneyTest();
    virtual void Tick(float DeltaSeconds) override;
private:
    void Finish(bool Passed, const FString& Reason);
    int32 Run = 0;
    int32 SectionProbe = -1;
    bool SectionProbeStarted = false;
    int32 Furthest = -1;
    int32 LastIndex = 0;
    int32 CenteredPlatform = -1;
    int32 TravelDirection = 1;
    bool BacktrackStarted = false;
    bool BacktrackCompleted = false;
    bool InsufficientShardsVerified = false;
    int32 Respawns = 0;
    int32 LastRespawnCount = 0;
    int32 JumpCount = 0;
    int32 EchoCount = 0;
    int32 BonkCount = 0;
    int32 SlideCount = 0;
    bool RestartBegan = false;
    bool EchoExpiryCompleted = false;
    int32 ExpiryGroup = -1;
    double EchoWaitUntil = 0;
    double WonAt = 0;
    bool Started = false;
    bool Finished = false;
    bool FallStarted = false;
    bool FallCompleted = false;
    bool EmptyHeartsObserved = false;
    bool BombAuditActive = false;
    bool BombDamageVerified = false;
    bool BombKnockbackVerified = false;
    int32 BombHeartsBefore = 0;
    int32 BombEventsBefore = 0;
    FVector BombAuditSource;
    double BombHitAt = 0;
    bool PauseCompleted = false;
    double Born = 0;
    double StartedAt = 0;
    double LastTick = 0;
    double LastProgress = 0;
    double LastEcho = 0;
    double PauseUntil = 0;
    double LastDiagnostic = 0;
    TSet<int32> LandedPlatforms;
    TSet<int32> Checkpoints;
    TSet<int32> EchoGroups;
    TArray<double> FrameMs;
};
