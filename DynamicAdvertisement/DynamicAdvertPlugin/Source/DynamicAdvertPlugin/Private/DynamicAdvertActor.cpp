#include "IDynamicAdvertPlugin.h"
#include "DynamicAdvertActor.h"
#include "UObject/ConstructorHelpers.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/ImageWrapper/Public/IImageWrapperModule.h"
#include "MediaPlayer.h"
#include "MediaTexture.h"
#include <SocketSubsystem.h>
#include <IPAddress.h>
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Runtime/Engine/Classes/Engine/Texture2DDynamic.h"
#include "Runtime/Engine/Classes/Engine/Texture.h"
#include "AsyncTaskDownloadImage.h"
#include "Runtime/Engine/Classes/Materials/MaterialInstanceDynamic.h"


void ADynamicAdvertActor::BeginPlay()
{
	Super::BeginPlay();
	EnableInput(GetWorld()->GetFirstPlayerController());	
	StartGame(GameId, Address);
}


void ADynamicAdvertActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	EndGame();
}

bool ADynamicAdvertActor::StartGame(FString GameIdVal, FString ServerAddress)
{
	if (SessionStarted) {
		UE_LOG(LogTemp, Log, TEXT("Game session already started"));
		return false;
	}

	SessionStarted = false;
	GameId = GameIdVal;
	Address = ServerAddress;
	UE_LOG(LogTemp, Log, TEXT("Trying to start game session"));
	bool canBind = false;
	TSharedRef<FInternetAddr> localIp = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, canBind);
	PlayerId = localIp.Get().ToString(false);

	Http = &FHttpModule::Get();
	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	FString url = Address + StartCommand + GameIdVal;
	Request->SetURL(url);
	Request->SetVerb("POST");
	Request->SetContentAsString("player_id=" + PlayerId);
	Request->SetHeader("Content-Type", "application/x-www-form-urlencoded");
	Request->OnProcessRequestComplete().BindUObject(this, &ADynamicAdvertActor::OnStartGameCompleted);
	Request->ProcessRequest();
	return true;
}

bool ADynamicAdvertActor::FindAdvert(int32 id, UMediaPlayer* MediaPlayer, UStaticMeshComponent* MeshComponent, UMaterial* ParentMaterial, FName TextureParamName, bool Hd){
	if (TextureHandlingIdx != 0) {
		return false;
	}

	Material = ParentMaterial;
	if (!SessionStarted) {
		StartGameOnceMoreEvent.Broadcast();
		OnBlStartGameOnceMore();
		UE_LOG(LogTemp, Log, TEXT("Game session hasn't been started yet"));
		return false;
	}

	Mesh = MeshComponent;
	Material = ParentMaterial;
	TextureParam = TextureParamName;
	VideoPlayer = MediaPlayer;
	NeedHd = Hd;

	TextureHandlingIdx = 0;
	FString url = Address + LoadCommand + GameId;
	UE_LOG(LogTemp, Log, TEXT("FindAdv for GameOdject id = %d"), id);
	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->SetURL(url);
	Request->SetVerb("POST");
	Request->SetContentAsString("player_id=" + PlayerId + "&" + "game_object_id=" + FString::FromInt(id));
	Request->SetHeader("Content-Type", "application/x-www-form-urlencoded");
	Request->OnProcessRequestComplete().BindUObject(this, &ADynamicAdvertActor::OnFindAdvCompleted);
	Request->ProcessRequest();

	return true;
}

void ADynamicAdvertActor::EndGame()
{
	if (!SessionStarted) {
		return;
	}
	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->SetURL(Address + EndCommand + GameId);
	Request->SetContentAsString("player_id=" + PlayerId);
	Request->SetHeader("Content-Type", "application/x-www-form-urlencoded");
	Request->OnProcessRequestComplete().BindUObject(this, &ADynamicAdvertActor::OnStartGameCompleted);
	Request->SetVerb("POST");
	Request->ProcessRequest();
}

void ADynamicAdvertActor::OnStartGameCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("StartGame Response is: %d"), Response.Get()->GetResponseCode());
	if (Response.Get()->GetResponseCode() == 200) {
		SessionStarted = true;
	}

	StartGameCompletedEvent.Broadcast(SessionStarted);
	OnBlStartGameCompleted(SessionStarted);
}

void ADynamicAdvertActor::OnFindAdvCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("FindAdv Response is: %d"), Response.Get()->GetResponseCode());
	if (Response.Get()->GetResponseCode() == 200) {

		TSharedPtr<FJsonObject> JsonObject;

		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid()) {
			TArray<TSharedPtr<FJsonValue>> TextureArray = JsonObject->GetArrayField(TEXT("textures"));
			TextureHandlingIdx = 0;
			TextureUrls.Empty();
			for (int32 i = 0; i < TextureArray.Num(); i++)
			{
				FString str;
				const TSharedPtr< FJsonObject >* StrObj;
				if (TextureArray[i]->TryGetObject(StrObj))
				{
					TArray<TSharedPtr<FJsonValue>> Values;
					StrObj->Get()->Values.GenerateValueArray(Values);
					str = Values[0].Get()->AsString();
					TextureUrls.Add(str);
				}
			}

			TArray<TSharedPtr<FJsonValue>> VideoArray = JsonObject->GetArrayField(TEXT("videos"));
			for (int32 i = 0; i < VideoArray.Num(); i++)
			{
				FString str;
				const TSharedPtr< FJsonObject >* StrObj;
				if (VideoArray[i]->TryGetObject(StrObj))
				{
					FString HdString = StrObj->Get()->GetStringField("hd");
					FString SdString = StrObj->Get()->GetStringField("sd");

					if (HdString.IsEmpty() && SdString.IsEmpty()) {
						continue;
					}
					if (VideoPlayer != NULL) {
						if ((NeedHd && !HdString.IsEmpty())||(!NeedHd && SdString.IsEmpty())) {
							SetVideoToMesh(HdString, VideoPlayer);
						}
						else {
							SetVideoToMesh(SdString, VideoPlayer);
						}
					}
				}
			}

			TextureHandlingIdx = 0;
			if (TextureUrls.Num() <= TextureHandlingIdx) {
				return;
			}
			ConvertJpgToTexture(TextureUrls[TextureHandlingIdx]);
		}
	}
}

