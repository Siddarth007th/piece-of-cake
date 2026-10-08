#include "POCCloud.h"
#include "POCGameInstance.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformTime.h"
#if PLATFORM_MAC
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#endif
namespace {
 FString JSON(const TSharedRef<FJsonObject>& O) {FString S;FJsonSerializer::Serialize(O,TJsonWriterFactory<>::Create(&S));return S;}
 TSharedPtr<FJsonObject> Parse(const FString& S) {
  TSharedPtr<FJsonObject> O;if(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),O))return O;
  TArray<TSharedPtr<FJsonValue>> Rows;if(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),Rows) && Rows.Num()==1)return Rows[0]->AsObject();
  return nullptr;
 }
#if PLATFORM_MAC
 CFMutableDictionaryRef TokenQuery(const FString& URL)
 {
  auto Q=CFDictionaryCreateMutable(nullptr,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);
  CFDictionarySetValue(Q,kSecClass,kSecClassGenericPassword);
  CFDictionarySetValue(Q,kSecAttrService,CFSTR("com.siddarth007th.pieceofcake.cloud"));
  auto Account=CFStringCreateWithCString(nullptr,TCHAR_TO_UTF8(*(FString(FCommandLine::Get()).Contains(TEXT("POCCloudSmoke")) ? URL+TEXT("/qa-smoke") : URL)),kCFStringEncodingUTF8);
  CFDictionarySetValue(Q,kSecAttrAccount,Account);CFRelease(Account);return Q;
 }
