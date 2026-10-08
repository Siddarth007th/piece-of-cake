#include "POCGameMode.h"
#include "InputKeyEventArgs.h"
#include "HAL/PlatformTime.h"
#include "POCJourneyTest.h"
#include "POCFlightTest.h"
#include "POCCameraTest.h"
#include "POCPresentationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "POCCharacter.h"
#include "POCWorld.h"
#include "POCGameInstance.h"
#include "POCCloud.h"
#include "Camera/PlayerCameraManager.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Engine/Texture.h"
#include "Engine/Font.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
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
    // Public preview keeps the requested in-game flight code, but exposes no engine console.
    if(FParse::Param(FCommandLine::Get(),TEXT("POCPublicSession")) && GetWorld()->GetGameViewport())
        GetWorld()->GetGameViewport()->ViewportConsole=nullptr;
    // Establish the final render preset before constructing the world, so starting
    // a journey does not invalidate warmed render state.
    GetGameInstance<UPOCGameInstance>()->ApplySettings();
    GetWorld()->SpawnActor<APOCWorld>();
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("POCPresentationTest")) || FString(FCommandLine::Get()).Contains(TEXT("POCCloudSmoke"))) GetWorld()->SpawnActor<APOCPresentationTest>();
    if(FParse::Param(FCommandLine::Get(),TEXT("POCCameraTest"))) GetWorld()->SpawnActor<APOCCameraTest>();
    if(FParse::Param(FCommandLine::Get(),TEXT("POCFlightTest"))) GetWorld()->SpawnActor<APOCFlightTest>();
    int32 TestRun = 0;
    if (FParse::Value(FCommandLine::Get(), TEXT("POCAutoRun="), TestRun) && TestRun > 0)
        GetWorld()->SpawnActor<APOCJourneyTest>();
#endif
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
bool APOCController::InputKey(const FInputKeyEventArgs& Params)
{
#if !UE_BUILD_SHIPPING
    // Engine traversal owns input; typing in another app must not pause a measurement.
    if(FString(FCommandLine::Get()).Contains(TEXT("POCAutoRun="))) return true;
#endif
    if (CodeKeys.Contains(Params.Key))
    {
        if (Params.Event == IE_Released) CodeKeys.Remove(Params.Key);
        if (Params.Event != IE_Pressed) return true;
    }
    if (Menu == EPOCMenu::Playing || Menu == EPOCMenu::Pause)
    {
        const FString Key = Params.Key.GetFName().ToString();
        if (Params.Event == IE_Pressed && Key.Len() == 1 && FChar::IsAlpha(Key[0]))
        {
            bool Consume = false;
            const bool Toggle = DeveloperCode.Push(Key[0], FPlatformTime::Seconds(), Consume);
            if (Consume)
            {
                if (auto* Player = Cast<APOCCharacter>(GetPawn()))
                {
                    Player->ResetHeldInput();
                    Player->GetCharacterMovement()->StopMovementImmediately();
                    if (Toggle) Player->SetDeveloperFlight(!Player->DeveloperFlight);
                }
                CodeKeys.Add(Params.Key);
                return true;
            }
        }
        else if (Params.Event == IE_Pressed && !Params.Key.IsModifierKey()) DeveloperCode.Reset();
    }
    else DeveloperCode.Reset();
    return Super::InputKey(Params);
}

