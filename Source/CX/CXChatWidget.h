// CXChatWidget.h

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "CXChatWidget.generated.h"

class UEditableTextBox;
class UScrollBox;

/**
 *  채팅창 위젯
 *  - Enter : 입력창 포커스 (PlayerController 가 처리)
 *  - 입력 후 Enter : 서버로 전송 → 서버가 GameState Multicast 로 모두에게 전달
 *  위젯 블루프린트에서 아래 이름과 똑같은 위젯을 만들어야 한다. (BindWidget)
 */
UCLASS(Abstract)
class CX_API UCXChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 채팅 로그에 한 줄 추가 */
	void AddChatMessage(const FString& SenderName, const FString& Message);

	/** 입력창 슬레이트 위젯 (InputMode 포커스 대상) */
	TSharedPtr<SWidget> GetChatInputSlateWidget() const;

	void FocusChatInput();

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> ChatInputBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ChatLogBox;

	/** 로그 최대 줄 수 */
	UPROPERTY(EditAnywhere, Category = "CX|Chat")
	int32 MaxChatLines = 50;

	UPROPERTY(EditAnywhere, Category = "CX|Chat")
	int32 ChatFontSize = 14;
};
