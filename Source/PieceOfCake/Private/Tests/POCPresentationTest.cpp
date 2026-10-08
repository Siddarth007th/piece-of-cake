#include "POCPresentationTest.h"
#include "POCGameMode.h"
#include "POCGameInstance.h"
#include "POCCloud.h"
#include "POCWorld.h"
#include "POCCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"
#include "Serialization/JsonSerializer.h"
APOCPresentationTest::APOCPresentationTest()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.bTickEvenWhenPaused=true;
}
void APOCPresentationTest::Tick(float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    Super::Tick(DeltaSeconds);
    const double Now=FPlatformTime::Seconds(); if(!Born) Born=Now;
    auto* PC=Cast<APOCController>(UGameplayStatics::GetPlayerController(this,0));
    auto* P=PC ? Cast<APOCCharacter>(PC->GetPawn()) : nullptr;
    if(!P || !PC->Journey || !PC->Journey->Ready) return;
    if(P->Dreaming && Now-Born>1) {++PresentationFrames;if(!P->IsPresentationVisible())++InvisibleFrames;}
    if(Now-Born<3) return;
    FString Dir; FParse::Value(FCommandLine::Get(),TEXT("POCArtifacts="),Dir);
    if(Dir.IsEmpty()) Dir=FPaths::ProjectSavedDir()/TEXT("PresentationQA");
    IFileManager::Get().MakeDirectory(*Dir,true);
    auto Shot=[&Dir](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Dir/Name,true,false);};
    auto* GI=GetGameInstance<UPOCGameInstance>();
    if(FString(FCommandLine::Get()).Contains(TEXT("POCCloudSmoke")))
    {
        auto* Cloud=GI->Cloud.Get();
        const bool Verify=FParse::Param(FCommandLine::Get(),TEXT("POCCloudSmokeVerify"));
        auto ReportCloud=[&](bool OK,const TCHAR* Reason)
        {
            auto R=MakeShared<FJsonObject>();R->SetBoolField(TEXT("passed"),OK);
            R->SetStringField(TEXT("reason"),Reason);R->SetStringField(TEXT("status"),Cloud?Cloud->Status:TEXT("No client"));
            R->SetBoolField(TEXT("native_https_connected"),Cloud && Cloud->Connected());
            R->SetBoolField(TEXT("outbox_empty"),GI->Save->PendingRuns.IsEmpty());
            R->SetBoolField(TEXT("keychain_session_restored_new_process"),Verify && Cloud && Cloud->Connected());
            FString S;FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&S));
            FFileHelper::SaveStringToFile(S,*(Dir/(Verify?TEXT("cloud-native-reload.json"):TEXT("cloud-native-write.json"))));
            UE_LOG(LogTemp,Display,TEXT("POC_CLOUD_SMOKE passed=%d %s"),OK,Reason);Stage=99;FPlatformMisc::RequestExit(false);
        };
        if(Stage==99) return;
        if(Now-Born>40) {ReportCloud(false,TEXT("Native cloud timeout"));return;}
        if(!Cloud || !Cloud->Configured()) {ReportCloud(false,TEXT("Packaged cloud config missing"));return;}
        if(Stage==0)
        {
            PC->OpenMenu(EPOCMenu::Cloud);
            if(!Verify) Cloud->Connect();
            Stage=1;return;
        }
        if(Stage==1 && Cloud->Connected() && Cloud->Status.Contains(TEXT("saved")))
        {
            if(Verify)
            {
                ReportCloud(GI->Save->BestShards>=420 && GI->Save->Completed,TEXT("New native process restored Keychain session and cloud progress"));return;
            }
            GI->AssistedRun=true;GI->RunSeconds=381;GI->Shards=420;GI->Relics=2;GI->Won=true;
            GI->SaveProgress();Stage=2;return;
        }
        if(Stage==2 && GI->Save->PendingRuns.IsEmpty() && Cloud->Status==TEXT("Cloud progress and run saved"))
        {Shot(TEXT("cloud-connected.png"));Stage=3;Start.X=Now;return;}
        if(Stage==3 && Now-Start.X>.5) ReportCloud(true,TEXT("Native client authenticated, synced progress and acknowledged an assisted run"));
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("POCSceneryTour")))
    {
        // Render review only. Teleporting here is never counted as traversal evidence.
        const int32 Room=Stage/2;
        if(Room>=8) { if(Now-Start.X>1) FPlatformMisc::RequestExit(false);return; }
        if(Stage%2==0)
        {
            P->Dreaming=false;PC->Journey->ShowDream(false);PC->OpenMenu(EPOCMenu::Playing);PC->SetViewTarget(P);
            P->SetDeveloperFlight(true);
            const auto& Point=PC->Journey->Route[Room*24+8];
            P->SetActorLocation(Point.Position-FRotator(0,Point.Yaw,0).Vector()*430+FVector(0,0,50));
            P->SetActorRotation(FRotator(0,Point.Yaw,0));PC->SetControlRotation(FRotator(-12,Point.Yaw,0));
            Start.X=Now;++Stage;return;
        }
        if(Now-Start.X>2)
        {
            Shot(*FString::Printf(TEXT("district-%d.png"),Room+1));
            Start.X=Now;++Stage;
        }
        return;
    }
    if(FParse::Param(FCommandLine::Get(),TEXT("POCVerifySave")))
    {
        auto R=MakeShared<FJsonObject>();
        const bool OK=FMath::IsNearlyEqual(GI->Save->MusicVolume,.35f) && FMath::IsNearlyEqual(GI->Save->Sensitivity,1.3f) && GI->Save->Completed && GI->Save->BestShards>=420;
        R->SetBoolField(TEXT("save_reloaded_in_new_process"),OK);
        FString S;FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&S));FFileHelper::SaveStringToFile(S,*(Dir/TEXT("save-reload.json")));
        FPlatformMisc::RequestExit(false);return;
    }
    const double Age=Now-Born;
    if(Stage==0) { Start=P->GetActorLocation();Shot(TEXT("title.png"));Stage=1; }
    if(Stage==1 && Age>4) { PC->Selection=0;PC->ActivateSelection();Stage=2; }
    if(Stage==2 && Age>5.7) { Shot(TEXT("thinking.png"));Stage=3; }
    if(Stage==3 && Age>8.3) { Shot(TEXT("cake-dream.png"));Stage=4; }
    if(Stage==4 && Age>11)
    {
        Shot(TEXT("ready-to-play.png"));
        auto R=MakeShared<FJsonObject>();
        R->SetBoolField(TEXT("mascot_never_disappeared"),InvisibleFrames==0 && PresentationFrames>100);
        R->SetNumberField(TEXT("presentation_frames_checked"),PresentationFrames);
        R->SetNumberField(TEXT("invisible_frames"),InvisibleFrames);
        R->SetBoolField(TEXT("intro_returns_to_play"),PC->Menu==EPOCMenu::Playing);
        R->SetBoolField(TEXT("no_fall_or_damage"),P->Hearts==3 && P->FallDeaths==0);
        R->SetBoolField(TEXT("no_character_teleport"),FVector::Dist(Start,P->GetActorLocation())<15);
        R->SetBoolField(TEXT("grounded_after_intro"),P->GetCharacterMovement()->IsMovingOnGround());
        R->SetBoolField(TEXT("pose_blended_out"),P->ThoughtBlend<.05f);
        GI->Save->MusicVolume=.35f;GI->Save->Sensitivity=1.3f;GI->Save->Completed=true;GI->Save->BestShards=420;GI->SaveProgress();
        FString S;FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&S));FFileHelper::SaveStringToFile(S,*(Dir/TEXT("presentation.json")));Stage=5;
    }
    if(Stage==5 && Age>12) FPlatformMisc::RequestExit(false);
#endif
}
