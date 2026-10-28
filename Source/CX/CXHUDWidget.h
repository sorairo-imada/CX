// CXHUDWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CXHUDWidget.generated.h"

class UTextBlock;
class ACXPlayerState;
class ACXGameState;

/**
 *  게임 HUD
 *  - 내 원화 / 추가 최대 속도 (PlayerState, OwnerOnly 복제값)
 *  - 게임 상태 / 남은 시간 / 승패 결과 (GameState)
 *  위젯 블루프린트에서 아래 이름과 똑같은 TextBlock 을 만들어야 한다. (BindWidget)
 */
UCLASS(Abstract)
class CX_API UCXHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** PlayerState / GameState 가 복제되어 들어오면 델리게이트 바인딩 */
	void TryBindDataSources();

	void RefreshPlayerInfo();
	void RefreshMatchInfo();
	void RefreshVehicleInfo();

	static FText GetMatchStateText(uint8 InMatchState);

protected:
	// ---------- 필수 위젯 (이름 그대로 만들기) ----------
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CoinsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BonusSpeedText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MatchStateText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> RemainingTimeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ResultText;

	// ---------- 선택 위젯 ----------
	/** 현재 최대 속도 표시 (선택) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MaxSpeedText;

	/** 가속 페달 고장 등 상태 메시지 (선택) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

private:
	TWeakObjectPtr<ACXPlayerState> BoundPlayerState;
	TWeakObjectPtr<ACXGameState> BoundGameState;

	FDelegateHandle PlayerStatChangedHandle;
	FDelegateHandle MatchInfoChangedHandle;
};
