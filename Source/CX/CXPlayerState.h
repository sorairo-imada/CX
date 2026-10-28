// CXPlayerState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "CXPlayerState.generated.h"

/** 원화/추가 최대 속도/패널티 값이 바뀌었을 때 UI 갱신용 (로컬 전용 델리게이트) */
DECLARE_MULTICAST_DELEGATE(FOnCXPlayerStatChanged);

/**
 *  플레이어 개별 데이터 (원화, 추가 최대 속도, 바퀴 피격 패널티)
 *  - 모든 값은 서버에서만 변경된다. (HasAuthority 검사)
 *  - 소유 클라이언트에게만 복제된다. (COND_OwnerOnly)
 */
UCLASS()
class CX_API ACXPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ACXPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ---------- Coins ----------
	/** [Server] 원화 증가 */
	void AddCoins(int32 Amount);

	/** [Server] 원화 차감. 보유량이 Amount 미만이면 아무것도 하지 않고 false 반환 */
	bool RemoveCoins(int32 Amount);

	int32 GetCoins() const { return Coins; }

	// ---------- Bonus Max Speed ----------
	/** [Server] 추가 최대 속도 증가 (km/h) */
	void AddBonusMaxSpeed(float Amount);

	float GetBonusMaxSpeed() const { return BonusMaxSpeed; }

	// ---------- Speed Penalty (도전: 바퀴 피격) ----------
	/** [Server] 음수 속도 패널티 적용. PenaltyAmount 는 양수로 넘기면 내부에서 음수로 누적된다 */
	void ApplySpeedPenalty(float PenaltyAmount);

	/** 항상 0 이하 */
	float GetSpeedPenalty() const { return SpeedPenalty; }

	/** 실제 차량에 적용되는 추가 최대 속도 = 버프 + 패널티 */
	float GetEffectiveBonusMaxSpeed() const { return BonusMaxSpeed + SpeedPenalty; }

	/** UI 갱신용 */
	FOnCXPlayerStatChanged OnStatChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Coins, VisibleInstanceOnly, BlueprintReadOnly, Category = "CX|Stat")
	int32 Coins = 0;

	UPROPERTY(ReplicatedUsing = OnRep_BonusMaxSpeed, VisibleInstanceOnly, BlueprintReadOnly, Category = "CX|Stat")
	float BonusMaxSpeed = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_SpeedPenalty, VisibleInstanceOnly, BlueprintReadOnly, Category = "CX|Stat")
	float SpeedPenalty = 0.0f;

	/** 추가 최대 속도 상한 */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Stat")
	float MaxBonusMaxSpeed = 100.0f;

	/** 패널티 하한 (이 값보다 더 내려가지 않음, 양수로 입력) */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Stat")
	float MaxSpeedPenalty = 60.0f;

	/** 초당 패널티 회복량 */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Stat")
	float PenaltyRecoveryPerSecond = 10.0f;

	/** 패널티 회복 타이머 간격 */
	UPROPERTY(EditDefaultsOnly, Category = "CX|Stat")
	float PenaltyRecoveryInterval = 0.2f;

	UFUNCTION()
	void OnRep_Coins();

	UFUNCTION()
	void OnRep_BonusMaxSpeed();

	UFUNCTION()
	void OnRep_SpeedPenalty();

private:
	/** 서버에서만 값 변경 허용 */
	bool CheckAuthority(const TCHAR* FunctionName) const;

	/** [Server] 타이머로 패널티를 0까지 회복 */
	void RecoverSpeedPenalty();

	FTimerHandle PenaltyRecoveryTimerHandle;
};
