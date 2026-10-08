#include "POCFlightTest.h"
#include "POCGameMode.h"
#include "POCCharacter.h"
#include "POCWorld.h"
#include "POCGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

APOCFlightTest::APOCFlightTest() { PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.bTickEvenWhenPaused=true; }
void APOCFlightTest::Tick(float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    Super::Tick(DeltaSeconds);
    if(Phase==99) return;
    const double Now=FPlatformTime::Seconds(); if(!Born) Born=Now;
    if(Now-Born>70) { Finish(false,TEXT("Flight test timeout"));return; }
    auto* PC=Cast<APOCController>(UGameplayStatics::GetPlayerController(this,0));
    auto* P=PC?Cast<APOCCharacter>(PC->GetPawn()):nullptr;
    if(!P || !PC->Journey || !PC->Journey->Ready) return;
    auto* GI=GetGameInstance<UPOCGameInstance>();
    auto Key=[&](FKey K,EInputEvent E) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,E,E==IE_Released?0:1)); };
    FString ShotDir;FParse::Value(FCommandLine::Get(),TEXT("POCArtifacts="),ShotDir);
    if(ShotDir.IsEmpty()) ShotDir=FPaths::ProjectSavedDir()/TEXT("FlightQA");
    IFileManager::Get().MakeDirectory(*ShotDir,true);
    auto Code=[&]() { for(TCHAR C:FString(TEXT("SIDDARTHISGOD"))) { FKey K(FName(*FString::Chr(C))); Key(K,IE_Pressed);Key(K,IE_Released); } };
    if(Phase==-1)
    {
        if(Now-Born<3) return;
        if(PC->Menu==EPOCMenu::Title) { PC->Selection=0;PC->ActivateSelection();return; }
        if(PC->Menu!=EPOCMenu::Playing || !P->GetCharacterMovement()->IsMovingOnGround()) return;
        InitialRespawns=P->RespawnCount; InitialShards=GI->Shards;
        PC->Journey->Complete(P,PC->Journey->Route.Last().Position);
        NormalGateProtected=!P->Eating && !GI->Won;
        Code();
        if(!P->DeveloperFlight || P->GetActorEnableCollision() || P->RespawnCount!=InitialRespawns)
        { Finish(false,TEXT("Phrase did not enable flight cleanly"));return; }
        const int32 Hearts=P->Hearts;
        P->InvulnerableRemaining=0; P->Hurt(P->GetActorLocation()+FVector(100,0,0),1000,1000,3);P->BeginDeath(true);
        DamageBlocked=P->Hearts==Hearts && !P->Dying;
        Phase=0;PhaseAt=Now;Start=P->GetActorLocation();Key(EKeys::C,IE_Pressed);return;
    }
    const FKey Keys[]={EKeys::C,EKeys::SpaceBar,EKeys::W,EKeys::S,EKeys::D,EKeys::A};
    if(Phase<6)
    {
        const double Duration=Phase<2?2.2:.7;
        if(Now-PhaseAt<Duration) return;
        Key(Keys[Phase],IE_Released);
        const FVector Delta=P->GetActorLocation()-Start;Deltas.Add(Delta);
        if(Phase==0) BelowFloor=P->GetActorLocation().Z < PC->Journey->Route[0].Position.Z-1700;
        const FVector Directions[]={-FVector::UpVector,FVector::UpVector,FVector::YAxisVector,-FVector::YAxisVector,-FVector::XAxisVector,FVector::XAxisVector};
        if(FVector::DotProduct(Delta,Directions[Phase])<150 || !P->DeveloperFlight || P->Dying || P->RespawnCount!=InitialRespawns)
        { Finish(false,FString::Printf(TEXT("Flight movement phase %d failed: %s"),Phase,*Delta.ToCompactString()));return; }
        UE_LOG(LogTemp,Display,TEXT("POC_FLIGHT_TEST axis=%d delta=%s"),Phase,*Delta.ToCompactString());
        P->GetCharacterMovement()->StopMovementImmediately();
        ++Phase;Start=P->GetActorLocation();PhaseAt=Now;
        if(Phase<6) Key(Keys[Phase],IE_Pressed);
        else
        {
            if(GI->Shards!=InitialShards) { Finish(false,TEXT("Flight collected gameplay shards"));return; }
            FScreenshotRequest::RequestScreenshot(ShotDir/TEXT("developer-flight-native.png"),false,false);
        }
        return;
    }
    if(Phase==6)
    {
        if(Now-PhaseAt<.3) return;
        const auto& LandingPoint=PC->Journey->Route[1];
        P->SetActorLocation(LandingPoint.Position+FVector(0,0,220));
        const FVector Before=P->GetActorLocation();
        Code();
        LandedWhereExplored=FVector::Dist2D(Before,P->GetActorLocation())<2 && P->RespawnCount==InitialRespawns;
        Restored=!P->DeveloperFlight && P->GetActorEnableCollision() && P->GetCharacterMovement()->GravityScale==1.6f
            && LandedWhereExplored;
        Phase=7;PhaseAt=Now;return;
    }
    if(Phase==7)
    {
        if(!P->GetCharacterMovement()->IsMovingOnGround()) return;
        NormalStart=P->GetActorLocation();Key(EKeys::SpaceBar,IE_Pressed);PhaseAt=Now;Phase=8;return;
    }
    if(Phase==8)
    {
        NormalJumpHeight=FMath::Max(NormalJumpHeight,float(P->GetActorLocation().Z-NormalStart.Z));
        if(!NormalJumpReleased && Now-PhaseAt>.05) { Key(EKeys::SpaceBar,IE_Released);NormalJumpReleased=true; }
        if(Now-PhaseAt<1.2) return;
        if(!P->GetCharacterMovement()->IsMovingOnGround()) {Finish(false,TEXT("Ordinary jump did not land"));return;}
        Code();P->SetActorLocation(PC->Journey->Route[1].Position-FVector(0,0,2300));Code();
        UnsafeExitBlocked=P->DeveloperFlight && !P->GetActorEnableCollision();
        Key(EKeys::R,IE_Pressed);Key(EKeys::R,IE_Released);Phase=9;PhaseAt=Now;return;
    }
    if(Phase==9)
    {
        if(Now-PhaseAt<1) return;
        ExplicitRescue=!P->DeveloperFlight && P->GetActorEnableCollision() && FVector::Dist(P->GetActorLocation(),P->Checkpoint.GetLocation())<100;
        if(!ExplicitRescue) {Finish(false,TEXT("R failed to restore checkpoint"));return;}
        InitialShards=GI->Shards; // Normal landing/jump may legitimately collect a shard.
        Code();
        const auto& Last=PC->Journey->Route.Last();
        P->SetActorLocation(Last.Position+FRotator(0,Last.Yaw,0).Vector()*680+FVector(0,0,100));
        P->GetCharacterMovement()->StopMovementImmediately();
        Key(EKeys::E,IE_Pressed);Key(EKeys::E,IE_Released);Phase=10;PhaseAt=Now;return;
    }
    if(Phase==10)
    {
        if(Now-PhaseAt<5) return;
        CakeCompleted=GI->Won && PC->Menu==EPOCMenu::Complete && !P->DeveloperFlight && PC->Journey->GateIsOpen();
        AssistedUnranked=GI->AssistedRun && GI->Shards==InitialShards && GI->ShardsSpent==0 && GI->Save->PendingRuns.IsEmpty();
        FScreenshotRequest::RequestScreenshot(ShotDir/TEXT("developer-cake-ending.png"),true,false);
        Phase=11;PhaseAt=Now;return;
    }
    if(Phase==11 && Now-PhaseAt>.4)
    {
        Finish(DamageBlocked && BelowFloor && Restored && NormalJumpHeight>80 && NormalGateProtected && UnsafeExitBlocked && ExplicitRescue && CakeCompleted && AssistedUnranked,
            TEXT("Flight directions, land here, blocked unsafe exit, R rescue, normal gate and assisted cake ending"));
    }
