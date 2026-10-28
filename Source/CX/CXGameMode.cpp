// CXGameMode.cpp

#include "CXGameMode.h"
#include "CX.h"
#include "CXPlayerController.h"
#include "CXPlayerState.h"
#include "CXGameState.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"

ACXGameMode::ACXGameMode()
{
	PlayerControllerClass = ACXPlayerController::StaticClass();
	GameStateClass = ACXGameState::StaticClass();
	PlayerStateClass = ACXPlayerState::StaticClass();
}

void ACXGameMode::InitGameState()
{
	Super::InitGameState();

	// 게임 시작 시 Waiting 상태
	if (ACXGameState* CXGameState = GetCXGameState())
	{
		CXGameState->SetMatchState(ECXMatchState::Waiting);
		CXGameState->SetRemainingTime(WaitingDuration);
	}
	else
	{
		UE_LOG(LogCX, Error, TEXT("GameStateClass is not ACXGameState. Check the GameMode Blueprint."));
	}
}

void ACXGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	++JoinedPlayerCount;

	// 채팅/결과창에서 보기 좋게 이름 지정
	if (IsValid(NewPlayer))
	{
		ChangeName(NewPlayer, FString::Printf(TEXT("Player%d"), JoinedPlayerCount), false);
	}

	// 첫 플레이어 접속 시 대기 카운트다운 시작
	if (bWaitingCountdownStarted == false)
	{
		StartWaitingCountdown();
	}
}

ACXGameState* ACXGameMode::GetCXGameState() const
{
	return GetGameState<ACXGameState>();
}

void ACXGameMode::StartWaitingCountdown()
{
	ACXGameState* CXGameState = GetCXGameState();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	bWaitingCountdownStarted = true;

	CXGameState->SetMatchState(ECXMatchState::Waiting);
	CXGameState->SetRemainingTime(WaitingDuration);

	GetWorldTimerManager().SetTimer(MatchTimerHandle, this, &ThisClass::OnMatchTimerTick, 1.0f, true);
}

void ACXGameMode::StartPlaying()
{
	ACXGameState* CXGameState = GetCXGameState();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	CXGameState->SetMatchState(ECXMatchState::Playing);
	CXGameState->SetRemainingTime(PlayingDuration);
}

void ACXGameMode::FinishMatch()
{
	GetWorldTimerManager().ClearTimer(MatchTimerHandle);

	ACXGameState* CXGameState = GetCXGameState();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	// 우승자 먼저 설정 -> 같은 네트워크 업데이트에 상태와 함께 전송됨
	DetermineWinners();
	CXGameState->SetRemainingTime(0);
	CXGameState->SetMatchState(ECXMatchState::Finished);
}

void ACXGameMode::OnMatchTimerTick()
{
	ACXGameState* CXGameState = GetCXGameState();
	if (IsValid(CXGameState) == false)
	{
		GetWorldTimerManager().ClearTimer(MatchTimerHandle);
		return;
	}

	const int32 NewRemainingTime = CXGameState->GetRemainingTime() - 1;
	CXGameState->SetRemainingTime(NewRemainingTime);

	if (NewRemainingTime > 0)
	{
		return;
	}

	switch (CXGameState->GetMatchState())
	{
	case ECXMatchState::Waiting:
		StartPlaying();
		break;

	case ECXMatchState::Playing:
		FinishMatch();
		break;

	default:
		GetWorldTimerManager().ClearTimer(MatchTimerHandle);
		break;
	}
}

void ACXGameMode::DetermineWinners()
{
	ACXGameState* CXGameState = GetCXGameState();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	int32 MaxCoins = -1;
	TArray<ACXPlayerState*> Winners;

	for (APlayerState* PlayerState : CXGameState->PlayerArray)
	{
		ACXPlayerState* CXPlayerState = Cast<ACXPlayerState>(PlayerState);
		if (IsValid(CXPlayerState) == false)
		{
			continue;
		}

		const int32 PlayerCoins = CXPlayerState->GetCoins();
		if (PlayerCoins > MaxCoins)
		{
			MaxCoins = PlayerCoins;
			Winners.Reset();
			Winners.Add(CXPlayerState);
		}
		else if (PlayerCoins == MaxCoins)
		{
			// 최고 원화가 같으면 공동 우승
			Winners.Add(CXPlayerState);
		}
	}

	CXGameState->SetWinners(Winners, FMath::Max(MaxCoins, 0));

	for (const ACXPlayerState* Winner : Winners)
	{
		UE_LOG(LogCX, Log, TEXT("Winner: %s (%d)"), *Winner->GetPlayerName(), MaxCoins);
	}
}

AActor* ACXGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<APlayerStart*> AvailablePlayerStarts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* PlayerStart = *It;

		if (IsPlayerStartAvailable(PlayerStart))
		{
			AvailablePlayerStarts.Add(PlayerStart);
		}
	}

	// 빈 PlayerStart 가 없으면 크래시 대신 기본 로직으로 대체
	if (AvailablePlayerStarts.IsEmpty())
	{
		UE_LOG(LogCX, Warning, TEXT("There is no available Vehicle PlayerStart. Fallback to default."));
		return Super::ChoosePlayerStart_Implementation(Player);
	}

	const int32 RandomIndex = FMath::RandHelper(AvailablePlayerStarts.Num());
	return Cast<AActor>(AvailablePlayerStarts[RandomIndex]);
}

bool ACXGameMode::IsPlayerStartAvailable(const APlayerStart* PlayerStart) const
{
	if (IsValid(PlayerStart) == false || IsValid(GetWorld()) == false)
	{
		return false;
	}

	const FTransform PlayerStartTransform = PlayerStart->GetActorTransform();
	const FVector CheckLocation = PlayerStartTransform.TransformPosition(VehicleSpawnCheckOffset);
	const FQuat CheckRotation = PlayerStartTransform.GetRotation();
	const FCollisionShape CheckShape = FCollisionShape::MakeBox(VehicleSpawnCheckHalfExtent);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);

	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VehiclePlayerStartCheck), false);

	const bool bVehicleExists =
		GetWorld()->OverlapAnyTestByObjectType(
			CheckLocation,
			CheckRotation,
			ObjectQueryParams,
			CheckShape,
			QueryParams
		);

	return !bVehicleExists;
}
