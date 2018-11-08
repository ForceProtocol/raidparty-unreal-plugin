#pragma once

#include "Runtime/Online/HTTP/Public/Http.h"
#include"Runtime/Engine/Classes/Engine/StaticMesh.h"
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DynamicAdvertActor.generated.h"

UCLASS()
class DYNAMICADVERTPLUGIN_API ADynamicAdvertActor : public AActor
{
  GENERATED_BODY()

protected:
	DECLARE_EVENT_OneParam(ADynamicAdvertisementGameMode, StartGameCompleted, bool);
	DECLARE_EVENT(ADynamicAdvertisementGameMode, StartGameOnceMore);
	DECLARE_EVENT_OneParam(ADynamicAdvertisementGameMode, EndGameCompleted, bool);
	DECLARE_EVENT_OneParam(ADynamicAdvertisementGameMode, HDVideoLoadingCompleted, bool);
	DECLARE_EVENT_OneParam(ADynamicAdvertisementGameMode, SDVideoLoadingCompleted, bool);
	DECLARE_EVENT_TwoParams(ADynamicAdvertisementGameMode, TextureLoadingCompleted, int, bool);


	const FString  StartCommand = "/start-session";
	const FString  EndCommand = "/end-session";
	FString GameId = "/b721c655-14e8-4636-b865-df2fa4873235";
	FString  Address = "http://c156057b.ngrok.io/sdk/advert";
	FString  LoadCommand = "/game-object";
	FString PlayerId;
	FHttpModule* Http;
	UStaticMesh* CurMesh;
	UMediaPlayer* VideoPlayer;
	UMaterial* Material;
	UStaticMeshComponent* Mesh;
	TArray<FString> TextureUrls;
	int TextureHandlingIdx;
	FName TextureParam;
	bool NeedHd;

	bool SessionStarted;
	void OnStartGameCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnFindAdvCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnEndGameCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void ExtractUri(TArray<TSharedPtr<FJsonValue>> &Array, const int32 &i, FString &str);
	void ConvertJpgToTexture(FString Url);
	void SetTextureToMesh(UTexture2D* Texture);
	bool SetVideoToMesh(FString Uri, UMediaPlayer* MediaPlayer);
	UFUNCTION()
		void OnMediaOpened(FString str);
	UFUNCTION()
		void OnMediaFailed(FString str);
	UFUNCTION()
		void OnJpgOpened(UTexture2DDynamic* Texture);
	UFUNCTION()
		void OnJpgFailed(UTexture2DDynamic* Texture);

public:  
  virtual void BeginPlay() override;  
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  UFUNCTION(BlueprintCallable)
	  bool FindAdvert(int id, UMediaPlayer* MediaPlayer, UStaticMeshComponent* Mesh, UMaterial* ParentMaterial, FName TextureParamName,  bool NeedHd = false);
  UFUNCTION(BlueprintCallable)
	  bool StartGame(FString GameId, FString ServerAddress);
  UFUNCTION(BlueprintCallable)
	  void EndGame();

  StartGameCompleted		StartGameCompletedEvent;
  StartGameOnceMore			StartGameOnceMoreEvent;
  HDVideoLoadingCompleted	VideoLoadingCompletedEvent;
  TextureLoadingCompleted	TextureLoadingCompletedEvent;
  EndGameCompleted			EndGameCompletedEvent;

  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	  void OnBlStartGameCompleted(bool result);
  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	  void OnBlStartGameOnceMore();
  UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	  void OnBlVideoLoadingCompleted(bool Result);
  UFUNCTION(BlueprintImplementableEvent)
	  void OnBlTextureLoadingCompleted(int MaterialIdx, bool Result);
  UFUNCTION(BlueprintImplementableEvent)
	  void OnEndGameCompleted(bool Result);
 
   
};