#endif
}
void APOCFlightTest::Finish(bool Passed,const FString& Reason)
{
#if !UE_BUILD_SHIPPING
    Phase=99;
    TSharedRef<FJsonObject> Report=MakeShared<FJsonObject>();
    Report->SetBoolField(TEXT("passed"),Passed);Report->SetStringField(TEXT("reason"),Reason);
    Report->SetBoolField(TEXT("damage_blocked"),DamageBlocked);Report->SetBoolField(TEXT("below_floor_without_death"),BelowFloor);
    Report->SetBoolField(TEXT("collision_gravity_restored"),Restored);
    Report->SetBoolField(TEXT("lands_where_explored_without_respawn"),LandedWhereExplored);
    Report->SetBoolField(TEXT("normal_shard_gate_preserved"),NormalGateProtected);
    Report->SetBoolField(TEXT("unsafe_exit_stays_in_flight"),UnsafeExitBlocked);
    Report->SetBoolField(TEXT("explicit_R_restores_checkpoint"),ExplicitRescue);
    Report->SetBoolField(TEXT("fly_to_cake_completes_game"),CakeCompleted);
    Report->SetBoolField(TEXT("assisted_without_fake_shards_or_ranked_score"),AssistedUnranked);Report->SetNumberField(TEXT("normal_50ms_tap_jump_height_cm"),NormalJumpHeight);
    TArray<TSharedPtr<FJsonValue>> Moves;
    for(const FVector& Delta:Deltas)
    {
        TArray<TSharedPtr<FJsonValue>> V;V.Add(MakeShared<FJsonValueNumber>(Delta.X));V.Add(MakeShared<FJsonValueNumber>(Delta.Y));V.Add(MakeShared<FJsonValueNumber>(Delta.Z));
        Moves.Add(MakeShared<FJsonValueArray>(V));
    }
    Report->SetArrayField(TEXT("deltas_down_up_forward_backward_right_left"),Moves);
    FString Text;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));
    FString Dir;FParse::Value(FCommandLine::Get(),TEXT("POCArtifacts="),Dir);
    if(Dir.IsEmpty()) Dir=FPaths::ProjectDir()/TEXT("Artifacts");
    IFileManager::Get().MakeDirectory(*Dir,true);
    FFileHelper::SaveStringToFile(Text,*(Dir/TEXT("developer-flight-test.json")));
    UE_LOG(LogTemp,Display,TEXT("POC_FLIGHT_TEST passed=%d %s"),Passed,*Reason);FPlatformMisc::RequestExit(false);
#endif
}
