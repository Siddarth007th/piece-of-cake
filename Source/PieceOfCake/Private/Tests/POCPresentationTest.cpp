#include "POCPresentationTest.h"
#include "POCGameMode.h"
#include "POCGameInstance.h"
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
    if(!P || !PC->Journey || !PC->Journey->Ready || Now-Born<3) return;
    FString Dir; FParse::Value(FCommandLine::Get(),TEXT("POCArtifacts="),Dir);
    if(Dir.IsEmpty()) Dir=FPaths::ProjectSavedDir()/TEXT("PresentationQA");
    IFileManager::Get().MakeDirectory(*Dir,true);
    auto Shot=[&Dir](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(Dir/Name,true,false);};
    auto* GI=GetGameInstance<UPOCGameInstance>();
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
