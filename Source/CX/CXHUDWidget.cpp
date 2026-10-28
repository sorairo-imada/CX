// CXHUDWidget.cpp

#include "CXHUDWidget.h"
#include "CXPlayerState.h"
#include "CXGameState.h"
#include "CXPawn.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "CXHUD"

void UCXHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResultText)
	{
		ResultText->SetVisibility(ESlateVisibility::Collapsed);
	}

	TryBindDataSources();
	RefreshPlayerInfo();
	RefreshMatchInfo();
}

void UCXHUDWidget::NativeDestruct()
{
	if (BoundPlayerState.IsValid())
	{
		BoundPlayerState->OnStatChanged.Remove(PlayerStatChangedHandle);
	}
	if (BoundGameState.IsValid())
	{
		BoundGameState->OnMatchInfoChanged.Remove(MatchInfoChangedHandle);
	}

	Super::NativeDestruct();
}

void UCXHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// PlayerState / GameState 는 접속 직후 늦게 복제될 수 있으므로 바인딩될 때까지 재시도
	if (BoundPlayerState.IsValid() == false || BoundGameState.IsValid() == false)
	{
		TryBindDataSources();
	}

	RefreshVehicleInfo();
}

void UCXHUDWidget::TryBindDataSources()
{
	if (BoundPlayerState.IsValid() == false)
	{
		APlayerController* OwningPC = GetOwningPlayer();
		ACXPlayerState* CXPlayerState = IsValid(OwningPC) ? OwningPC->GetPlayerState<ACXPlayerState>() : nullptr;
		if (IsValid(CXPlayerState))
		{
			BoundPlayerState = CXPlayerState;
			PlayerStatChangedHandle = CXPlayerState->OnStatChanged.AddUObject(this, &ThisClass::RefreshPlayerInfo);
			RefreshPlayerInfo();
			RefreshMatchInfo(); // 승패 판정에 로컬 PlayerState 필요
		}
	}

	if (BoundGameState.IsValid() == false)
	{
		UWorld* World = GetWorld();
		ACXGameState* CXGameState = IsValid(World) ? World->GetGameState<ACXGameState>() : nullptr;
		if (IsValid(CXGameState))
		{
			BoundGameState = CXGameState;
			MatchInfoChangedHandle = CXGameState->OnMatchInfoChanged.AddUObject(this, &ThisClass::RefreshMatchInfo);
			RefreshMatchInfo();
		}
	}
}

void UCXHUDWidget::RefreshPlayerInfo()
{
	const ACXPlayerState* CXPlayerState = BoundPlayerState.Get();

	const int32 Coins = IsValid(CXPlayerState) ? CXPlayerState->GetCoins() : 0;
	const float Bonus = IsValid(CXPlayerState) ? CXPlayerState->GetBonusMaxSpeed() : 0.0f;
	const float Penalty = IsValid(CXPlayerState) ? CXPlayerState->GetSpeedPenalty() : 0.0f;

	if (CoinsText)
	{
		CoinsText->SetText(FText::Format(LOCTEXT("Coins", "Coins : {0} KRW"), FText::AsNumber(Coins)));
	}

	if (BonusSpeedText)
	{
		if (Penalty < 0.0f)
		{
			BonusSpeedText->SetText(FText::Format(
				LOCTEXT("BonusWithPenalty", "Bonus Max Speed : +{0} km/h  (Penalty {1})"),
				FText::AsNumber(FMath::RoundToInt(Bonus)),
				FText::AsNumber(FMath::RoundToInt(Penalty))));
		}
		else
		{
			BonusSpeedText->SetText(FText::Format(
				LOCTEXT("Bonus", "Bonus Max Speed : +{0} km/h"),
				FText::AsNumber(FMath::RoundToInt(Bonus))));
		}
	}
}

FText UCXHUDWidget::GetMatchStateText(uint8 InMatchState)
{
	switch (static_cast<ECXMatchState>(InMatchState))
	{
	case ECXMatchState::Waiting:	return LOCTEXT("Waiting", "WAITING");
	case ECXMatchState::Playing:	return LOCTEXT("Playing", "PLAYING");
	case ECXMatchState::Finished:	return LOCTEXT("Finished", "FINISHED");
	default:						return FText::GetEmpty();
	}
}

void UCXHUDWidget::RefreshMatchInfo()
{
	const ACXGameState* CXGameState = BoundGameState.Get();
	if (IsValid(CXGameState) == false)
	{
		return;
	}

	const ECXMatchState MatchState = CXGameState->GetMatchState();

	if (MatchStateText)
	{
		MatchStateText->SetText(GetMatchStateText(static_cast<uint8>(MatchState)));
	}

	if (RemainingTimeText)
	{
		const int32 Time = CXGameState->GetRemainingTime();
		RemainingTimeText->SetText(FText::Format(LOCTEXT("Time", "{0}:{1}"),
			FText::AsNumber(Time / 60),
			FText::FromString(FString::Printf(TEXT("%02d"), Time % 60))));
	}

	if (ResultText == nullptr)
	{
		return;
	}

	if (MatchState != ECXMatchState::Finished)
	{
		ResultText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// 로컬 플레이어 승패 표시
	const APlayerController* OwningPC = GetOwningPlayer();
	const APlayerState* LocalPlayerState = IsValid(OwningPC) ? OwningPC->PlayerState.Get() : nullptr;

	const bool bIsWinner = CXGameState->IsWinner(LocalPlayerState);
	const bool bIsTie = CXGameState->GetWinners().Num() > 1;

	FText Result;
	if (bIsWinner && bIsTie)
	{
		Result = FText::Format(LOCTEXT("Draw", "DRAW! Co-Winner ({0} KRW)"), FText::AsNumber(CXGameState->GetWinningCoins()));
	}
	else if (bIsWinner)
	{
		Result = FText::Format(LOCTEXT("Win", "YOU WIN! ({0} KRW)"), FText::AsNumber(CXGameState->GetWinningCoins()));
	}
	else
	{
		Result = FText::Format(LOCTEXT("Lose", "YOU LOSE... (Winner {0} KRW)"), FText::AsNumber(CXGameState->GetWinningCoins()));
	}

	ResultText->SetText(Result);
	ResultText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCXHUDWidget::RefreshVehicleInfo()
{
	const APlayerController* OwningPC = GetOwningPlayer();
	const ACXPawn* VehiclePawn = IsValid(OwningPC) ? Cast<ACXPawn>(OwningPC->GetPawn()) : nullptr;

	if (MaxSpeedText)
	{
		const float MaxSpeed = IsValid(VehiclePawn) ? VehiclePawn->GetCurrentMaxSpeed() : 0.0f;
		MaxSpeedText->SetText(FText::Format(LOCTEXT("MaxSpeed", "Max Speed : {0} km/h"), FText::AsNumber(FMath::RoundToInt(MaxSpeed))));
	}

	if (StatusText)
	{
		const bool bBroken = IsValid(VehiclePawn) && VehiclePawn->IsAcceleratorBroken();
		StatusText->SetText(bBroken ? LOCTEXT("Broken", "ACCELERATOR BROKEN!") : FText::GetEmpty());
	}
}

#undef LOCTEXT_NAMESPACE
