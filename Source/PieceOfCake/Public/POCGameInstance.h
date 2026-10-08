#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/SaveGame.h"
#include "POCGameInstance.generated.h"

class GenericApplication;
class UPOCCloud;

USTRUCT()
struct FPOCPendingRun
{
    GENERATED_BODY()
    UPROPERTY(SaveGame) FString Id;
    UPROPERTY(SaveGame) int32 Seconds=0;
    UPROPERTY(SaveGame) int32 Shards=0;
    UPROPERTY(SaveGame) int32 Relics=0;
    UPROPERTY(SaveGame) bool Assisted=false;
};

UCLASS()
class PIECEOFCAKE_API UPOCSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) TArray<FPOCPendingRun> PendingRuns;
    UPROPERTY(SaveGame) bool Completed = false;
    UPROPERTY(SaveGame) int32 BestShards = 0;
    UPROPERTY(SaveGame) int32 BestRelics = 0;
    UPROPERTY(SaveGame) float MusicVolume = .55f;
    UPROPERTY(SaveGame) float EffectsVolume = .8f;
    UPROPERTY(SaveGame) float Sensitivity = 1.f;
    UPROPERTY(SaveGame) bool Subtitles = true;
    UPROPERTY(SaveGame) bool InvertY = false;
    UPROPERTY(SaveGame) int32 Quality = 1;
    UPROPERTY(SaveGame) bool Fullscreen = false;
};

UCLASS()
class PIECEOFCAKE_API UPOCGameInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    virtual void Init() override;
    virtual void Shutdown() override;
    UFUNCTION(BlueprintCallable) void SaveProgress(bool SyncCloud = true);
    UFUNCTION(BlueprintCallable) void ResetRun();
    UFUNCTION(BlueprintCallable) void ApplySettings();
    bool TrySpendShards(int32 Cost);
    UPROPERTY(BlueprintReadOnly) int32 Shards = 0;
    UPROPERTY(BlueprintReadOnly) int32 Relics = 0;
    UPROPERTY(BlueprintReadOnly) int32 ShardsSpent = 0;
    UPROPERTY(BlueprintReadOnly) int32 Section = 0;
    UPROPERTY(BlueprintReadOnly) float RunSeconds = 0;
    UPROPERTY(BlueprintReadOnly) bool Won = false;
    UPROPERTY() TObjectPtr<UPOCSaveGame> Save;
    UPROPERTY() TObjectPtr<UPOCCloud> Cloud;
    FGuid RunId=FGuid::NewGuid();
    bool AssistedRun=false;
    bool RunQueued=false;
    TSet<int32> Collected;
    TSharedPtr<GenericApplication> NativeApplication;
    bool QARestartPending = false; // Not saved; survives only the QA level reload.
};
