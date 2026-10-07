#include "POCGameMode.h"
#include "POCCharacter.h"
#include "POCWorld.h"
#include "POCGameInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraActor.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

APOCGameMode::APOCGameMode()
{
    DefaultPawnClass = APOCCharacter::StaticClass();
    PlayerControllerClass = APOCController::StaticClass();
    HUDClass = APOCHUD::StaticClass();
}
void APOCGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    if (UClass* Class = LoadClass<APOCCharacter>(nullptr, TEXT("/Game/Characters/BP_Nori.BP_Nori_C"))) DefaultPawnClass = Class;
}
void APOCGameMode::StartPlay()
{
    Super::StartPlay();
    GetWorld()->SpawnActor<APOCWorld>();
}

APOCController::APOCController()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    bShouldPerformFullTickWhenPaused = true;
}
void APOCController::BeginPlay()
{
    Super::BeginPlay();
    PlayerCameraManager->ViewPitchMin = -65;
    PlayerCameraManager->ViewPitchMax = 28;
}
void APOCController::SetupInputComponent()
{
    Super::SetupInputComponent();
    auto& PauseBinding = InputComponent->BindAction(TEXT("Pause"), IE_Pressed, this, &APOCController::TogglePause);
    PauseBinding.bExecuteWhenPaused = true;
}
void APOCController::OpenMenu(EPOCMenu NewMenu)
{
    Menu = NewMenu; Selection = 0;
    const bool Playing = Menu == EPOCMenu::Playing;
    bShowMouseCursor = !Playing;
    SetPause(!Playing);
    if (Playing)
    {
        FInputModeGameOnly Mode; SetInputMode(Mode);
        if (auto* Player = Cast<APOCCharacter>(GetPawn())) Player->GetCharacterMovement()->StopMovementImmediately();
    }
    else
    {
        FInputModeGameAndUI Mode;
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        SetInputMode(Mode);
    }
    FlushPressedKeys();
    if (NewMenu == EPOCMenu::Title && Journey && Journey->IntroCamera) SetViewTarget(Journey->IntroCamera);
}
void APOCController::TogglePause()
{
    if (Menu == EPOCMenu::Settings || Menu == EPOCMenu::Controls)
    {
        GetGameInstance<UPOCGameInstance>()->ApplySettings(); OpenMenu(ReturnMenu);
    }
    else if (Menu == EPOCMenu::Playing) OpenMenu(EPOCMenu::Pause);
    else if (Menu == EPOCMenu::Pause) OpenMenu(EPOCMenu::Playing);
}

TArray<FString> APOCController::MenuLabels() const
{
    const auto* GI = GetGameInstance<UPOCGameInstance>();
    if (Menu == EPOCMenu::Title) return { TEXT("Begin the journey"), TEXT("Settings"), TEXT("How to play") };
    if (Menu == EPOCMenu::Pause) return { TEXT("Keep going"), TEXT("Restart checkpoint"), TEXT("Settings"), TEXT("How to play"), TEXT("Start a new journey") };
    if (Menu == EPOCMenu::Complete) return { TEXT("One more slice?"), TEXT("Settings") };
    if (Menu == EPOCMenu::Controls) return { TEXT("Back") };
    if (Menu == EPOCMenu::Settings)
        return {
            FString::Printf(TEXT("Music                         %d%%"), FMath::RoundToInt(GI->Save->MusicVolume * 100)),
            FString::Printf(TEXT("Sound effects                %d%%"), FMath::RoundToInt(GI->Save->EffectsVolume * 100)),
            FString::Printf(TEXT("Camera sensitivity          %.2f"), GI->Save->Sensitivity),
            FString::Printf(TEXT("Subtitles                     %s"), GI->Save->Subtitles ? TEXT("On") : TEXT("Off")),
            FString::Printf(TEXT("Invert camera Y              %s"), GI->Save->InvertY ? TEXT("On") : TEXT("Off")),
            FString::Printf(TEXT("Graphics                      %s"), GI->Save->Quality == 0 ? TEXT("Low") : GI->Save->Quality == 1 ? TEXT("Medium") : GI->Save->Quality == 2 ? TEXT("High") : TEXT("Epic")),
            FString::Printf(TEXT("Display                        %s"), GI->Save->Fullscreen ? TEXT("Fullscreen") : TEXT("Windowed")),
            TEXT("Save and back") };
    return {};
}