void APOCController::OpenMenu(EPOCMenu NewMenu)
{
    DeveloperCode.Reset(); CodeKeys.Reset();
    Menu = NewMenu; Selection = 0;
    const bool Playing = Menu == EPOCMenu::Playing;
    bShowMouseCursor = !Playing;
    SetPause(!Playing && HasStarted);
    if (Playing)
    {
        FInputModeGameOnly Mode; SetInputMode(Mode);
    }
    else
    {
        if (auto* Player = Cast<APOCCharacter>(GetPawn())) Player->ResetHeldInput();
        FInputModeGameAndUI Mode;
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        Mode.SetHideCursorDuringCapture(false);
        SetInputMode(Mode);
    }
    FlushPressedKeys();
    if (NewMenu == EPOCMenu::Title && Journey && Journey->IntroCamera) SetViewTarget(Journey->IntroCamera);
    if(NewMenu==EPOCMenu::Complete && Journey)
        if(auto* Player=Cast<APOCCharacter>(GetPawn()))
        {
            // Only the completed-game portrait cuts camera; gameplay keeps its manual camera.
            const FVector P=Player->GetActorLocation();
            const FVector Eye=P+Player->GetActorForwardVector()*160+Player->GetActorRightVector()*390+FVector(0,0,140);
            const FVector ViewRight=FRotationMatrix((P-Eye).Rotation()).GetUnitAxis(EAxis::Y);
            const FVector Target=P-ViewRight*75+FVector(0,0,32);
            Player->SetActorRotation(FRotator(0,(Eye-P).Rotation().Yaw,0));
            if(!Journey->RewardCamera) Journey->RewardCamera=GetWorld()->SpawnActor<ACameraActor>(Eye,(Target-Eye).Rotation());
            Journey->RewardCamera->GetCameraComponent()->SetFieldOfView(50);
            SetViewTarget(Journey->RewardCamera);
        }

}
void APOCController::TogglePause()
{
    if (Menu == EPOCMenu::Settings || Menu == EPOCMenu::Controls || Menu == EPOCMenu::Cloud)
    {
        GetGameInstance<UPOCGameInstance>()->ApplySettings(); OpenMenu(ReturnMenu);
    }
    else if (Menu == EPOCMenu::Playing) OpenMenu(EPOCMenu::Pause);
    else if (Menu == EPOCMenu::Pause) OpenMenu(EPOCMenu::Playing);
}