void ADynamicAdvertActor::OnEndGameCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("EndGame Response is: %d"), Response.Get()->GetResponseCode());
	if (Response.Get()->GetResponseCode() == 200) {
		SessionStarted = false;
	}

	EndGameCompletedEvent.Broadcast(!SessionStarted);
	OnEndGameCompleted(!SessionStarted);
}

void ADynamicAdvertActor::ExtractUri(TArray<TSharedPtr<FJsonValue>> &Array, const int32 &i, FString &str)
{
	const TSharedPtr< FJsonObject >* StrObj;
	if (Array[i]->TryGetObject(StrObj))
	{
		TArray<TSharedPtr<FJsonValue>> Values;
		StrObj->Get()->Values.GenerateValueArray(Values);
		str = Values[0].Get()->AsString();
		UE_LOG(LogTemp, Log, TEXT("url = %s"), &str);
	}
}

void ADynamicAdvertActor::ConvertJpgToTexture(FString Url)
{
	// Represents the entire file in memory.
	TArray<uint8> RawFileData;
	UAsyncTaskDownloadImage* DownloadImage = UAsyncTaskDownloadImage::DownloadImage(Url);
	DownloadImage->OnSuccess.AddDynamic(this, &ADynamicAdvertActor::OnJpgOpened);
	DownloadImage->OnFail.AddDynamic(this, &ADynamicAdvertActor::OnJpgFailed);
}


bool ADynamicAdvertActor::SetVideoToMesh(FString Uri, UMediaPlayer* MediaPlayer) {

	if (!MediaPlayer) {
		TextureLoadingCompletedEvent.Broadcast(-1, false);
		OnBlTextureLoadingCompleted(-1, false);
		return false;
	}
	
	/*if (MediaPlayer->GetUrl().Compare(Uri)) {
		MediaPlayer->Rewind();
		VideoLoadingCompletedEvent.Broadcast(true);
		OnBlVideoLoadingCompleted(true);
		return false;
	}*/

	MediaPlayer->OnMediaOpened.AddDynamic(this, &ADynamicAdvertActor::OnMediaOpened);
	MediaPlayer->OnMediaOpenFailed.AddDynamic(this, &ADynamicAdvertActor::OnMediaFailed);

	MediaPlayer->SetLooping(false);

	if (MediaPlayer->OpenUrl(Uri)) {
		MediaPlayer->Rewind();
		return true;
	}

	return false;
}

void ADynamicAdvertActor::OnMediaOpened(FString str)
{
	UE_LOG(LogTemp, Log, TEXT("Video opened"));
	VideoLoadingCompletedEvent.Broadcast(true);
	OnBlVideoLoadingCompleted(true);
}

void ADynamicAdvertActor::OnMediaFailed(FString str)
{
	UE_LOG(LogTemp, Log, TEXT("Video failed"));
	VideoLoadingCompletedEvent.Broadcast(false);
	OnBlVideoLoadingCompleted(false);
}

void ADynamicAdvertActor::OnJpgOpened(UTexture2DDynamic* Texture)
{
	if (Mesh == 0) {
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("Jpg %d opened"), TextureHandlingIdx);
	UTexture* T = Texture;
	UMaterialInstanceDynamic * DynamicMaterial = UMaterialInstanceDynamic::Create(Material, Mesh);
	DynamicMaterial->SetTextureParameterValue(TextureParam, Texture);
	Mesh->SetMaterial(TextureHandlingIdx, DynamicMaterial);

	TextureLoadingCompletedEvent.Broadcast(TextureHandlingIdx, true);
	OnBlTextureLoadingCompleted(TextureHandlingIdx, true);

	TextureHandlingIdx++;
	if (TextureUrls.Num() <= TextureHandlingIdx ) {
		TextureHandlingIdx = 0;
		return;
	}
	ConvertJpgToTexture(TextureUrls[TextureHandlingIdx]);
}

void ADynamicAdvertActor::OnJpgFailed(UTexture2DDynamic* Texture)
{
	UE_LOG(LogTemp, Log, TEXT("Jpg %d failed"), TextureHandlingIdx);
	TextureLoadingCompletedEvent.Broadcast(TextureHandlingIdx, false);
	OnBlTextureLoadingCompleted(TextureHandlingIdx, false);
	TextureHandlingIdx++;
	if (TextureUrls.Num() <= TextureHandlingIdx ) {
		TextureHandlingIdx = 0;
		return;
	}
	ConvertJpgToTexture(TextureUrls[TextureHandlingIdx]);
}

