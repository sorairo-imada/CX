// CXGameState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "CXGameState.generated.h"

class ACXPlayerState;

/** 게임 진행 단계 */
UENUM(BlueprintType)
enum class ECXMatchState : uint8
{
	Waiting		UMETA(DisplayName = "Waiting"),
	Playing		UMETA(DisplayName = "Playing"),
	Finished	UMETA(DisplayName = "Finished")
};

/** 상태/남은 시간/우승자 변경 시 UI 갱신용 */
DECLARE_MULTICAST_DELEGATE(FOnCXMatchInfoChanged);

/**
 *  모든 플레이어가 공유하는 게임 정보
 *  - 현재 게임 상태, 남은 시간, 우승자 목록
 *  - 값 변경은 GameMode(서버)만 한다.
 */
UCLASS()
class CX_API ACXGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ACXGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ---------- Server only setters ----------
	void SetMatchState(ECXMatchState NewState);
	void SetRemainingTime(int32 NewRemainingTime);
	void SetWinners(const TArray<ACXPlayerState*>& NewWinners, int32 InWinningCoins);

	// ---------- Getters ----------
	ECXMatchState GetMatchState() const { return MatchState; }
	int32 GetRemainingTime() const { return RemainingTime; }
	const TArray<TObjectPtr<ACXPlayerState>>& GetWinners() const { return Winners; }
	int32 GetWinningCoins() const { return WinningCoins; }

	bool IsPlaying() const { return MatchState == ECXMatchState::Playing; }
	bool IsWinner(const APlayerState* InPlayerState) const;

	/**
	 *  월드의 GameState 가 Playing 인지 검사하는 헬퍼.
	 *  CXGameState 가 아닌 GameState 를 쓰는 경우(설정 누락)에는 게임이 막히지 않도록 true 반환.
	 */
	static bool IsMatchPlaying(const UWorld* World);

	// ---------- Chat ----------
	/** [Server -> All] 채팅 메시지를 모든 클라이언트에 전달 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastBroadcastChatMessage(const FString& SenderName, const FString& Message);

	FOnCXMatchInfoChanged OnMatchInfoChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_MatchState, VisibleInstanceOnly, BlueprintReadOnly, Category = "CX|Match")
	ECXMatchState MatchState = ECXMatchState::Waiting;

	UPROPERTY(ReplicatedUsing = OnRep_RemainingTime, VisibleInstanceOnly, BlueprintReadOnly, Category = "CX|Match")
	int32 RemainingTime = 0;

	/** 우승자 목록 (공동 우승 가능) */
	UPROPERTY(ReplicatedUsing = OnRep_Winners, VisibleInstanceOnly, BlueprintReadOnly, Category = "CX|Match")
	TArray<TObjectPtr<ACXPlayerState>> Winners;

	/** 우승 원화 (Coins 가 OwnerOnly 복제라 결과 표시용으로 따로 복제) */
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "CX|Match")
	int32 WinningCoins = 0;

	UFUNCTION()
	void OnRep_MatchState();

	UFUNCTION()
	void OnRep_RemainingTime();

	UFUNCTION()
	void OnRep_Winners();
};
