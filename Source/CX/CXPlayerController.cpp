// CXPlayerController.cpp


#include "CXPlayerController.h"
#include "CX.h"
#include "CXPawn.h"
#include "CXUI.h"
#include "CXHUDWidget.h"
#include "CXChatWidget.h"
#include "CXGameState.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

void ACXPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() == false)
	{
		return;
	}

	// spawn the UI widget and add it to the viewport
	VehicleUI = CreateWidget<UCXUI>(this, VehicleUIClass);

	check(VehicleUI);

	VehicleUI->AddToViewport();

	// [CX] HUD
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UCXHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(1);
		}
	}
	else
	{
		UE_LOG(LogCX, Warning, TEXT("HUDWidgetClass is not set in %s"), *GetClass()->GetName());
	}

	// [CX] Chat
	if (ChatWidgetClass)
	{
		ChatWidget = CreateWidget<UCXChatWidget>(this, ChatWidgetClass);
		if (ChatWidget)
		{
			ChatWidget->AddToViewport(2);
		}
	}
	else
	{
		UE_LOG(LogCX, Warning, TEXT("ChatWidgetClass is not set in %s"), *GetClass()->GetName());
	}

	SetInputMode(FInputModeGameOnly());
}

void ACXPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// get the enhanced input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// add the mapping context so we get controls
		Subsystem->AddMappingContext(InputMappingContext, 0);

		// optionally add the steering wheel context
		if (bUseSteeringWheelControls && SteeringWheelInputMappingContext)
		{
			Subsystem->AddMappingContext(SteeringWheelInputMappingContext, 1);
		}
	}

	// [CX] 채팅 열기 (Enter)
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (OpenChatAction && EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(OpenChatAction, ETriggerEvent::Started, this, &ThisClass::EnterChatMode);
	}
	else if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ThisClass::EnterChatMode);
	}
}

void ACXPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, VehiclePawn);
}

void ACXPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	if (IsValid(VehiclePawn) && IsValid(VehicleUI))
	{
		VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
		VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
	}
}

void ACXPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// get a pointer to the controlled pawn
	VehiclePawn = Cast<ACXPawn>(InPawn);
}

// ============================================================
// Chat
// ============================================================

void ACXPlayerController::EnterChatMode()
{
	if (IsLocalController() == false || IsValid(ChatWidget) == false || bIsChatting)
	{
		return;
	}

	bIsChatting = true;

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(ChatWidget->GetChatInputSlateWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	ChatWidget->FocusChatInput();
}

void ACXPlayerController::ExitChatMode()
{
	// 커밋 → 포커스 이동 → 다시 커밋 이벤트가 들어올 수 있으므로 재진입 방지
	if (bIsChatting == false)
	{
		return;
	}

	bIsChatting = false;
	SetInputMode(FInputModeGameOnly());
}

void ACXPlayerController::SendChatMessage(const FString& Message)
{
	const FString TrimmedMessage = Message.TrimStartAndEnd();
	if (TrimmedMessage.IsEmpty())
	{
		return;
	}

	ServerSendChatMessage(TrimmedMessage.Left(MaxChatMessageLength));
}

void ACXPlayerController::ServerSendChatMessage_Implementation(const FString& Message)
{
	// 서버 유효성 검사 : 빈 문자열 / 길이 / 도배
	FString CleanMessage = Message.TrimStartAndEnd();
	if (CleanMessage.IsEmpty())
	{
		return;
	}
	CleanMessage.LeftInline(MaxChatMessageLength);

	UWorld* World = GetWorld();
	if (IsValid(World) == false)
	{
		return;
	}

	const double Now = World->GetTimeSeconds();
	if (Now - LastChatServerTime < ChatCooldown)
	{
		return;
	}
	LastChatServerTime = Now;

	ACXGameState* CXGameState = World->GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	// 보낸 사람 이름은 클라가 보낸 값이 아니라 서버의 PlayerState 기준
	const FString SenderName = IsValid(PlayerState) ? PlayerState->GetPlayerName() : TEXT("Unknown");

	CXGameState->MulticastBroadcastChatMessage(SenderName, CleanMessage);
}

void ACXPlayerController::ReceiveChatMessage(const FString& SenderName, const FString& Message)
{
	if (IsValid(ChatWidget))
	{
		ChatWidget->AddChatMessage(SenderName, Message);
	}
}
