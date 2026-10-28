// CXPlayerState.cpp

#include "CXPlayerState.h"
#include "CX.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

ACXPlayerState::ACXPlayerState()
{
	// PlayerState 기본 NetUpdateFrequency 가 낮으므로 값 변경 시 ForceNetUpdate() 로 즉시 전송한다.
}

void ACXPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, Coins, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ThisClass, BonusMaxSpeed, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(ThisClass, SpeedPenalty, COND_OwnerOnly);
}

bool ACXPlayerState::CheckAuthority(const TCHAR* FunctionName) const
{
	if (HasAuthority() == false)
	{
		UE_LOG(LogCX, Warning, TEXT("[%s] %s must be called on server only."), *GetNameSafe(this), FunctionName);
		return false;
	}
	return true;
}

// ---------------- Coins ----------------

void ACXPlayerState::AddCoins(int32 Amount)
{
	if (CheckAuthority(TEXT("AddCoins")) == false || Amount <= 0)
	{
		return;
	}

	// 오버플로 방어
	Coins = (Coins > MAX_int32 - Amount) ? MAX_int32 : Coins + Amount;

	ForceNetUpdate();
	OnRep_Coins(); // 리슨 서버 호스트는 OnRep 이 자동 호출되지 않으므로 직접 호출
}

bool ACXPlayerState::RemoveCoins(int32 Amount)
{
	if (CheckAuthority(TEXT("RemoveCoins")) == false || Amount <= 0)
	{
		return false;
	}

	// 보유 원화가 부족하면 아무 동작도 하지 않는다.
	if (Coins < Amount)
	{
		return false;
	}

	Coins -= Amount;

	ForceNetUpdate();
	OnRep_Coins();
	return true;
}

// ---------------- Bonus Max Speed ----------------

void ACXPlayerState::AddBonusMaxSpeed(float Amount)
{
	if (CheckAuthority(TEXT("AddBonusMaxSpeed")) == false || Amount <= 0.0f)
	{
		return;
	}

	BonusMaxSpeed = FMath::Clamp(BonusMaxSpeed + Amount, 0.0f, MaxBonusMaxSpeed);

	ForceNetUpdate();
	OnRep_BonusMaxSpeed();
}

// ---------------- Speed Penalty ----------------

void ACXPlayerState::ApplySpeedPenalty(float PenaltyAmount)
{
	if (CheckAuthority(TEXT("ApplySpeedPenalty")) == false || PenaltyAmount <= 0.0f)
	{
		return;
	}

	// 음수 패널티 누적 (하한 -MaxSpeedPenalty)
	SpeedPenalty = FMath::Clamp(SpeedPenalty - PenaltyAmount, -MaxSpeedPenalty, 0.0f);

	ForceNetUpdate();
	OnRep_SpeedPenalty();

	// 회복 타이머 시작 (이미 돌고 있으면 유지)
	UWorld* World = GetWorld();
	if (IsValid(World) && World->GetTimerManager().IsTimerActive(PenaltyRecoveryTimerHandle) == false)
	{
		World->GetTimerManager().SetTimer(
			PenaltyRecoveryTimerHandle,
			this,
			&ThisClass::RecoverSpeedPenalty,
			PenaltyRecoveryInterval,
			true);
	}
}

void ACXPlayerState::RecoverSpeedPenalty()
{
	// 회복값이 0을 넘어 양수가 되지 않도록 Min(…, 0) 으로 제한
	SpeedPenalty = FMath::Min(SpeedPenalty + PenaltyRecoveryPerSecond * PenaltyRecoveryInterval, 0.0f);

	if (FMath::IsNearlyZero(SpeedPenalty))
	{
		SpeedPenalty = 0.0f;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PenaltyRecoveryTimerHandle);
		}
	}

	ForceNetUpdate();
	OnRep_SpeedPenalty();
}

// ---------------- OnRep ----------------

void ACXPlayerState::OnRep_Coins()
{
	OnStatChanged.Broadcast();
}

void ACXPlayerState::OnRep_BonusMaxSpeed()
{
	OnStatChanged.Broadcast();
}

void ACXPlayerState::OnRep_SpeedPenalty()
{
	OnStatChanged.Broadcast();
}