void APOCController::AdjustSetting(int32 Direction)
{
    auto* Save = GetGameInstance<UPOCGameInstance>()->Save.Get();
    switch (Selection)
    {
    case 0: Save->MusicVolume = FMath::Clamp(Save->MusicVolume + Direction * .05f, 0.f, 1.f); break;
    case 1: Save->EffectsVolume = FMath::Clamp(Save->EffectsVolume + Direction * .05f, 0.f, 1.f); break;
    case 2: Save->Sensitivity = FMath::Clamp(Save->Sensitivity + Direction * .1f, .25f, 2.f); break;
    case 3: Save->Subtitles = !Save->Subtitles; break;
    case 4: Save->InvertY = !Save->InvertY; break;
    case 5: Save->Quality = FMath::Clamp(Save->Quality + Direction, 0, 3); break;
    case 6: Save->Fullscreen = !Save->Fullscreen; break;
    default: break;
    }
}

void APOCController::ActivateSelection()
{
    auto* GI = GetGameInstance<UPOCGameInstance>();
    auto OpenSettings = [this]() { ReturnMenu = Menu; OpenMenu(EPOCMenu::Settings); };
    auto OpenControls = [this]() { ReturnMenu = Menu; OpenMenu(EPOCMenu::Controls); };
    if (Menu == EPOCMenu::Settings)
    {
        if (Selection == 7) { GI->ApplySettings(); OpenMenu(ReturnMenu); }
        else AdjustSetting(1);
    }
    else if (Menu == EPOCMenu::Controls) OpenMenu(ReturnMenu);
    else if (Menu == EPOCMenu::Title)
    {
        if (Selection == 0 && Journey && Journey->Ready)
        { HasStarted = true; GI->ApplySettings(); OpenMenu(EPOCMenu::Playing); SetViewTargetWithBlend(GetPawn(), .8f); Journey->ShowCaption(TEXT("There it is. Just up there. How hard could it be?"), 6); }
        else if (Selection == 1) OpenSettings();
        else if (Selection == 2) OpenControls();
    }
    else if (Menu == EPOCMenu::Pause)
    {
        if (Selection == 0) OpenMenu(EPOCMenu::Playing);
        else if (Selection == 1) { OpenMenu(EPOCMenu::Playing); if (auto* Player = Cast<APOCCharacter>(GetPawn())) Player->Respawn(); }
        else if (Selection == 2) OpenSettings();
        else if (Selection == 3) OpenControls();
        else if (Selection == 4) { GI->ResetRun(); SetPause(false); UGameplayStatics::OpenLevel(this, TEXT("L_LongWayToCake")); }
    }
    else if (Menu == EPOCMenu::Complete)
    {
        if (Selection == 0) { GI->ResetRun(); SetPause(false); UGameplayStatics::OpenLevel(this, TEXT("L_LongWayToCake")); }
        else OpenSettings();
    }
}

