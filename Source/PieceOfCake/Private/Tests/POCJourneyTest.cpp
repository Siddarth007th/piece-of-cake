#include "POCJourneyTest.h"
#include "POCCharacter.h"
#include "POCWorld.h"
#include "POCGameMode.h"
#include "POCGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Components/StaticMeshComponent.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    FString QAOutputDirectory()
    {
        FString Directory;
        if (!FParse::Value(FCommandLine::Get(), TEXT("POCArtifacts="), Directory))
            Directory = FPaths::ProjectDir()/TEXT("Artifacts");
        IFileManager::Get().MakeDirectory(*Directory, true);
        return Directory;
    }
}

APOCJourneyTest::APOCJourneyTest()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void APOCJourneyTest::Tick(float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    Super::Tick(DeltaSeconds);
    if (Finished) return;
    const double Now = FPlatformTime::Seconds();
    if (Born == 0) { Born = Now; FParse::Value(FCommandLine::Get(), TEXT("POCAutoRun="), Run); FParse::Value(FCommandLine::Get(),TEXT("POCSectionProbe="),SectionProbe); }
    auto* PC = Cast<APOCController>(UGameplayStatics::GetPlayerController(this, 0));
    auto* Player = PC ? Cast<APOCCharacter>(PC->GetPawn()) : nullptr;
    auto* Journey = PC ? PC->Journey.Get() : nullptr;
    if (!Player || !Journey || !Journey->Ready) { if (Now - Born > 90) Finish(false, TEXT("World did not become ready")); return; }
    auto* GI = GetGameInstance<UPOCGameInstance>();
    if (GI->QARestartPending)
    {
        if (Now - Born < 3) return;
        if (!RestartBegan)
        {
            if (PC->Menu != EPOCMenu::Title || GI->Won || GI->Shards > 1 || GI->Relics != 0)
            { Finish(false, TEXT("New journey did not reset title/progress")); return; }
            PC->Selection = 0; PC->ActivateSelection(); RestartBegan = true; Born = Now; return;
        }
        if(PC->Menu==EPOCMenu::Dream && Now-Born<12) return;
        FString Contents;
        const FString File = QAOutputDirectory()/FString::Printf(TEXT("journey-run-%d.json"),Run);
        TSharedPtr<FJsonObject> Report;
        FFileHelper::LoadFileToString(Contents, *File);
        if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Contents), Report))
        {
            const bool RestartOK = PC->Menu == EPOCMenu::Playing && !GI->Won && !Player->Eating
                && Player->GetCharacterMovement()->IsMovingOnGround() && GI->Shards <= 1;
            Report->SetBoolField(TEXT("restart_verified"), RestartOK);
            Report->SetBoolField(TEXT("completed"), RestartOK && Report->GetBoolField(TEXT("completed")));
            FString Text; FJsonSerializer::Serialize(Report.ToSharedRef(), TJsonWriterFactory<>::Create(&Text));
            FFileHelper::SaveStringToFile(Text, *File);
            UE_LOG(LogTemp, Display, TEXT("POC_QA RESTART verified=%d"), RestartOK);
        }
        Finished = true; FPlatformMisc::RequestExit(false); return;
    }
    if (!Started)
    {
        if (PC->Menu != EPOCMenu::Title || Now - Born < 3) return;
        int32 Quality = GI->Save->Quality;
        if (FParse::Value(FCommandLine::Get(), TEXT("POCQuality="), Quality)) GI->Save->Quality = FMath::Clamp(Quality, 0, 3);
        GI->ApplySettings();
        PC->Selection = 0; PC->ActivateSelection();
        PC->ConsoleCommand(TEXT("stat unit"), false);
        Started = true; StartedAt = LastProgress = Now; LastRespawnCount = Player->RespawnCount;
        UE_LOG(LogTemp, Display, TEXT("POC_QA START run=%d physics traversal, no teleport or time scaling"), Run);
    }
    if (Player->DeveloperFlight) { Finish(false, TEXT("Developer flight must remain off in a beatability run")); return; }
    if (GI->Won)
    {
        if (WonAt == 0)
        {
            WonAt = Now;
            FScreenshotRequest::RequestScreenshot(QAOutputDirectory()/FString::Printf(TEXT("run-%d-ending.png"),Run), false, false);
            return;
        }
        if (Now - WonAt > 2) Finish(true, TEXT("Reached cake using ordinary traversal"));
        return;
    }
    if (Now - StartedAt > 1800) { Finish(false, TEXT("Run exceeded 30 minutes")); return; }
    if (PauseUntil > 0)
    {
        if (Now >= PauseUntil) { PC->OpenMenu(EPOCMenu::Playing); PauseUntil = 0; PauseCompleted = true; }
        LastTick = 0; return;
    }
    if (PC->Menu != EPOCMenu::Playing) { LastTick = 0; LastProgress = Now; return; }
    if(SectionProbe>=0 && !SectionProbeStarted)
    {
        Journey->TeleportSection(SectionProbe);SectionProbeStarted=true;
        LastIndex=SectionProbe*24;Furthest=LastIndex-1;LastRespawnCount=Player->RespawnCount;
        UE_LOG(LogTemp,Display,TEXT("POC_QA SECTION PROBE start=%d; not a full journey"),SectionProbe);
    }
    if(SectionProbe>=0 && Furthest>=(SectionProbe+1)*24)
    { Finish(true,TEXT("Isolated district traversal passed; not a complete journey"));return; }
    if (LastTick > 0 && Now - StartedAt > 5) FrameMs.Add((Now - LastTick) * 1000);
    LastTick = Now;
    if (Journey->ActiveCheckpoint >= 0) Checkpoints.Add(Journey->ActiveCheckpoint);
    if(BombAuditActive && Player->DamageEvents>BombEventsBefore && FVector::Dist(Player->LastDamageSource,BombAuditSource)<5)
    {
        BombDamageVerified |= Player->Hearts < BombHeartsBefore;
        if(BombHitAt==0) BombHitAt=Now;
        const FVector Away=(Player->GetActorLocation()-BombAuditSource).GetSafeNormal2D();
        BombKnockbackVerified |= FVector::DotProduct(Player->GetVelocity(),Away)>200 && Player->GetVelocity().Z>200;
        if(BombDamageVerified && BombKnockbackVerified)
        { BombAuditActive=false; UE_LOG(LogTemp,Display,TEXT("POC_QA BOMB verified health loss and physical knockback")); }
        else if(Now-BombHitAt>.3) { Finish(false,TEXT("Bomb failed health/knockback audit"));return; }
    }
    if(Player->Dying)
    {
        EmptyHeartsObserved |= Player->Hearts==0;
        LastProgress=Now;
        return;
    }
    if (EchoWaitUntil > 0)
    {
        if (Now < EchoWaitUntil) return;
        bool Expired = true, Found = false;
        for (TActorIterator<APOCProp> It(GetWorld()); It; ++It)
            if (It->Kind == EPOCProp::EchoBridge && It->Group == ExpiryGroup)
            { Found = true; Expired &= It->ActiveRemaining <= 0 && !It->Mesh->IsVisible(); }
        EchoExpiryCompleted = Found && Expired;
        if (!EchoExpiryCompleted) { Finish(false, TEXT("Echo bridge expiry failed")); return; }
        Player->Echo(); ++EchoCount; EchoWaitUntil = 0; LastProgress = Now;
        UE_LOG(LogTemp, Display, TEXT("POC_QA ECHO expiry and reactivation verified group=%d"), ExpiryGroup);
    }
    const auto* Nearest = Journey->Nearest(Player->GetActorLocation());
    if (!Nearest) { Finish(false, TEXT("Route missing")); return; }
    auto* Movement = Player->GetCharacterMovement();
    const bool Grounded = Movement->IsMovingOnGround();
    // The opening must be passable with short taps, not only perfectly held jumps.
    if (Furthest < 8 && Player->JumpHeld && GetWorld()->GetTimeSeconds()-Player->LastJumpTime > .05f)
        Player->JumpReleased();
    if (Player->RespawnCount != LastRespawnCount)
    {
        Respawns += Player->RespawnCount - LastRespawnCount; LastRespawnCount = Player->RespawnCount;
        UE_LOG(LogTemp, Display, TEXT("POC_QA RESPAWN count=%d checkpoint=%d"), Respawns, Journey->ActiveCheckpoint);
        if (FallStarted) FallCompleted = true;
        LastIndex = Nearest->Index; CenteredPlatform=-1; LastProgress = Now;
        if (Respawns > 6) { Finish(false, TEXT("Repeated traversal failures")); return; }
    }
    if (Grounded)
    {
        if(LastIndex!=Nearest->Index) LastProgress=Now;
        LastIndex = Nearest->Index; LandedPlatforms.Add(LastIndex);
        if (LastIndex > Furthest)
        {
            Furthest = LastIndex; LastProgress = Now;
            UE_LOG(LogTemp, Display, TEXT("POC_QA PLATFORM %d section=%d"), LastIndex, Nearest->Section);
            if (FParse::Param(FCommandLine::Get(), TEXT("POCScreenshots")) && LastIndex % 24 == 3)
                FScreenshotRequest::RequestScreenshot(QAOutputDirectory()/FString::Printf(TEXT("run-%d-section-%d.png"), Run, Nearest->Section), false, false);
        }
    }
    if (Now - LastProgress > 45) { Finish(false, TEXT("No forward progress for 45 seconds")); return; }
    const auto& Point = Journey->Route[FMath::Clamp(LastIndex, 0, Journey->Route.Num()-1)];
    const int32 Needed=(Journey->LeftSwitch?0:APOCWorld::SwitchPrice)+(Journey->RightSwitch?0:APOCWorld::SwitchPrice);
    if(TravelDirection<0 && GI->Shards-GI->ShardsSpent>=Needed)
    { TravelDirection=1; BacktrackCompleted=true; LastProgress=Now; }
    const FVector Forward = FRotator(0, Point.Yaw, 0).Vector()*TravelDirection;
    const FVector Right = FRotationMatrix(FRotator(0, Point.Yaw, 0)).GetUnitAxis(EAxis::Y);
    const FVector Relative = Player->GetActorLocation() - Point.Position;
    const double Along = FVector::DotProduct(Relative, Forward);
    // On weaving islands, finish the landing before aiming for the next island.
    // Nearest-centre changes near an edge; immediately turning there cuts corners.
    if(Point.Section==4 && Grounded && CenteredPlatform!=Point.Index)
    {
        FVector Center=Point.Position;
        if(Run==3 && !BacktrackStarted && LastIndex>=112) Center+=Right*290;
        if(FVector::Dist2D(Center,Player->GetActorLocation())>95)
        {
            const FVector ToCenter=(Center-Player->GetActorLocation()).GetSafeNormal2D();
            PC->SetControlRotation(FRotator(-14,ToCenter.Rotation().Yaw,0));
            Player->AddMovementInput(ToCenter,1);return;
        }
        CenteredPlatform=Point.Index;
    }
    if (Run == 2 && Furthest >= 10 && !FallCompleted)
    {
        FallStarted = true;
        PC->SetControlRotation(FRotator(-14, Right.Rotation().Yaw, 0));
        if (Grounded) Player->AddMovementInput(Right, 1);
        return;
    }
    if (Run == 3 && Furthest >= 14 && !PauseCompleted && Grounded)
    {
        PC->OpenMenu(EPOCMenu::Pause); PauseUntil = Now + 1.5; LastTick = 0; return;
    }
    if(Now-LastEcho>1)
    {
        for(TActorIterator<APOCProp> It(GetWorld());It;++It)
            if(It->Kind==EPOCProp::EchoNode && FVector::Dist(It->GetActorLocation(),Player->GetActorLocation())<410)
            {
                Player->Echo();LastEcho=Now;++EchoCount;
                EchoGroups.Add(It->Group);
                if(Run==3 && !EchoExpiryCompleted)
                { ExpiryGroup=It->Group;EchoWaitUntil=Now+19;return; }
                break;
            }
    }
    // Run two deliberately accepts a live, unmodified bomb on the route.
    // No synthetic Hurt call, health override or teleport is used.
    if(Run==2 && LastIndex==11 && !BombKnockbackVerified)
        for(TActorIterator<APOCProp> It(GetWorld());It;++It)
            if(It->Kind==EPOCProp::Bomb && It->Id==43011)
            {
                if(!BombAuditActive)
                { BombAuditActive=true;BombHeartsBefore=Player->Hearts;BombEventsBefore=Player->DamageEvents;BombAuditSource=It->GetActorLocation()+FVector(0,0,40); }
                const FVector Target=It->GetActorLocation()-Forward*70;
                if(FVector::Dist2D(Player->GetActorLocation(),Target)>20) Player->AddMovementInput((Target-Player->GetActorLocation()).GetSafeNormal2D(),1);
                else Movement->StopMovementImmediately();
                return;
            }
    for (TActorIterator<APOCEnemy> It(GetWorld()); It; ++It)
    {
        const FVector Delta = It->GetActorLocation() - Player->GetActorLocation();
        if (!It->IsHidden() && Delta.Size2D() < 200 && FMath::Abs(Delta.Z) < 100 && Player->AttackRemaining <= 0)
        { Player->Bonk(); ++BonkCount; break; }
    }
    for (TActorIterator<APOCProp> It(GetWorld()); It; ++It)
    {
        if(It->Kind==EPOCProp::Bomb && !It->IsHidden() && FVector::DistSquared(It->GetActorLocation(),Player->GetActorLocation())<FMath::Square(190.f) && Player->AttackRemaining<=0)
        { Player->Bonk(); ++BonkCount; }
        if(It->Id==40000+Point.Index && Along>-330 && Along<-140 && Grounded)
        {
            const float Phase=FMath::Fmod(GetWorld()->GetTimeSeconds()+(It->Id%24)/8*.35f,3.6f);
            const float PressPhase=FMath::Fmod(GetWorld()->GetTimeSeconds()+It->Id*.31f,4.f);
            const bool Wait=It->Kind==EPOCProp::Crusher ? PressPhase<2.6f || PressPhase>3.25f : Phase<2.1f || Phase>2.8f;
            if(Wait) { Movement->StopMovementImmediately(); return; }
        }
    }
    if(Point.Sweeper && Along>-320 && Along<0 && Grounded)
    { Player->JumpReleased(); Player->JumpPressed(); ++JumpCount; }
    if (Point.SlideGate && Along > -240 && Along < 100 && Grounded && Player->SlideRemaining <= 0)
    { Player->SlidePressed(); ++SlideCount; }
    if(Point.Section==2 && (Point.Index%24==3 || Point.Index%24==19))
    for(TActorIterator<APOCProp> Gate(GetWorld());Gate;++Gate)
        if(Gate->Kind==EPOCProp::ArenaGate && Gate->Group==Point.Index && !Gate->EncounterCleared)
        {
            APOCEnemy* Target=nullptr;float Best=FLT_MAX;
            for(TActorIterator<APOCEnemy> Enemy(GetWorld());Enemy;++Enemy)
                if(!Enemy->IsHidden() && Enemy->ArenaGroup==Point.Index)
                { const float Dist=FVector::DistSquared2D(Enemy->GetActorLocation(),Player->GetActorLocation());if(Dist<Best){Best=Dist;Target=*Enemy;} }
            if(Target)
            {
                const FVector ToEnemy=(Target->GetActorLocation()-Player->GetActorLocation()).GetSafeNormal2D();
                PC->SetControlRotation(FRotator(-14,ToEnemy.Rotation().Yaw,0));
                if(Best>FMath::Square(130.f)) Player->AddMovementInput(ToEnemy,1); else Movement->StopMovementImmediately();
                LastProgress=Now;return;
            }
        }
    FVector Destination;
    if(LastIndex==Journey->Route.Num()-1 && TravelDirection>0)
    {
        if(!Journey->GateIsOpen())
        {
            Destination=Point.Position-Forward*180+Right*(Journey->LeftSwitch?520:-520);
            if(FVector::Dist2D(Player->GetActorLocation(),Destination)<120 && Now-LastEcho>.5)
            {
                const int32 Before=GI->ShardsSpent;
                const bool Paid=Journey->ActivateEcho(Player); LastEcho=Now;
                if(!Paid && GI->Shards-GI->ShardsSpent<APOCWorld::SwitchPrice)
                {
                    InsufficientShardsVerified=GI->ShardsSpent==Before && !Journey->GateIsOpen();
                    BacktrackStarted=true;TravelDirection=-1;LastProgress=Now;
                    UE_LOG(LogTemp,Display,TEXT("POC_QA BACKTRACK insufficient shards, door remains shut"));
                }
            }
        }
        else
        {
            Destination=Point.Position+Forward*780;
            if(FVector::Dist2D(Player->GetActorLocation(),Destination)<140) { Player->Echo();return; }
        }
    }
    else
    {
        const int32 NextIndex=FMath::Clamp(LastIndex+TravelDirection,0,Journey->Route.Num()-1);
        const auto& Next=Journey->Route[NextIndex]; Destination=Next.Position;
        if(Next.Kind==TEXT("moving") || Next.Kind==TEXT("lift"))
            for(TActorIterator<APOCProp> It(GetWorld());It;++It)
                if(It->Id==10000+NextIndex) { Destination=It->GetActorLocation();break; }
        // Run three deliberately misses late shards, then earns the missing
        // balance by returning along the staircase; no inventory manipulation.
        if(Run==3 && !BacktrackStarted && LastIndex>=112) Destination+=Right*290;
        const float Rise=Destination.Z-Player->GetActorLocation().Z+44;
        if(Next.Kind==TEXT("lift") && Grounded && Along>Point.Size.X*.5-220 && Rise>100)
        { Movement->StopMovementImmediately();LastProgress=Now;return; }
        const float Gap=FVector::Dist2D(Point.Position,Next.Position)-(Point.Size.X+Next.Size.X)*.5f;
        if(Grounded && Along>Point.Size.X*.5-(LastIndex<8 ? (Run==1 ? 100 : Run==2 ? 150 : 80) : 100) && !Player->bIsCrouched && (Gap>30 || Rise>45))
        { Player->JumpReleased();Player->JumpPressed();++JumpCount; }
        if(!Grounded && Player->AirTime>.27f && Movement->Velocity.Z<180 && !Player->DoubleJumpUsed && (Gap>350 || Rise>130))
        { Player->JumpReleased();Player->JumpPressed();++JumpCount; }
        if(!Grounded && Gap>600 && Player->DoubleJumpUsed && Movement->Velocity.Z<120 && !Player->AirDashUsed) Player->Dash();
    }
    const FVector Direction = (Destination - Player->GetActorLocation()).GetSafeNormal2D();
    PC->SetControlRotation(FRotator(-14, Direction.Rotation().Yaw, 0));
    Player->AddMovementInput(Direction, 1);
    if (Now - LastDiagnostic > 5)
    {
        LastDiagnostic = Now;
        if (const auto* Viewport = GetWorld()->GetGameViewport())
        {
            const FStatUnitData* Unit = Viewport->GetStatUnitData();
            UE_LOG(LogTemp, Display, TEXT("POC_QA TIMING game=%.2f render=%.2f gpu=%.2f frame=%.2f ms"), Unit->GameThreadTime, Unit->RenderThreadTime, Unit->GPUFrameTime[0], Unit->FrameTime);
        }
        UE_LOG(LogTemp, Display, TEXT("POC_QA STATE platform=%d xyz=%s velocity=%s grounded=%d shards=%d"), LastIndex, *Player->GetActorLocation().ToCompactString(), *Player->GetVelocity().ToCompactString(), Grounded, GI->Shards);
    }
