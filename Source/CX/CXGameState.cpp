// CXGameState.cpp

#include "CXGameState.h"
#include "CX.h"
#include "CXPlayerState.h"
#include "CXPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

ACXGameState::ACXGameState()
{
}

void ACXGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, MatchState);
	DOREPLIFETIME(ThisClass, RemainingTime);
	DOREPLIFETIME(ThisClass, Winners);
	DOREPLIFETIME(ThisClass, WinningCoins);
}

void ACXGameState::SetMatchState(ECXMatchState NewState)
{
	if (HasAuthority() == false)
	{
		return;
	}

	MatchState = NewState;
	ForceNetUpdate();
	OnRep_MatchState(); // 리슨 서버 호스트용
}

void ACXGameState::SetRemainingTime(int32 NewRemainingTime)
{
	if (HasAuthority() == false)
	{
		return;
	}

	RemainingTime = FMath::Max(NewRemainingTime, 0);
	ForceNetUpdate();
	OnRep_RemainingTime();
}

void ACXGameState::SetWinners(const TArray<ACXPlayerState*>& NewWinners, int32 InWinningCoins)
{
	if (HasAuthority() == false)
	{
		return;
	}

	Winners.Reset();
	for (ACXPlayerState* Winner : NewWinners)
	{
		Winners.Add(Winner);
	}
	WinningCoins = InWinningCoins;

	ForceNetUpdate();
	OnRep_Winners();
}

bool ACXGameState::IsWinner(const APlayerState* InPlayerState) const
{
	if (IsValid(InPlayerState) == false)
	{
		return false;
	}

	for (const TObjectPtr<ACXPlayerState>& Winner : Winners)
	{
		if (Winner == InPlayerState)
		{
			return true;
		}
	}
	return false;
}

bool ACXGameState::IsMatchPlaying(const UWorld* World)
{
	if (IsValid(World) == false)
	{
		return false;
	}

	const ACXGameState* CXGameState = World->GetGameState<ACXGameState>();
	if (IsValid(CXGameState) == false)
	{
		// GameState 가 아직 복제되지 않았으면 잠금(false),
		// CXGameState 가 아닌 GameState 로 설정된 경우엔 게임이 막히지 않도록 true
		return World->GetGameState() != nullptr;
	}

	return CXGameState->IsPlaying();
}

void ACXGameState::MulticastBroadcastChatMessage_Implementation(const FString& SenderName, const FString& Message)
{
	UWorld* World = GetWorld();
	if (IsValid(World) == false)
	{
		return;
	}

	// 이 머신의 로컬 플레이어 컨트롤러에만 UI 출력 (데디 서버는 로컬 PC 없음)
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ACXPlayerController* PC = Cast<ACXPlayerController>(It->Get());
		if (IsValid(PC) && PC->IsLocalController())
		{
			PC->ReceiveChatMessage(SenderName, Message);
		}
	}
}

void ACXGameState::OnRep_MatchState()
{
	UE_LOG(LogCX, Log, TEXT("MatchState -> %d"), static_cast<int32>(MatchState));
	OnMatchInfoChanged.Broadcast();
}

void ACXGameState::OnRep_RemainingTime()
{
	OnMatchInfoChanged.Broadcast();
}

void ACXGameState::OnRep_Winners()
{
	OnMatchInfoChanged.Broadcast();
}