TArray<FString> APOCController::MenuLabels() const
{
    const auto* GI = GetGameInstance<UPOCGameInstance>();
    if (Menu == EPOCMenu::Title) return { TEXT("Start adventure"), TEXT("Settings"), TEXT("How to play"), TEXT("Cloud saves"), TEXT("Quit game") };
    if (Menu == EPOCMenu::Pause) return { TEXT("Keep going"), TEXT("Restart checkpoint"), TEXT("Settings"), TEXT("How to play"), TEXT("Start a new journey") };
    if (Menu == EPOCMenu::Complete) return { TEXT("One more slice?"), TEXT("Settings") };
    if (Menu == EPOCMenu::Controls) return { TEXT("Back") };
    if (Menu == EPOCMenu::Cloud) return { GI->Cloud->Connected() ? TEXT("Sync and refresh") : TEXT("Connect cloud profile"), TEXT("Back") };
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
    else if (Menu == EPOCMenu::Cloud) { if(Selection==0) GI->Cloud->Connect(); else OpenMenu(ReturnMenu); }
    else if (Menu == EPOCMenu::Title)
    {
        if (Selection == 0 && Journey && Journey->Ready)
        {
            HasStarted=true; OpenMenu(EPOCMenu::Playing);
            Menu=EPOCMenu::Dream; DreamTime=0; Journey->AnimateDream(0); Journey->ShowDream(true);
            if(auto* P=Cast<APOCCharacter>(GetPawn())) { P->Dreaming=true; P->GetCharacterMovement()->DisableMovement(); }
            SetViewTargetWithBlend(Journey->DreamCamera,.5f);
        }
        else if (Selection == 1) OpenSettings();
        else if (Selection == 2) OpenControls();
        else if (Selection == 3) {ReturnMenu=Menu;OpenMenu(EPOCMenu::Cloud);GI->Cloud->RefreshBoard();}
        else if (Selection == 4) FPlatformMisc::RequestExit(false);
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

void APOCController::FinishDream()
{
    if(Journey) Journey->ShowDream(false);
    if(auto* P=Cast<APOCCharacter>(GetPawn())) { P->Dreaming=false; P->SetActorRotation(FRotator(0,Journey->Route[0].Yaw,0)); P->InvulnerableRemaining=0; P->ResetHeldInput();P->GetCharacterMovement()->SetMovementMode(MOVE_Walking); }
    OpenMenu(EPOCMenu::Playing); SetViewTargetWithBlend(GetPawn(),.8f);
    Journey->ShowCaption(TEXT("One dream. One cake. Collect shards to open the final door."),5);
}

void APOCController::PlayerTick(float DeltaSeconds)
{
    Super::PlayerTick(DeltaSeconds);
    if (!Journey)
    {
        TActorIterator<APOCWorld> It(GetWorld());
        if (It) Journey = *It;
    }
    if (!Initialized && Journey && Journey->Ready && GetPawn())
    {
        Initialized = true;
        auto* Player=Cast<APOCCharacter>(GetPawn());
        Journey->Player=Player; Player->Journey=Journey;
        Player->Checkpoint=FTransform(FRotator(0,Journey->Route[0].Yaw,0),Journey->Route[0].Position+FVector(0,0,52));
        Player->SetActorLocation(Journey->Route[0].Position+FVector(0,0,44)); Player->SetActorRotation(FRotator(0,Journey->Route[0].Yaw+135,0)); Player->Dreaming=true; Player->InvulnerableRemaining=0; Player->GetCharacterMovement()->DisableMovement();
        OpenMenu(EPOCMenu::Title);
    }
    if(Menu==EPOCMenu::Dream)
    {
        DreamTime+=DeltaSeconds;
        Journey->AnimateDream(DreamTime);
        if(DreamTime>5.5f || WasInputKeyJustPressed(EKeys::Enter) || WasInputKeyJustPressed(EKeys::SpaceBar) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom)) FinishDream();
        return;
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

APOCHUD::APOCHUD()
{
    static ConstructorHelpers::FObjectFinder<UFont> Font(TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    InterfaceFont = Font.Object;
}

void APOCHUD::Text(const FString& Value, float X, float Y, float Scale, FLinearColor Color, bool Center)
{
    // Rasterize at the actual display size instead of enlarging a small bitmap atlas.
    const FSlateFontInfo Font(InterfaceFont, FMath::Max(11, FMath::RoundToInt(Scale * 30.f * UIScale)),
        Scale >= 1.f ? TEXT("Bold") : TEXT("Regular"));
    FCanvasTextItem Item(FVector2D(X * UIScale, Y * UIScale), FText::FromString(Value), Font, Color);
    Item.bCentreX = Center;
    Canvas->DrawItem(Item);
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
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("POCSceneryTour"))) return;
#endif
    if(PC->Menu==EPOCMenu::Title)
    {
        // A readable game logo and simple menu leave most of the live scene to Nori.
        for(int32 I=0;I<50;++I)
            DrawRect(FLinearColor(.009,.025,.04,.90f*FMath::Pow(1-I/50.f,1.4f)),I*15*UIScale,0,15*UIScale,Canvas->SizeY);
        Text(TEXT("A NORI ADVENTURE"),76,116,.44,Ivory);
        Text(TEXT("PIECE OF"),72,156,1.5,FLinearColor(.015,.05,.065));
        Text(TEXT("PIECE OF"),68,150,1.5,Ivory);
        Text(TEXT("CAKE"),68,220,2.85,FLinearColor(.015,.05,.065));
        Text(TEXT("CAKE"),64,212,2.85,Gold);
        DrawRect(Gold,76*UIScale,321*UIScale,68*UIScale,3*UIScale);
        Text(TEXT("Small paws. One enormous adventure."),76,345,.43,Ivory);
        const auto Labels=PC->MenuLabels();
        for(int32 Index=0;Index<Labels.Num();++Index)
        {
            const float X=76,Y=384+Index*52;
            const bool Selected=PC->Selection==Index;
            DrawRect(Selected?Gold:FLinearColor(.02,.07,.09,.6),X*UIScale,Y*UIScale,404*UIScale,42*UIScale);
            if(Selected) Text(TEXT(">"),92,Y+9,.52,Dark);
            Text(Labels[Index],Selected?120:96,Y+9,.52,Selected?Dark:Ivory);
            ButtonRects.Add(FBox2D(FVector2D(X,Y)*UIScale,FVector2D(X+404,Y+42)*UIScale));
        }
        Text(TEXT("ARROWS choose    ENTER play    /    Click to select"),76,H-40,.32,Muted);
        Text(TEXT("NORI"),W-235,H-95,.65,Ivory);
        Text(TEXT("NINJA. RABBIT. DESSERT ENTHUSIAST."),W-397,H-58,.30,Gold);
        if(PC->Journey && !PC->Journey->Ready) Text(PC->Journey->LoadError,76,H-73,.34,FLinearColor(1,.4,.2));
        return;
    }
    if(PC->Menu==EPOCMenu::Complete)
    {
        for(int32 I=0;I<55;++I)
            DrawRect(FLinearColor(.025,.020,.035,.93f*FMath::Pow(1-I/55.f,1.3f)),I*15*UIScale,0,15*UIScale,Canvas->SizeY);
        Text(TEXT("THE LONG WAY WAS WORTH IT"),70,118,.42,Gold);
        Text(TEXT("SWEET"),64,167,2.0,Ivory);
        Text(TEXT("VICTORY."),64,242,2.0,Gold);
        Text(TEXT("One small rabbit. One well-earned slice."),70,337,.43,Ivory);
        Text(FString::Printf(TEXT("%d:%02d  JOURNEY     %d  SHARDS     %d / 8  RELICS"),int32(GI->RunSeconds)/60,int32(GI->RunSeconds)%60,GI->Shards,GI->Relics),70,385,.40,Gold);
        Text(GI->AssistedRun?TEXT("Developer-assisted ending / unranked"):GI->Cloud?GI->Cloud->Status:TEXT("Adventure complete"),70,423,.34,Muted);
        const auto Labels=PC->MenuLabels();
        for(int32 I=0;I<Labels.Num();++I)
        {
            const float X=70,Y=488+I*56;const bool Selected=PC->Selection==I;
            DrawRect(Selected?Gold:Dark,X*UIScale,Y*UIScale,430*UIScale,43*UIScale);
            Text(Labels[I],X+24,Y+9,.52,Selected?Dark:Ivory);
            ButtonRects.Add(FBox2D(FVector2D(X,Y)*UIScale,FVector2D(X+430,Y+43)*UIScale));
        }
        Text(TEXT("ARROWS choose    ENTER select"),70,H-43,.32,Muted);
        return;
    }
    if(PC->Menu==EPOCMenu::Cloud)
    {
        DrawRect(FLinearColor(.018,.045,.065,.97),0,0,Canvas->SizeX,Canvas->SizeY);
        Text(TEXT("YOUR CLOUD PROFILE"),W*.5,68,1.1,Ivory,true);
        Text(GI->Cloud->Status,W*.5,125,.5,Gold,true);
        Text(TEXT("Best shards, relics and completed runs are saved online."),W*.5,172,.4,Ivory,true);
        Text(TEXT("This installation has its own private profile, remembered in Mac Keychain."),W*.5,202,.34,Muted,true);
        Text(TEXT("CASUAL LEADERBOARD"),W*.5,258,.5,Gold,true);
        if(GI->Cloud->Board.IsEmpty()) Text(GI->Cloud->Connected()?TEXT("No ranked runs to display. Finish a journey or sync to refresh."):TEXT("Connect your profile to see the board."),W*.5,305,.4,Muted,true);
        for(int32 I=0;I<FMath::Min(6,GI->Cloud->Board.Num());++I) Text(GI->Cloud->Board[I],W*.5,300+I*29,.43,Ivory,true);
        Text(TEXT("Client-reported runs. Developer-flight runs are excluded."),W*.5,495,.32,Muted,true);
        const auto Labels=PC->MenuLabels();
        for(int32 I=0;I<Labels.Num();++I) {
            const float X=W*.5-230,Y=539+I*53;const bool Selected=PC->Selection==I;
            DrawRect(Selected?Gold:Dark,X*UIScale,Y*UIScale,460*UIScale,42*UIScale);
            Text(Labels[I],W*.5,Y+9,.5,Selected?Dark:Ivory,true);
            ButtonRects.Add(FBox2D(FVector2D(X,Y)*UIScale,FVector2D(X+460,Y+42)*UIScale));
        }
        Text(TEXT("No email, payment details or gameplay video are uploaded."),W*.5,H-36,.32,Muted,true);
        return;
    }
    if(PC->Menu==EPOCMenu::Dream)
    {
        DrawRect(FLinearColor(.025,.04,.09,.13),0,0,Canvas->SizeX,Canvas->SizeY);
        DrawRect(FLinearColor(.025,.035,.06,.95),0,0,Canvas->SizeX,48*UIScale);
        DrawRect(FLinearColor(.025,.035,.06,.95),0,(H-105)*UIScale,Canvas->SizeX,105*UIScale);
        Text(PC->DreamTime<2.2f?TEXT("Nori had one very important dream..."):TEXT("A slice of cake. All to himself."),W*.5,H-83,.8,Ivory,true);
        Text(TEXT("SPACE / ENTER  to wake up"),W*.5,H-37,.4,Gold,true);
        return;
    }
    if (PC->Menu == EPOCMenu::Playing)
    {
        Text(FString::Printf(TEXT("%03d     SHARDS"), GI->Shards-GI->ShardsSpent), 40, 30, .52, Ivory);
        Text(FString::Printf(TEXT("%d / 8  RELICS"), GI->Relics), 40, 56, .4, Gold);
        // Pixel hearts are drawn directly, independent of font glyph support.
        const char* HeartRows[]={"011000110","111101111","111111111","111111111","011111110","001111100","000111000","000010000"};
        TArray<FCanvasUVTri> HeartTriangles; HeartTriangles.Reserve(300);
        for(int32 Heart=0;Heart<3;++Heart)
            for(int32 Y=0;Y<8;++Y) for(int32 X=0;X<9;++X)
            {
                if(HeartRows[Y][X]!='1') continue;
                const bool Edge=X==0||X==8||Y==0||Y==7||HeartRows[Y-1][X]!='1'||HeartRows[Y+1][X]!='1'||HeartRows[Y][X-1]!='1'||HeartRows[Y][X+1]!='1';
                const bool Full=Heart<(Player?Player->Hearts:3);
                FLinearColor Color=Edge?FLinearColor(.14,.04,.075):Full?FLinearColor(.95,.13,.23):FLinearColor(.22,.20,.26);
                if(Full&&!Edge&&Y==2&&(X==2||X==6)) Color=FLinearColor(1,.66,.67);
                const FVector2D P((W-156+Heart*39+X*3)*UIScale,(31+Y*3)*UIScale);
                const FVector2D DX(3*UIScale,0), DY(0,3*UIScale);
                FCanvasUVTri A,B;
                A.V0_Pos=P;A.V1_Pos=P+DX;A.V2_Pos=P+DY;
                B.V0_Pos=P+DX;B.V1_Pos=P+DX+DY;B.V2_Pos=P+DY;
                A.V0_Color=A.V1_Color=A.V2_Color=B.V0_Color=B.V1_Color=B.V2_Color=Color;
                HeartTriangles.Add(A);HeartTriangles.Add(B);
            }
        FCanvasTriangleItem HeartItem(HeartTriangles,GWhiteTexture);
        HeartItem.BlendMode=SE_BLEND_Translucent;
        Canvas->DrawItem(HeartItem);
        if(Player && Player->DeveloperFlight)
        {
            DrawRect(FLinearColor(.015,.09,.12,.92),(W*.5f-300)*UIScale,65*UIScale,600*UIScale,88*UIScale);
            Text(TEXT("DEVELOPER FLIGHT  /  NO CLIP"),W*.5f,73,.55,FLinearColor(.3,1,.83),true);
            Text(TEXT("WASD move   SPACE up   C / CTRL down   SHIFT fast"),W*.5f,104,.38,Ivory,true);
            Text(TEXT("Type siddarthisgod: land here   R: checkpoint   E: cake"),W*.5f,128,.34,Muted,true);
            Text(FString::Printf(TEXT("X %.0f   Y %.0f   Z %.0f   /   F: mouse camera   X: center"),Player->GetActorLocation().X,Player->GetActorLocation().Y,Player->GetActorLocation().Z),W*.5f,160,.35,Ivory,true);
        }
        if(Player && Player->Dying)
        {
            DrawRect(FLinearColor(.25,.015,.025,.22),0,0,Canvas->SizeX,Canvas->SizeY);
            Text(TEXT("BACK TO CHECKPOINT"),W*.5,H*.4,.75,Ivory,true);
        }
#if !UE_BUILD_SHIPPING
        if(FParse::Param(FCommandLine::Get(),TEXT("POCAutoRun")) || FString(FCommandLine::Get()).Contains(TEXT("POCAutoRun=")))
            Text(TEXT("AUTOMATED PLAYTEST"),W*.5,78,.35,Gold,true);
#endif
        if(Player && !Player->DeveloperFlight)
        {
            Text(Player->DashCooldown<=0 ? TEXT("Q  DASH READY") : TEXT("Q  RECHARGING"),W-200,68,.4,Player->DashCooldown<=0 ? FLinearColor(.35,.9,.8) : Muted);
            DrawRect(FLinearColor(.15,.3,.32,.8),(W-200)*UIScale,94*UIScale,145*UIScale,3*UIScale);
            DrawRect(FLinearColor(.3,.9,.8),(W-200)*UIScale,94*UIScale,145*UIScale*(1-FMath::Clamp(Player->DashCooldown/.9f,0.f,1.f)),3*UIScale);
        }
        if(Player && !Player->DeveloperFlight)
            Text(Player->IsMouseCameraActive() ? TEXT("MOUSE CAMERA ON  /  F to lock  /  X center") : TEXT("F: toggle mouse look  /  ARROWS: camera  /  X: center"),W*.5,56,.34,Player->IsMouseCameraActive()?Gold:Muted,true);
        if (PC->Journey)
        {
            Text(PC->Journey->SectionName, W * .5, 28, .4, Muted, true);
            if(!Player || !Player->DeveloperFlight) Text(PC->Journey->Prompt, W * .5, H - 110, .55, Ivory, true);
            if (GI->Save->Subtitles && !PC->Journey->Caption.IsEmpty())
            {
                DrawRect(FLinearColor(0, 0, 0, .55), (W * .5 - 450) * UIScale, (H - 65) * UIScale, 900 * UIScale, 42 * UIScale);
                Text(PC->Journey->Caption, W * .5, H - 57, .48, Ivory, true);
            }
        }
        if (GI->Section == 0 && GI->RunSeconds < 40 && (!Player || !Player->DeveloperFlight))
            Text(TEXT("WASD roam   SPACE double jump   Q dash   J spin   E interact   Arrows: camera   X: center"), W * .5, H - 155, .4, Muted, true);
    }
    else
    {
        DrawRect(FLinearColor(.015, .03, .045, .72), 0, 0, Canvas->SizeX, Canvas->SizeY);
        const bool Settings = PC->Menu == EPOCMenu::Settings;
        const bool Controls = PC->Menu == EPOCMenu::Controls;
        Text(TEXT("THE LONG WAY TO CAKE"), W * .5, Settings ? 50 : 85, .42, Gold, true);
        const FString Title = Settings ? TEXT("Make yourself comfortable.") : Controls ? TEXT("Small paws. Big possibilities.") : PC->Menu == EPOCMenu::Pause ? TEXT("Catch your breath.") : TEXT("PIECE OF CAKE");
        Text(Title, W * .5, Settings ? 80 : 120, Settings || Controls ? 1.1 : 1.65, Ivory, true);
        if (PC->Menu == EPOCMenu::Pause && Player && Player->DeveloperFlight) Text(TEXT("DEVELOPER FLIGHT ENABLED  -  resume to fly"),W*.5,205,.48,FLinearColor(.3,1,.83),true);
        if (PC->Menu == EPOCMenu::Title) Text(TEXT("An enormous journey. A perfectly ordinary dessert."), W * .5, 205, .52, Muted, true);
        if (PC->Menu == EPOCMenu::Complete)
        {
            Text(TEXT("It was, in fact, a piece of cake."), W * .5, 215, .6, Ivory, true);
            if(GI->AssistedRun) Text(TEXT("Developer-assisted ending / unranked"),W*.5,299,.34,Muted,true);
            else if(GI->Cloud) Text(GI->Cloud->Status,W*.5,299,.34,Muted,true);
            Text(FString::Printf(TEXT("%d:%02d     |     %d memory shards     |     %d relics"), int32(GI->RunSeconds) / 60, int32(GI->RunSeconds) % 60, GI->Shards, GI->Relics), W * .5, 260, .48, Gold, true);
        }
        if (Controls)
        {
            const TCHAR* Lines[] = {TEXT("WASD / left stick       Move"), TEXT("Arrows / right stick: camera   F: mouse toggle   X / R3: center"), TEXT("Space / A       Jump; press again for double jump"), TEXT("Q / right click / RB       Dash     Shift / LB       Sprint"), TEXT("J or left click / X       Spin attack / kick bombs"), TEXT("C or Ctrl / B       Slide on land; slam in the air"), TEXT("E / Y       Echo or eat cake"), TEXT("Esc / Menu       Pause          R       Checkpoint")};
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
