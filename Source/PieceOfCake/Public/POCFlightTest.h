#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "POCFlightTest.generated.h"

// Isolated developer-feature test. Never used as evidence of route completion.
UCLASS(Transient, NotBlueprintable)
class PIECEOFCAKE_API APOCFlightTest : public AActor
{
    GENERATED_BODY()
public:
    APOCFlightTest();
    virtual void Tick(float DeltaSeconds) override;
private:
    void Finish(bool Passed, const FString& Reason);
    int32 Phase = -1;
    double Born = 0, PhaseAt = 0;
    FVector Start, NormalStart;
    TArray<FVector> Deltas;
    bool DamageBlocked = false, BelowFloor = false, Restored = false;
    bool NormalJumpReleased = false;
    bool LandedWhereExplored=false, NormalGateProtected=false, UnsafeExitBlocked=false;
    bool ExplicitRescue=false, CakeCompleted=false, AssistedUnranked=false;
    float NormalJumpHeight = 0;
    int32 InitialRespawns = 0, InitialShards = 0;
};