#endif
}

void APOCJourneyTest::Finish(bool Passed, const FString& Reason)
{
#if !UE_BUILD_SHIPPING
    Finished = true;
    auto* GI = GetGameInstance<UPOCGameInstance>();
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetBoolField(TEXT("completed"), Passed && GI->Won && SectionProbe<0);
    Report->SetNumberField(TEXT("section_probe"),SectionProbe);
    Report->SetBoolField(TEXT("section_probe_passed"),SectionProbe>=0 && Passed);
    Report->SetBoolField(TEXT("restart_verified"), false);
    for(TActorIterator<APOCWorld> It(GetWorld());It;++It) Report->SetBoolField(TEXT("cake_door_open"),It->GateIsOpen());
    Report->SetNumberField(TEXT("shards_spent"),GI->ShardsSpent);
    Report->SetBoolField(TEXT("insufficient_shards_rejected"),InsufficientShardsVerified);
    Report->SetBoolField(TEXT("backtracked_and_earned_missing_shards"),BacktrackCompleted);
    Report->SetNumberField(TEXT("checkpoints_activated"), Checkpoints.Num());
    Report->SetNumberField(TEXT("echo_groups_activated"), EchoGroups.Num());
    Report->SetBoolField(TEXT("echo_expiry_reactivation_verified"), EchoExpiryCompleted);
    int32 Arenas=0,Lifts=0,Presses=0;
    for(TActorIterator<APOCProp> It(GetWorld());It;++It)
    {
        if(It->Kind==EPOCProp::ArenaGate && It->EncounterCleared) ++Arenas;
        if(It->Kind==EPOCProp::Lift && LandedPlatforms.Contains(It->Id-10000)) ++Lifts;
        if(It->Kind==EPOCProp::Crusher && LandedPlatforms.Contains(It->Id-40000)) ++Presses;
    }
    Report->SetNumberField(TEXT("sentry_courts_open"),Arenas);
    Report->SetNumberField(TEXT("lift_platforms_landed"),Lifts);
    Report->SetNumberField(TEXT("press_platforms_landed"),Presses);
    int32 Defeated = 0;
    for (TActorIterator<APOCEnemy> It(GetWorld()); It; ++It) if (It->IsHidden()) ++Defeated;
    Report->SetNumberField(TEXT("enemies_defeated"), Defeated);
    Report->SetStringField(TEXT("mode"), TEXT("automated engine physics traversal; not a manual/browser-input playthrough"));
    Report->SetStringField(TEXT("reason"), Reason);
    Report->SetNumberField(TEXT("run"), Run);
    Report->SetBoolField(TEXT("developer_flight_used"),false);
    Report->SetNumberField(TEXT("opening_tap_seconds"),.05);
    Report->SetNumberField(TEXT("opening_takeoff_lead_cm"),Run==1?100:Run==2?150:80);
    Report->SetNumberField(TEXT("quality"), GI->Save->Quality);
    int32 Width=0, Height=0;
    if (auto* PC=UGameplayStatics::GetPlayerController(this,0)) PC->GetViewportSize(Width,Height);
    Report->SetNumberField(TEXT("render_width"),Width);
    Report->SetNumberField(TEXT("render_height"),Height);
    Report->SetNumberField(TEXT("seconds"), FPlatformTime::Seconds()-StartedAt);
    Report->SetNumberField(TEXT("furthest_platform"), Furthest);
    Report->SetNumberField(TEXT("platforms_landed"), LandedPlatforms.Num());
    Report->SetNumberField(TEXT("respawns"), Respawns);
    Report->SetNumberField(TEXT("jumps"), JumpCount);
    auto* TestedPlayer = Cast<APOCCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    Report->SetNumberField(TEXT("double_jumps"),TestedPlayer ? TestedPlayer->DoubleJumpsPerformed : 0);
    Report->SetNumberField(TEXT("dashes"),TestedPlayer ? TestedPlayer->DashesPerformed : 0);
    Report->SetNumberField(TEXT("damage_events"),TestedPlayer ? TestedPlayer->DamageEvents : 0);
    Report->SetNumberField(TEXT("fall_deaths"),TestedPlayer ? TestedPlayer->FallDeaths : 0);
    Report->SetBoolField(TEXT("empty_hearts_before_checkpoint_observed"),EmptyHeartsObserved);
    Report->SetBoolField(TEXT("live_bomb_health_loss_verified"),BombDamageVerified);
    Report->SetBoolField(TEXT("live_bomb_knockback_verified"),BombKnockbackVerified);
    int32 Kicks=0, Explosions=0;
    for(TActorIterator<APOCProp> It(GetWorld());It;++It) { Kicks+=It->Kicks; Explosions+=It->Explosions; }
    Report->SetNumberField(TEXT("bomb_kicks"),Kicks);
    Report->SetNumberField(TEXT("bomb_explosions"),Explosions);
    Report->SetNumberField(TEXT("echo_inputs"), EchoCount);
    Report->SetNumberField(TEXT("bonk_inputs"), BonkCount);
    Report->SetNumberField(TEXT("slide_inputs"), SlideCount);
    Report->SetNumberField(TEXT("shards"), GI->Shards);
    Report->SetBoolField(TEXT("deliberate_fall_recovered"), FallCompleted);
    Report->SetBoolField(TEXT("pause_resume_exercised"), PauseCompleted);
    if (FrameMs.Num())
    {
        FrameMs.Sort(); double Total=0; for (double Ms : FrameMs) Total += Ms;
        int32 Slow33=0, Slow50=0, Slow100=0;
        for (double Ms : FrameMs) { Slow33 += Ms > 33.34; Slow50 += Ms > 50; Slow100 += Ms > 100; }
        Report->SetNumberField(TEXT("frames_over_33ms"), Slow33);
        Report->SetNumberField(TEXT("frames_over_50ms"), Slow50);
        Report->SetNumberField(TEXT("frames_over_100ms"), Slow100);
        Report->SetBoolField(TEXT("section_screenshots_during_run"), FParse::Param(FCommandLine::Get(), TEXT("POCScreenshots")));
        Report->SetNumberField(TEXT("frames"), FrameMs.Num());
        Report->SetNumberField(TEXT("mean_fps"), FrameMs.Num()*1000/Total);
        Report->SetNumberField(TEXT("p95_frame_ms"), FrameMs[FMath::Min(FrameMs.Num()-1, FMath::FloorToInt(FrameMs.Num()*.95))]);
        Report->SetNumberField(TEXT("p99_frame_ms"), FrameMs[FMath::Min(FrameMs.Num()-1, FMath::FloorToInt(FrameMs.Num()*.99))]);
        Report->SetNumberField(TEXT("worst_frame_ms"), FrameMs.Last());
    }
    FString Text; FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Text));
    const FString Directory = QAOutputDirectory();
    IFileManager::Get().MakeDirectory(*Directory, true);
    FFileHelper::SaveStringToFile(Text, *(Directory/FString::Printf(TEXT("journey-run-%d.json"),Run)));
    UE_LOG(LogTemp, Display, TEXT("POC_QA FINISH passed=%d %s"), Passed, *Reason);
    if (Passed && GI->Won)
    {
        GI->QARestartPending = true;
        if (auto* PC = Cast<APOCController>(UGameplayStatics::GetPlayerController(this, 0)))
        { PC->OpenMenu(EPOCMenu::Complete); PC->Selection = 0; PC->ActivateSelection(); }
    }
    else FPlatformMisc::RequestExit(false);
#endif
}
