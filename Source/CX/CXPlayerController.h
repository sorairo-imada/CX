// CXPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CXPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class ACXPawn;
class UCXUI;
class UCXHUDWidget;
class UCXChatWidget;

/**
 *  Vehicle Player Controller class
 *  Handles input mapping and user interface
 *
 *  [CX 추가]
 *  - HUD / 채팅 위젯 생성 (로컬 컨트롤러만)
 *  - 채팅 : Client -> ServerSendChatMessage -> GameState::MulticastBroadcastChatMessage -> 모든 클라 UI
 */
UCLASS(abstract)
class CX_API ACXPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	/** Input Mapping Context to be used for player input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* InputMappingContext;

	/** If true, the optional steering wheel input mapping context will be registered */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	bool bUseSteeringWheelControls = false;

	/** Optional Input Mapping Context to be used for steering wheel input.
	 *  This is added alongside the default Input Mapping Context and does not block other forms of input.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta=(EditCondition="bUseSteeringWheelControls"))
	UInputMappingContext* SteeringWheelInputMappingContext;

	/** (선택) 채팅창 열기 Action. 비워두면 Enter 키로 직접 바인딩 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* OpenChatAction;

	/** Pointer to the controlled vehicle pawn */
	UPROPERTY(Replicated)
	TObjectPtr<ACXPawn> VehiclePawn;

	/** Type of the UI to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	TSubclassOf<UCXUI> VehicleUIClass;

	/** Pointer to the UI widget */
	TObjectPtr<UCXUI> VehicleUI;

	/** [CX] 원화/속도/게임상태 HUD 위젯 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	TSubclassOf<UCXHUDWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UCXHUDWidget> HUDWidget;

	/** [CX] 채팅 위젯 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = UI)
	TSubclassOf<UCXChatWidget> ChatWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UCXChatWidget> ChatWidget;

	/** 채팅 최대 길이 */
	UPROPERTY(EditDefaultsOnly, Category = Chat)
	int32 MaxChatMessageLength = 100;

	/** 서버측 채팅 도배 방지 간격 */
	UPROPERTY(EditDefaultsOnly, Category = Chat)
	float ChatCooldown = 0.3f;

	bool bIsChatting = false;

	double LastChatServerTime = -1000.0;

	// Begin Actor interface
protected:

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:

	virtual void Tick(float Delta) override;

	// End Actor interface

	// ---------- Chat ----------

	/** [Local] 채팅창 입력 모드 진입 */
	void EnterChatMode();

	/** [Local] 게임 입력 모드로 복귀 */
	void ExitChatMode();

	/** [Local] 위젯에서 호출 → 서버로 전송 */
	void SendChatMessage(const FString& Message);

	/** [Local] GameState 멀티캐스트가 호출 → 채팅창에 출력 */
	void ReceiveChatMessage(const FString& SenderName, const FString& Message);

protected:
	UFUNCTION(Server, Reliable)
	void ServerSendChatMessage(const FString& Message);

	// Begin PlayerController interface
protected:

	virtual void OnPossess(APawn* InPawn) override;

	// End PlayerController interface
};
