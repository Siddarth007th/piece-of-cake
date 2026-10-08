#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "POCPresentationTest.generated.h"
UCLASS()
class PIECEOFCAKE_API APOCPresentationTest : public AActor
{
    GENERATED_BODY()
public:
    APOCPresentationTest();
    virtual void Tick(float DeltaSeconds) override;
private:
    double Born=0;
    int32 Stage=0;
    FVector Start;
};
