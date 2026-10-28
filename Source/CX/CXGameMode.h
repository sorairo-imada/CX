// CXGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CXGameMode.generated.h"

class ACXGameState;

/**
 *  게임 규칙 (서버 전용)
 *  Waiting(대기 카운트다운) -> Playing(플레이 카운트다운) -> Finished(우승자 판정)
 */
UCLASS(MinimalAPI)
class ACXGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACXGameMode();

	virtual void InitGameState() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

protected:
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

private:
	bool IsPlayerStartAvailable(const APlayerStart* PlayerStart) const;

	ACXGameState* GetCXGameState() const;

	/** 첫 플레이어 접속 시 호출 */
	void StartWaitingCountdown();

	/** 대기 카운트다운 종료 시 호출 */
	void StartPlaying();

	/** 플레이 카운트다운 종료 시 호출 */
	void FinishMatch();

	/** 1초마다 호출되는 서버 타이머 */
	void OnMatchTimerTick();

	/** 원화 비교로 우승자 선정 (동점이면 공동 우승) */
	void DetermineWinners();

private:
	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckHalfExtent = FVector(250.0, 120.0, 80.0);

	UPROPERTY(EditDefaultsOnly)
	FVector VehicleSpawnCheckOffset = FVector(0.0, 0.0, 80.0);

	/** 대기 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Match", meta = (ClampMin = "1"))
	int32 WaitingDuration = 15;

	/** 플레이 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Match", meta = (ClampMin = "1"))
	int32 PlayingDuration = 90;

	FTimerHandle MatchTimerHandle;

	bool bWaitingCountdownStarted = false;

	int32 JoinedPlayerCount = 0;
};
