#include "POCGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"

void UPOCGameInstance::Init()
{
    Super::Init();
    Save = Cast<UPOCSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("PieceOfCake_v1"), 0));
    if (!Save) Save = Cast<UPOCSaveGame>(UGameplayStatics::CreateSaveGameObject(UPOCSaveGame::StaticClass()));
    Save->MusicVolume = FMath::Clamp(Save->MusicVolume, 0.f, 1.f);
    Save->EffectsVolume = FMath::Clamp(Save->EffectsVolume, 0.f, 1.f);
    Save->Sensitivity = FMath::Clamp(Save->Sensitivity, .25f, 2.f);
    Save->Quality = FMath::Clamp(Save->Quality, 0, 3);
}

void UPOCGameInstance::SaveProgress()
{
    Save->Completed |= Won;
    Save->BestShards = FMath::Max(Save->BestShards, Shards);
    Save->BestRelics = FMath::Max(Save->BestRelics, Relics);
    if (!UGameplayStatics::SaveGameToSlot(Save, TEXT("PieceOfCake_v1"), 0))
        UE_LOG(LogTemp, Warning, TEXT("Piece of Cake: could not write local save."));
}

void UPOCGameInstance::ResetRun()
{
    Shards = Relics = Section = 0;
    RunSeconds = 0;
    Won = false;
    Collected.Reset();
}

void UPOCGameInstance::ApplySettings()
{
    if (UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings())
    {
        Settings->SetOverallScalabilityLevel(Save->Quality);
        Settings->SetFullscreenMode(Save->Fullscreen ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
        Settings->ApplySettings(false);
    }
    SaveProgress();
}
