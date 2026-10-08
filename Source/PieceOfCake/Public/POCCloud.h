#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "POCCloud.generated.h"
class UPOCGameInstance;
UCLASS()
class PIECEOFCAKE_API UPOCCloud : public UObject
{
 GENERATED_BODY()
public:
 void Initialize(UPOCGameInstance* Owner);
 void Connect();
 void Sync();
 void RefreshBoard();
 void SubmitRun();
 void Disconnect();
 bool Configured() const { return !URL.IsEmpty() && !Key.IsEmpty(); }
 bool Connected() const { return !AccessToken.IsEmpty(); }
 FString Status=TEXT("Cloud saves are off");
 TArray<FString> Board;
private:
 UPROPERTY() TObjectPtr<UPOCGameInstance> Game;
 FString URL,Key,AccessToken,RefreshToken,LastProgress,LastRun;
 double ExpiresAt=0;
 bool Busy=false;
 bool RunBusy=false;
 void Authenticate(bool NewPlayer);
 void StoreToken(const FString& Token);
 FString LoadToken();
 void Request(const FString& Path,const FString& Body,TFunction<void(int32,const FString&)> Done);
};
