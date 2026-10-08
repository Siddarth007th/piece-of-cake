#include "POCCameraTest.h"
#include "POCGameMode.h"
#include "POCCharacter.h"
#include "POCWorld.h"
#include "POCGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

APOCCameraTest::APOCCameraTest() { PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.bTickEvenWhenPaused=true; }
void APOCCameraTest::Tick(float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    Super::Tick(DeltaSeconds);
    if(Phase==99) return;
    const double Now=FPlatformTime::Seconds();if(!Born) Born=Now;
    if(Now-Born>60) { Finish(false,TEXT("Camera test timeout"));return; }
    auto* PC=Cast<APOCController>(UGameplayStatics::GetPlayerController(this,0));
    auto* P=PC?Cast<APOCCharacter>(PC->GetPawn()):nullptr;
    if(!P || !PC->Journey || !PC->Journey->Ready) return;
    auto Key=[&](FKey K,EInputEvent E,float Value=1) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,E,E==IE_Released?0:Value,1)); };
    auto Tap=[&](FKey K) { Key(K,IE_Pressed);Key(K,IE_Released); };
    auto Next=[&](int32 Step) { Phase=Step;PhaseAt=Now;StartView=PC->GetControlRotation();StartPosition=P->GetActorLocation(); };
    auto YawDelta=[&]() { return FMath::FindDeltaAngleDegrees(StartView.Yaw,PC->GetControlRotation().Yaw); };
    if(Phase==-1)
    {
        if(Now-Born<3) return;
        if(PC->Menu==EPOCMenu::Title) { PC->Selection=0;PC->ActivateSelection();return; }
        if(PC->Menu!=EPOCMenu::Playing || !P->GetCharacterMovement()->IsMovingOnGround()) return;
        GetGameInstance<UPOCGameInstance>()->Save->Sensitivity=1;
        GetGameInstance<UPOCGameInstance>()->Save->InvertY=false;
        Next(0);return;
    }
    const double Elapsed=Now-PhaseAt;
    if(Phase==0)
    {
        Key(EKeys::MouseX,IE_Axis,10);
        if(Elapsed<.4) return;
        Measures.Add(TEXT("locked_mouse_yaw_change"),YawDelta());
        if(FMath::Abs(YawDelta())>.1) { Finish(false,TEXT("Mouse moved camera while locked"));return; }
        Next(1);Key(EKeys::Right,IE_Pressed);return;
    }
    if(Phase==1 || Phase==2)
    {
        if(Elapsed<.6) return;
        const FKey K=Phase==1?EKeys::Right:EKeys::Left;Key(K,IE_Released);
        const float Delta=YawDelta();Measures.Add(Phase==1?TEXT("right_arrow_degrees"):TEXT("left_arrow_degrees"),Delta);
        if((Phase==1?Delta:-Delta)<35 || FVector::Dist(P->GetActorLocation(),StartPosition)>5)
        { Finish(false,TEXT("Arrow orbit failed or moved Nori"));return; }
        if(Phase==1) { Next(2);Key(EKeys::Left,IE_Pressed); }
        else { Next(3);Key(EKeys::Up,IE_Pressed); }return;
    }
    if(Phase==3 || Phase==4)
    {
        if(Elapsed<(Phase==3?1.2:1.8)) return;
        Key(Phase==3?EKeys::Up:EKeys::Down,IE_Released);
        const float Pitch=FRotator::NormalizeAxis(PC->GetControlRotation().Pitch);
        Measures.Add(Phase==3?TEXT("up_pitch_limit"):TEXT("down_pitch_limit"),Pitch);
        if(FMath::Abs(Pitch-(Phase==3?28.f:-65.f))>1) { Finish(false,TEXT("Pitch clamp or arrow direction failed"));return; }
        if(Phase==3) { Next(4);Key(EKeys::Down,IE_Pressed); }
        else { Next(5);Tap(EKeys::X); }return;
    }
    if(Phase==5)
    {
        if(Elapsed<.65) return;
        const float Error=FMath::Abs(FMath::FindDeltaAngleDegrees(P->GetActorRotation().Yaw,PC->GetControlRotation().Yaw));
        Measures.Add(TEXT("recenter_yaw_error"),Error);
        if(Error>.5 || FMath::Abs(FRotator::NormalizeAxis(PC->GetControlRotation().Pitch)+14)>.5) { Finish(false,TEXT("Recenter failed"));return; }
        Next(6);Tap(EKeys::F);return;
    }
    if(Phase==6)
    {
        if(Elapsed<.1) return;
        ToggleOn=P->IsMouseCameraActive();
        if(!ToggleOn) { Finish(false,TEXT("A single F tap did not latch camera"));return; }
        Next(7);return;
    }
    if(Phase==7 || Phase==9 || Phase==10)
    {
        Key(EKeys::MouseX,IE_Axis,10);
        if(Elapsed<.45) return;
        const float Delta=YawDelta();Measures.Add(Phase==7?TEXT("toggled_mouse_degrees"):Phase==9?TEXT("untoggled_mouse_degrees"):TEXT("middle_drag_degrees"),Delta);
        if(Phase==9 ? FMath::Abs(Delta)>.1 : Delta<15) { Finish(false,TEXT("Mouse toggle/drag state failed"));return; }
        if(Phase==7) { Next(8);Tap(EKeys::F); }
        else if(Phase==9) { Next(10);Key(EKeys::MiddleMouseButton,IE_Pressed); }
        else { Key(EKeys::MiddleMouseButton,IE_Released);Next(11);Tap(EKeys::F); }return;
    }
    if(Phase==8)
    {
        if(Elapsed<.15) return;
        if(P->IsMouseCameraActive()) { Finish(false,TEXT("Second F tap did not lock"));return; }
        Next(9);return;
    }
    if(Phase==11)
    {
        if(Elapsed<.15) return;
        PC->OpenMenu(EPOCMenu::Pause);PauseReset=!P->IsMouseCameraActive();Next(12);Key(EKeys::Right,IE_Pressed);return;
    }
    if(Phase==12)
    {
        if(Elapsed<.35) return;
        Key(EKeys::Right,IE_Released);
        if(!PauseReset || FMath::Abs(YawDelta())>.1) { Finish(false,TEXT("Pause failed to lock camera"));return; }
        PC->OpenMenu(EPOCMenu::Playing);Tap(EKeys::X);Next(13);return;
    }
    if(Phase==13)
    {
        if(Elapsed<.6) return;
        Next(14);Key(EKeys::W,IE_Pressed);return;
    }
    if(Phase==14)
    {
        if(Elapsed<.25) return;
        Key(EKeys::W,IE_Released);Measures.Add(TEXT("walking_camera_yaw_change"),YawDelta());
        const bool Stable=FMath::Abs(YawDelta())<.1 && FVector::Dist(P->GetActorLocation(),StartPosition)>30;
        Finish(Stable && ToggleOn && PauseReset && FMath::IsNearlyEqual(P->Boom->TargetArmLength,420.f) && FMath::IsNearlyEqual(P->Camera->FieldOfView,82.f),TEXT("Native key/axis input: arrows, clamps, recenter, F toggle, middle drag, pause reset and stable walking"));
    }
#endif
}
void APOCCameraTest::Finish(bool Passed,const FString& Reason)
{
#if !UE_BUILD_SHIPPING
    Phase=99;TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    Report->SetBoolField(TEXT("passed"),Passed);Report->SetStringField(TEXT("reason"),Reason);
    Report->SetStringField(TEXT("mode"),TEXT("Native controller key and mouse-axis simulation in running Unreal; browser checks recorded separately"));
    Report->SetBoolField(TEXT("tap_F_latches_mouse"),ToggleOn);Report->SetBoolField(TEXT("pause_locks_camera"),PauseReset);
    for(const auto& Pair:Measures) Report->SetNumberField(Pair.Key,Pair.Value);
    FString Text;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));
    FFileHelper::SaveStringToFile(Text,*(FPaths::ProjectDir()/TEXT("Artifacts/camera-control-test.json")));
    UE_LOG(LogTemp,Display,TEXT("POC_CAMERA_TEST passed=%d %s"),Passed,*Reason);FPlatformMisc::RequestExit(false);
#endif
}