void APOCController::PlayerTick(float DeltaSeconds)
{
    Super::PlayerTick(DeltaSeconds);
    if (!Journey) for (TActorIterator<APOCWorld> It(GetWorld()); It; ++It) { Journey = *It; break; }
    if (!Initialized && Journey && GetPawn())
    {
        Initialized = true;
        OpenMenu(EPOCMenu::Title);
    }
    if (Menu == EPOCMenu::Playing && GetGameInstance<UPOCGameInstance>()->Won) OpenMenu(EPOCMenu::Complete);
    if (Menu != EPOCMenu::Playing)
    {
        const int32 Count = MenuLabels().Num();
        if (Count == 0) return;
        if (WasInputKeyJustPressed(EKeys::Down) || WasInputKeyJustPressed(EKeys::Gamepad_DPad_Down)) Selection = (Selection + 1) % Count;
        if (WasInputKeyJustPressed(EKeys::Up) || WasInputKeyJustPressed(EKeys::Gamepad_DPad_Up)) Selection = (Selection + Count - 1) % Count;
        if (Menu == EPOCMenu::Settings)
        {
            if (WasInputKeyJustPressed(EKeys::Left) || WasInputKeyJustPressed(EKeys::Gamepad_DPad_Left)) AdjustSetting(-1);
            if (WasInputKeyJustPressed(EKeys::Right) || WasInputKeyJustPressed(EKeys::Gamepad_DPad_Right)) AdjustSetting(1);
        }
        float MouseX = 0, MouseY = 0;
        if (GetMousePosition(MouseX, MouseY))
        {
            const FVector2D Mouse(MouseX, MouseY);
            if (auto* HUD = Cast<APOCHUD>(GetHUD()))
            {
                const int32 Hover = HUD->HitMenu(Mouse);
                if (Hover >= 0 && !Mouse.Equals(PreviousMouse, .5)) Selection = Hover;
                if (Hover >= 0 && WasInputKeyJustPressed(EKeys::LeftMouseButton)) { Selection = Hover; ActivateSelection(); }
                if (Menu == EPOCMenu::Settings && Hover >= 0 && WasInputKeyJustPressed(EKeys::RightMouseButton)) { Selection = Hover; AdjustSetting(-1); }
            }
            PreviousMouse = Mouse;
        }
        if (WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom)) ActivateSelection();
        if (WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right)) TogglePause();
    }
#if !UE_BUILD_SHIPPING
    if (WasInputKeyJustPressed(EKeys::F3)) ShowFPS = !ShowFPS;
    if (Journey && IsInputKeyDown(EKeys::LeftAlt))
    {
        const FKey Keys[] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight};
        for (int32 Index = 0; Index < 8; ++Index)
            if (WasInputKeyJustPressed(Keys[Index])) { OpenMenu(EPOCMenu::Playing); Journey->TeleportSection(Index); }
    }
#endif
}

void APOCHUD::Text(const FString& Value, float X, float Y, float Scale, FLinearColor Color, bool Center)
{
    UFont* Font = GEngine->GetLargeFont();
    float W = 0, H = 0;
    GetTextSize(Value, W, H, Font, Scale * UIScale);
    DrawText(Value, Color, X * UIScale - (Center ? W * .5 : 0), Y * UIScale, Font, Scale * UIScale, false);
}
int32 APOCHUD::HitMenu(FVector2D Mouse) const
{
    for (int32 Index = 0; Index < ButtonRects.Num(); ++Index) if (ButtonRects[Index].IsInside(Mouse)) return Index;
    return -1;
}

void APOCHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;
    auto* PC = Cast<APOCController>(PlayerOwner);
    if (!PC) return;
    auto* GI = GetGameInstance<UPOCGameInstance>();
    auto* Player = Cast<APOCCharacter>(PC->GetPawn());
    UIScale = FMath::Min(Canvas->SizeX / 1280.f, Canvas->SizeY / 720.f);
    const float W = Canvas->SizeX / UIScale, H = Canvas->SizeY / UIScale;
    const FLinearColor Ivory(.95, .91, .79), Muted(.55, .66, .67), Gold(.9, .65, .32), Dark(.025, .055, .075, .94);
    ButtonRects.Reset();
    if (PC->Menu == EPOCMenu::Playing)
    {
        Text(FString::Printf(TEXT("%03d  /  576     MEMORY SHARDS"), GI->Shards), 40, 30, .52, Ivory);
        Text(FString::Printf(TEXT("%d / 8  RELICS"), GI->Relics), 40, 56, .4, Gold);
        FString Hearts;
        for (int32 I = 0; I < (Player ? Player->Hearts : 3); ++I) Hearts += TEXT("o  ");
        Text(Hearts, W - 150, 30, .7, Gold);
        if (PC->Journey)
        {
            Text(PC->Journey->SectionName, W * .5, 28, .4, Muted, true);
            Text(PC->Journey->Prompt, W * .5, H - 110, .55, Ivory, true);
            if (GI->Save->Subtitles && !PC->Journey->Caption.IsEmpty())
            {
                DrawRect(FLinearColor(0, 0, 0, .55), (W * .5 - 450) * UIScale, (H - 65) * UIScale, 900 * UIScale, 42 * UIScale);
                Text(PC->Journey->Caption, W * .5, H - 57, .48, Ivory, true);
            }
        }
        if (GI->Section == 0 && GI->RunSeconds < 40)
            Text(TEXT("WASD  move     SPACE  jump     SHIFT  sprint     J / click  bonk     C  slide / slam     E  Echo"), W * .5, H - 155, .4, Muted, true);
    }
    else
    {
        DrawRect(FLinearColor(.015, .03, .045, .72), 0, 0, Canvas->SizeX, Canvas->SizeY);
        const bool Settings = PC->Menu == EPOCMenu::Settings;
        const bool Controls = PC->Menu == EPOCMenu::Controls;
        Text(TEXT("THE LONG WAY TO CAKE"), W * .5, Settings ? 50 : 85, .42, Gold, true);
        const FString Title = Settings ? TEXT("Make yourself comfortable.") : Controls ? TEXT("Small paws. Big possibilities.") : PC->Menu == EPOCMenu::Pause ? TEXT("Catch your breath.") : TEXT("PIECE OF CAKE");
        Text(Title, W * .5, Settings ? 80 : 120, Settings || Controls ? 1.1 : 1.65, Ivory, true);
        if (PC->Menu == EPOCMenu::Title) Text(TEXT("An enormous journey. A perfectly ordinary dessert."), W * .5, 205, .52, Muted, true);
        if (PC->Menu == EPOCMenu::Complete)
        {
            Text(TEXT("It was, in fact, a piece of cake."), W * .5, 215, .6, Ivory, true);
            Text(FString::Printf(TEXT("%d:%02d     |     %d memory shards     |     %d relics"), int32(GI->RunSeconds) / 60, int32(GI->RunSeconds) % 60, GI->Shards, GI->Relics), W * .5, 260, .48, Gold, true);
        }
        if (Controls)
        {
            const TCHAR* Lines[] = {TEXT("WASD / left stick       Move"), TEXT("Mouse / right stick     Camera"), TEXT("Space / A       Jump  (hold for height)"), TEXT("Shift / LB       Sprint"), TEXT("J or left click / X       Bonk  (also in the air)"), TEXT("C or Ctrl / B       Slide on land; slam in the air"), TEXT("E / Y       Echo or eat cake"), TEXT("Esc / Menu       Pause          R       Checkpoint")};
            for (int32 I = 0; I < 8; ++I) Text(Lines[I], W * .5, 210 + I * 34, .48, I % 2 ? Muted : Ivory, true);
        }
        const auto Labels = PC->MenuLabels();
        const float StartY = Settings ? 160 : Controls ? 535 : PC->Menu == EPOCMenu::Pause ? 260 : 330;
        const float Row = Settings ? 52 : 58;
        for (int32 Index = 0; Index < Labels.Num(); ++Index)
        {
            const float X = W * .5 - 250, Y = StartY + Index * Row;
            const bool Selected = PC->Selection == Index;
            DrawRect(Selected ? FLinearColor(.75, .52, .25, .95) : Dark, X * UIScale, Y * UIScale, 500 * UIScale, 44 * UIScale);
            Text(Labels[Index], W * .5, Y + 10, .55, Selected ? FLinearColor(.04, .055, .065) : Ivory, true);
            ButtonRects.Add(FBox2D(FVector2D(X, Y) * UIScale, FVector2D(X + 500, Y + 44) * UIScale));
        }
        Text(Settings ? TEXT("Left / right to adjust  |  Click + / right-click -  |  Esc to save") : TEXT("Arrow keys / D-pad to choose     Enter / A to select"), W * .5, H - 45, .38, Muted, true);
        if (PC->Journey && !PC->Journey->Ready) Text(PC->Journey->LoadError, W * .5, H - 85, .45, FLinearColor(1, .4, .2), true);
    }
#if !UE_BUILD_SHIPPING
    if (PC->ShowFPS) Text(FString::Printf(TEXT("%.1f fps | %.2f ms | Alt + 1..8: section"), 1.f / FMath::Max(GetWorld()->GetDeltaSeconds(), .001f), GetWorld()->GetDeltaSeconds() * 1000), 30, H - 25, .4, Gold);
#endif
}
