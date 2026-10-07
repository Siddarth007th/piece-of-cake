#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/SaveGame.h"
#include "POCGameInstance.generated.h"

UCLASS()
class PIECEOFCAKE_API UPOCSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) bool Completed = false;
    UPROPERTY(SaveGame) int32 BestShards = 0;
    UPROPERTY(SaveGame) int32 BestRelics = 0;
    UPROPERTY(SaveGame) float MusicVolume = .55f;
    UPROPERTY(SaveGame) float EffectsVolume = .8f;
    UPROPERTY(SaveGame) float Sensitivity = 1.f;
    UPROPERTY(SaveGame) bool Subtitles = true;
    UPROPERTY(SaveGame) bool InvertY = false;
    UPROPERTY(SaveGame) int32 Quality = 2;
    UPROPERTY(SaveGame) bool Fullscreen = false;
};

UCLASS()
class PIECEOFCAKE_API UPOCGameInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    virtual void Init() override;
    UFUNCTION(BlueprintCallable) void SaveProgress();
    UFUNCTION(BlueprintCallable) void ResetRun();
    UFUNCTION(BlueprintCallable) void ApplySettings();
    UPROPERTY(BlueprintReadOnly) int32 Shards = 0;
    UPROPERTY(BlueprintReadOnly) int32 Relics = 0;
    UPROPERTY(BlueprintReadOnly) int32 Section = 0;
    UPROPERTY(BlueprintReadOnly) float RunSeconds = 0;
    UPROPERTY(BlueprintReadOnly) bool Won = false;
    UPROPERTY() TObjectPtr<UPOCSaveGame> Save;
    TSet<int32> Collected;
};
