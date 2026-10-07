#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "POCGameMode.generated.h"

class APOCWorld;

UENUM()
enum class EPOCMenu : uint8 { Title, Playing, Pause, Settings, Complete, Controls };

UCLASS()
class PIECEOFCAKE_API APOCGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    APOCGameMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
    virtual void StartPlay() override;
};

UCLASS()
class PIECEOFCAKE_API APOCController : public APlayerController
{
    GENERATED_BODY()
public:
    APOCController();
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaSeconds) override;
    UFUNCTION(BlueprintCallable) void TogglePause();
    void OpenMenu(EPOCMenu NewMenu);
    void ActivateSelection();
    void AdjustSetting(int32 Direction);
    TArray<FString> MenuLabels() const;
    UPROPERTY() TObjectPtr<APOCWorld> Journey;
    EPOCMenu Menu = EPOCMenu::Title;
    EPOCMenu ReturnMenu = EPOCMenu::Title;
    int32 Selection = 0;
    bool HasStarted = false;
    bool ShowFPS = false;
private:
    bool Initialized = false;
    FVector2D PreviousMouse = FVector2D(-1, -1);
};

UCLASS()
class PIECEOFCAKE_API APOCHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    int32 HitMenu(FVector2D Mouse) const;
private:
    void Text(const FString& Value, float X, float Y, float Scale, FLinearColor Color, bool Center = false);
    TArray<FBox2D> ButtonRects;
    float UIScale = 1;
};
