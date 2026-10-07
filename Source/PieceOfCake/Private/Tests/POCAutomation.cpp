#include "Misc/AutomationTest.h"
#include "POCGameInstance.h"
#include "POCCharacter.h"
#include "POCVisuals.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPOCSaveRoundTrip, "PieceOfCake.Persistence.MemoryRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPOCSaveRoundTrip::RunTest(const FString& Parameters)
{
    UPOCSaveGame* Original = NewObject<UPOCSaveGame>();
    Original->Completed = true;
    Original->BestShards = 215;
    Original->Sensitivity = 1.35;
    Original->MusicVolume = .25;
    TArray<uint8> Bytes;
    TestTrue(TEXT("Save serializes"), UGameplayStatics::SaveGameToMemory(Original, Bytes));
    UPOCSaveGame* Restored = Cast<UPOCSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Save can be restored"), Restored)) return false;
    TestTrue(TEXT("Completion persists"), Restored->Completed);
    TestEqual(TEXT("Collectible best persists"), Restored->BestShards, 215);
    TestEqual(TEXT("Sensitivity persists"), Restored->Sensitivity, 1.35f);
    TestEqual(TEXT("Music persists"), Restored->MusicVolume, .25f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPOCCookedRoute, "PieceOfCake.Content.RouteAndPhysics", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPOCCookedRoute::RunTest(const FString& Parameters)
{
    FString Text;
    TSharedPtr<FJsonObject> Data;
    if (!TestTrue(TEXT("Staged route is readable"), FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("Data/journey.json"))))) return false;
    if (!TestTrue(TEXT("Route parses"), FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Data))) return false;
    const auto Physics = Data->GetObjectField(TEXT("movement"));
    const auto* Character = GetDefault<APOCCharacter>();
    const auto* Movement = Character->GetCharacterMovement();
    TestEqual(TEXT("Physics route validator uses actual run speed"), Character->RunSpeed, static_cast<float>(Physics->GetNumberField(TEXT("run_speed"))));
    TestEqual(TEXT("Physics route validator uses actual jump velocity"), Movement->JumpZVelocity, static_cast<float>(Physics->GetNumberField(TEXT("jump_speed"))));
    TestEqual(TEXT("Physics route validator uses actual gravity"), Movement->GravityScale * 980.f, static_cast<float>(Physics->GetNumberField(TEXT("gravity"))));
    TestTrue(TEXT("Material exists"), StaticLoadObject(UObject::StaticClass(), nullptr, TEXT("/Game/Materials/M_World.M_World")) != nullptr);
    TestTrue(TEXT("Cake payoff audio exists"), StaticLoadObject(UObject::StaticClass(), nullptr, TEXT("/Game/Audio/A_Cake.A_Cake")) != nullptr);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPOCOriginalMeshes, "PieceOfCake.Content.OriginalMeshes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPOCOriginalMeshes::RunTest(const FString& Parameters)
{
    const TCHAR* Names[] = {TEXT("NoriBody"), TEXT("NoriHead"), TEXT("NoriEar"), TEXT("NoriPaw"),
        TEXT("NoriPack"), TEXT("NoriScarf"), TEXT("DressedStone"), TEXT("WeatheredRock"), TEXT("Fern")};
    for (const TCHAR* Name : Names)
    {
        UStaticMesh* Mesh = POCVisuals::Shape(Name);
        if (!TestNotNull(FString::Printf(TEXT("Imported mesh %s exists"), Name), Mesh)) continue;
        const FVector Size = Mesh->GetBounds().BoxExtent * 2;
        TestTrue(FString::Printf(TEXT("Mesh %s has centimetre dimensions"), Name),
            Size.GetMin() > 25 && Size.GetMax() < 110);
        if (FCString::Strcmp(Name, TEXT("NoriEar")) == 0)
            TestEqual(TEXT("Ear retains cream and teal material slots"), Mesh->GetStaticMaterials().Num(), 2);
    }
    return true;
}
#endif