#endif
}
void UPOCCloud::Initialize(UPOCGameInstance* Owner)
{
 Game=Owner;
 // Automated traversal never posts artificial scores or opens the owner's Keychain.
 const FString Cmd=FCommandLine::Get();
 if(Cmd.Contains(TEXT("POCAutoRun")) || Cmd.Contains(TEXT("POCPresentationTest")) || Cmd.Contains(TEXT("POCFlightTest")) || Cmd.Contains(TEXT("POCCameraTest"))) {Status=TEXT("Cloud disabled during automated tests");return;}
 // Cooked games reject arbitrary loose INI files. The packager merges the public
 // connection into DefaultGame.ini, which Unreal loads through its normal config hierarchy.
 GConfig->GetString(TEXT("PieceOfCake.Cloud"),TEXT("URL"),URL,GGameIni);
 GConfig->GetString(TEXT("PieceOfCake.Cloud"),TEXT("PublishableKey"),Key,GGameIni);
 if(!FPlatformProperties::RequiresCookedData() && URL.IsEmpty())
 {
  FConfigFile Config;Config.Read(FPaths::ProjectDir()/TEXT("Config/Cloud.ini"));
  Config.GetString(TEXT("PieceOfCake.Cloud"),TEXT("URL"),URL);
  Config.GetString(TEXT("PieceOfCake.Cloud"),TEXT("PublishableKey"),Key);
 }
 // Only an HTTPS Supabase project and a public key are allowed in packaged config.
 if(!URL.StartsWith(TEXT("https://")) || !URL.EndsWith(TEXT(".supabase.co")) || !Key.StartsWith(TEXT("sb_publishable_"))) {URL.Empty();Key.Empty();Status=TEXT("Cloud service is not configured");return;}
 RefreshToken=LoadToken(); if(!RefreshToken.IsEmpty()) Authenticate(false);
}
FString UPOCCloud::LoadToken()
{
#if PLATFORM_MAC
 auto Q=TokenQuery(URL);CFDictionarySetValue(Q,kSecReturnData,kCFBooleanTrue);CFDictionarySetValue(Q,kSecMatchLimit,kSecMatchLimitOne);
 CFTypeRef Result=nullptr;const OSStatus Code=SecItemCopyMatching(Q,&Result);CFRelease(Q);
 if(Code==errSecSuccess && Result) {auto Data=(CFDataRef)Result;FUTF8ToTCHAR Value((const ANSICHAR*)CFDataGetBytePtr(Data),CFDataGetLength(Data));FString S(Value.Length(),Value.Get());CFRelease(Result);return S;}
#endif
 return FString();
}
void UPOCCloud::StoreToken(const FString& Token)
{
#if PLATFORM_MAC
 auto Q=TokenQuery(URL);
 if(Token.IsEmpty()) {SecItemDelete(Q);CFRelease(Q);return;}
 FTCHARToUTF8 UTF(*Token);auto D=CFDataCreate(nullptr,(const UInt8*)UTF.Get(),UTF.Length());
 auto Update=CFDictionaryCreateMutable(nullptr,0,&kCFTypeDictionaryKeyCallBacks,&kCFTypeDictionaryValueCallBacks);CFDictionarySetValue(Update,kSecValueData,D);
 OSStatus Result=SecItemUpdate(Q,Update);
 if(Result==errSecItemNotFound) {CFDictionarySetValue(Q,kSecValueData,D);CFDictionarySetValue(Q,kSecAttrAccessible,kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly);Result=SecItemAdd(Q,nullptr);}
 CFRelease(D);CFRelease(Update);CFRelease(Q);
 if(Result!=errSecSuccess) UE_LOG(LogTemp,Warning,TEXT("Cloud session could not be retained in Keychain; reconnect next launch."));
#endif
}
void UPOCCloud::Request(const FString& Path,const FString& Body,TFunction<void(int32,const FString&)> Done)
{
 auto R=FHttpModule::Get().CreateRequest();R->SetURL(URL+Path);R->SetVerb(Body.IsEmpty()?TEXT("GET"):TEXT("POST"));R->SetTimeout(8.f);
 R->SetHeader(TEXT("apikey"),Key);R->SetHeader(TEXT("Content-Type"),TEXT("application/json"));
 if(!AccessToken.IsEmpty()) R->SetHeader(TEXT("Authorization"),TEXT("Bearer ")+AccessToken);
 if(!Body.IsEmpty()) R->SetContentAsString(Body);
 R->OnProcessRequestComplete().BindLambda([Weak=TWeakObjectPtr<UPOCCloud>(this),Done=MoveTemp(Done)](FHttpRequestPtr,FHttpResponsePtr Response,bool OK){if(Weak.IsValid()) Done(OK&&Response.IsValid()?Response->GetResponseCode():0,Response.IsValid()?Response->GetContentAsString():FString());});
 R->ProcessRequest();
}
void UPOCCloud::Connect()
{
 if(Busy)return;
 if(!Configured()) {Status=TEXT("Cloud service is not configured");return;}
 if(Connected() && FPlatformTime::Seconds()<ExpiresAt) {Sync();RefreshBoard();return;}
 Authenticate(RefreshToken.IsEmpty());
}
void UPOCCloud::Authenticate(bool NewPlayer)
{
 if(Busy)return;Busy=true;Status=TEXT("Connecting cloud profile...");
 auto Body=MakeShared<FJsonObject>();if(!NewPlayer)Body->SetStringField(TEXT("refresh_token"),RefreshToken);
 Request(NewPlayer?TEXT("/auth/v1/signup"):TEXT("/auth/v1/token?grant_type=refresh_token"),JSON(Body),[this](int32 Code,const FString& Text){
  Busy=false;auto O=Parse(Text);FString Access,Refresh;
  if(Code<200 || Code>=300 || !O || !O->TryGetStringField(TEXT("access_token"),Access) || !O->TryGetStringField(TEXT("refresh_token"),Refresh)) {
   Status=Code==0?TEXT("Cloud unavailable. Your game can still start."):TEXT("Cloud connection failed. Try again later.");return;}
  AccessToken=Access;RefreshToken=Refresh;double TTL=3600;O->TryGetNumberField(TEXT("expires_in"),TTL);ExpiresAt=FPlatformTime::Seconds()+FMath::Max(30.0,TTL-60);StoreToken(RefreshToken);Sync();RefreshBoard();
 });
}
void UPOCCloud::Sync()
{
 if(Busy || !Connected())return;
 if(FPlatformTime::Seconds()>ExpiresAt) {Authenticate(false);return;}
 auto O=MakeShared<FJsonObject>();O->SetNumberField(TEXT("p_shards"),Game->Save->BestShards);O->SetNumberField(TEXT("p_relics"),Game->Save->BestRelics);O->SetBoolField(TEXT("p_completed"),Game->Save->Completed);
 const FString Payload=JSON(O);
 if(Payload==LastProgress) {SubmitRun();return;}
 Busy=true;Status=TEXT("Saving to cloud...");
 Request(TEXT("/rest/v1/rpc/sync_progress"),Payload,[this,Payload](int32 Code,const FString& Text){
  Busy=false;auto R=Parse(Text);
  if(Code<200 || Code>=300 || !R) {Status=TEXT("Cloud save pending. Reconnect to retry.");return;}
  Game->Save->BestShards=FMath::Max(Game->Save->BestShards,R->GetIntegerField(TEXT("best_shards")));
  Game->Save->BestRelics=FMath::Max(Game->Save->BestRelics,R->GetIntegerField(TEXT("best_relics")));
  Game->Save->Completed|=R->GetBoolField(TEXT("completed"));LastProgress=Payload;Status=TEXT("Cloud progress saved");
  // Cache a successful merge without recursively scheduling another request.
  Game->SaveProgress(false);SubmitRun();
 });
}
void UPOCCloud::SubmitRun()
{
 if(!Connected() || RunBusy || Game->Save->PendingRuns.IsEmpty()) return;
 const auto Run=Game->Save->PendingRuns[0];
 auto O=MakeShared<FJsonObject>();O->SetStringField(TEXT("p_run_id"),Run.Id);O->SetNumberField(TEXT("p_seconds"),Run.Seconds);
 O->SetNumberField(TEXT("p_shards"),Run.Shards);O->SetNumberField(TEXT("p_relics"),Run.Relics);O->SetBoolField(TEXT("p_assisted"),Run.Assisted);
 RunBusy=true;
 Request(TEXT("/rest/v1/rpc/finish_run"),JSON(O),[this,ID=Run.Id](int32 Code,const FString&){
  RunBusy=false;
  if(Code>=200 && Code<300){
   Game->Save->PendingRuns.RemoveAll([&ID](const FPOCPendingRun& R){return R.Id==ID;});
   Game->SaveProgress(false);Status=TEXT("Cloud progress and run saved");RefreshBoard();SubmitRun();
  }else Status=TEXT("Run saved for retry. Open Cloud saves to reconnect.");
 });
}
void UPOCCloud::RefreshBoard()
{
 if(!Connected())return;
 Request(TEXT("/rest/v1/rpc/cake_leaderboard"),TEXT("{}"),[this](int32 Code,const FString& Text){
  if(Code!=200)return;TArray<TSharedPtr<FJsonValue>> Rows;
  if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Rows))return;
  Board.Reset();for(int32 I=0;I<FMath::Min(8,Rows.Num());++I){auto R=Rows[I]->AsObject();if(!R)continue;int32 Seconds=R->GetIntegerField(TEXT("seconds"));Board.Add(FString::Printf(TEXT("%d.  %s     %d:%02d"),I+1,*R->GetStringField(TEXT("nickname")),Seconds/60,Seconds%60));}
 });
}
void UPOCCloud::Disconnect()
{
 // Forget only this app's credential. Cloud records are preserved.
 StoreToken(TEXT(""));AccessToken.Empty();RefreshToken.Empty();LastProgress.Empty();Board.Reset();Status=TEXT("Cloud saves are off");
}
