#include "POCGameInstance.h"
#include "POCCloud.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/IConsoleManager.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericApplication.h"
#include "IPixelStreaming2Module.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"

namespace
{
    FString SaveSlot()
    {
#if !UE_BUILD_SHIPPING
        if (FParse::Param(FCommandLine::Get(), TEXT("POCPresentationTest"))) return TEXT("PieceOfCake_PresentationQA");
        if (FParse::Param(FCommandLine::Get(), TEXT("POCFlightTest"))) return TEXT("PieceOfCake_FlightQA");
        if (FString(FCommandLine::Get()).Contains(TEXT("POCCloudSmoke"))) return TEXT("PieceOfCake_CloudQA");
        int32 TestRun = 0;
        if (FParse::Value(FCommandLine::Get(), TEXT("POCAutoRun="), TestRun) && TestRun > 0) return TEXT("PieceOfCake_QA");
#endif
        return TEXT("PieceOfCake_v1");
    }
}

void UPOCGameInstance::Init()
{
    Super::Init();
    if (FSlateApplication::IsInitialized()) NativeApplication = FSlateApplication::Get().GetPlatformApplication();
    Save = Cast<UPOCSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlot(), 0));
    if (!Save) Save = Cast<UPOCSaveGame>(UGameplayStatics::CreateSaveGameObject(UPOCSaveGame::StaticClass()));
    Save->MusicVolume = FMath::Clamp(Save->MusicVolume, 0.f, 1.f);
    Save->EffectsVolume = FMath::Clamp(Save->EffectsVolume, 0.f, 1.f);
    Save->Sensitivity = FMath::Clamp(Save->Sensitivity, .25f, 2.f);
    Save->Quality = FMath::Clamp(Save->Quality, 0, 3);
    Cloud=NewObject<UPOCCloud>(this);Cloud->Initialize(this);
    // Cloud saves are opt-out for normal players: the first launch creates a
    // private anonymous Supabase profile, while QA command-line sessions exit
    // from UPOCCloud::Initialize before this path is reached.
    if (Cloud && Cloud->Configured()) Cloud->Connect();
}

void UPOCGameInstance::Shutdown()
{
    // UE 5.8.3's streaming input wrapper releases the native Mac application
    // during PreExit. Restore its owner before Slate destroys the game window.
    if (IPixelStreaming2Module::IsAvailable()) IPixelStreaming2Module::Get().StopStreaming();
    if (NativeApplication.IsValid() && FSlateApplication::IsInitialized())
        FSlateApplication::Get().OverridePlatformApplication(NativeApplication);
    Super::Shutdown();
}

void UPOCGameInstance::SaveProgress(bool SyncCloud)
{
    Save->Completed |= Won;
    if(SyncCloud && Won && !RunQueued && RunSeconds>=30 && Shards>=360)
    {
        FPOCPendingRun Run;Run.Id=RunId.ToString(EGuidFormats::DigitsWithHyphensLower);
        Run.Seconds=FMath::RoundToInt(RunSeconds);Run.Shards=Shards;Run.Relics=Relics;Run.Assisted=AssistedRun;
        Save->PendingRuns.Add(Run);RunQueued=true;
        // A tiny bounded outbox survives network loss and app restarts.
        if(Save->PendingRuns.Num()>100) Save->PendingRuns.RemoveAt(0,Save->PendingRuns.Num()-100);
    }
    Save->BestShards = FMath::Max(Save->BestShards, Shards);
    Save->BestRelics = FMath::Max(Save->BestRelics, Relics);
    if (!UGameplayStatics::SaveGameToSlot(Save, SaveSlot(), 0))
        UE_LOG(LogTemp, Warning, TEXT("Piece of Cake: could not write local save."));
    if(SyncCloud && Cloud) Cloud->Sync();
}

bool UPOCGameInstance::TrySpendShards(int32 Cost)
{
    if(Cost<=0 || Shards-ShardsSpent<Cost) return false;
    ShardsSpent+=Cost; return true;
}

void UPOCGameInstance::ResetRun()
{
    Shards = Relics = Section = ShardsSpent = 0;
    RunSeconds = 0;
    Won = false;
    Collected.Reset();
    RunId=FGuid::NewGuid();AssistedRun=false;RunQueued=false;
}

void UPOCGameInstance::ApplySettings()
{
    if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
    {
        Settings->SetVSyncEnabled(false);
        Settings->SetFrameRateLimit(60.f);
        Settings->SetOverallScalabilityLevel(Save->Quality);
        Settings->SetShadowQuality(0);
        Settings->SetResolutionScaleValueEx(Save->Quality >= 2 ? 100.f : Save->Quality == 1 ? 80.f : 75.f);
        int32 Width = 0, Height = 0;
        if (FParse::Param(FCommandLine::Get(), TEXT("ForceRes"))
            && FParse::Value(FCommandLine::Get(), TEXT("ResX="), Width)
            && FParse::Value(FCommandLine::Get(), TEXT("ResY="), Height))
            Settings->SetScreenResolution(FIntPoint(Width, Height));
        Settings->SetFullscreenMode(Save->Fullscreen ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
        Settings->ApplySettings(false);
        // Authored fill lighting and matte surfaces keep the enclosed world readable.
        // All presets honour the requested shadow-free art direction.
        const bool Advanced = false; // Shadow-free art direction at every quality level.
        IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicGlobalIlluminationMethod"))->Set(Advanced ? 1 : 0, ECVF_SetByCode);
        IConsoleManager::Get().FindConsoleVariable(TEXT("r.ReflectionMethod"))->Set(Advanced ? 1 : 0, ECVF_SetByCode);
        IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.Enable"))->Set(0, ECVF_SetByCode);
        IConsoleManager::Get().FindConsoleVariable(TEXT("r.ShadowQuality"))->Set(0, ECVF_SetByCode);
        IConsoleManager::Get().FindConsoleVariable(TEXT("r.AmbientOcclusionLevels"))->Set(0, ECVF_SetByCode);
        IConsoleManager::Get().FindConsoleVariable(TEXT("r.ContactShadows"))->Set(0, ECVF_SetByCode);
        IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod"))->Set(Advanced ? 4 : 2, ECVF_SetByCode);
        IConsoleManager::Get().FindConsoleVariable(TEXT("r.DistanceFieldAO"))->Set(Advanced ? 1 : 0, ECVF_SetByCode);
    }
    SaveProgress();
}
