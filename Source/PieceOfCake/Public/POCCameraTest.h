#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "POCCameraTest.generated.h"

// Native input regression for camera control; not a journey completion test.
UCLASS(Transient, NotBlueprintable)
class PIECEOFCAKE_API APOCCameraTest : public AActor
{
    GENERATED_BODY()
public:
    APOCCameraTest();
    virtual void Tick(float DeltaSeconds) override;
private:
    void Finish(bool Passed,const FString& Reason);
    int32 Phase=-1;
    double Born=0,PhaseAt=0;
    FRotator StartView;
    FVector StartPosition;
    bool ToggleOn=false, PauseReset=false;
    TMap<FString,double> Measures;
};
