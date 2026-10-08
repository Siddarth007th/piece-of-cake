#include "Misc/AutomationTest.h"
#include "POCGameInstance.h"
#include "POCCharacter.h"
#include "POCDeveloperCode.h"
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
    FPOCPendingRun Run;Run.Id=TEXT("00000000-0000-4000-8000-000000000001");Run.Seconds=381;Run.Shards=420;Run.Assisted=true;Original->PendingRuns.Add(Run);
    TArray<uint8> Bytes;
    TestTrue(TEXT("Save serializes"), UGameplayStatics::SaveGameToMemory(Original, Bytes));
    UPOCSaveGame* Restored = Cast<UPOCSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Save can be restored"), Restored)) return false;
    TestEqual(TEXT("Cloud outbox survives restart"),Restored->PendingRuns.Num(),1);
    if(Restored->PendingRuns.Num()==1) {TestEqual(TEXT("Cloud retry ID stays stable"),Restored->PendingRuns[0].Id,Run.Id);TestTrue(TEXT("Assisted flag survives offline retry"),Restored->PendingRuns[0].Assisted);}
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPOCShardBudget, "PieceOfCake.Reward.ShardBudget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPOCShardBudget::RunTest(const FString& Parameters)
{
    auto* GI=NewObject<UPOCGameInstance>(); GI->Shards=359;
    TestTrue(TEXT("First switch accepts 180 shards"),GI->TrySpendShards(180));
    TestFalse(TEXT("One short cannot power the second switch"),GI->TrySpendShards(180));
    TestEqual(TEXT("Rejected payment preserves all remaining shards"),GI->ShardsSpent,180);
    ++GI->Shards;
    TestTrue(TEXT("Returning with the missing shard permits payment"),GI->TrySpendShards(180));
    TestEqual(TEXT("Both switches cost exactly 360"),GI->ShardsSpent,360);
    TestFalse(TEXT("Negative cost cannot mint shards"),GI->TrySpendShards(-1));
    GI->ResetRun(); TestEqual(TEXT("Restart clears gate spending"),GI->ShardsSpent,0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPOCDeveloperPhrase, "PieceOfCake.Input.DeveloperPhrase", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPOCDeveloperPhrase::RunTest(const FString& Parameters)
{
    FPOCDeveloperCode Code; double Time=0; bool Consume=false; int32 Toggles=0;
    auto Type=[&](const FString& Words) { for(TCHAR C:Words) { Time+=.1; Toggles+=Code.Push(C,Time,Consume); } };
    Type(TEXT("wawsd")); TestEqual(TEXT("Movement does not enable flight"),Toggles,0);
    Code.Reset(); Type(TEXT("s")); TestFalse(TEXT("Ordinary backward key is not swallowed"),Consume);
    Type(TEXT("i")); TestTrue(TEXT("Distinctive prefix consumes later gameplay bindings"),Consume);
    Type(TEXT("ddarthisgod")); TestEqual(TEXT("Full phrase toggles once"),Toggles,1);
    Type(TEXT("SIDDARTHISGOD")); TestEqual(TEXT("Case insensitive second toggle"),Toggles,2);
    Type(TEXT("siddarth")); Time+=4; Type(TEXT("isgod")); TestEqual(TEXT("Partial stale phrase expires"),Toggles,2);
    Type(TEXT("siddarthXisgod")); TestEqual(TEXT("Wrong letter cannot activate flight"),Toggles,2);
    Type(TEXT("ssiddarthisgod")); TestEqual(TEXT("Prefix overlap recovers"),Toggles,3);
    return true;
}
#endif
