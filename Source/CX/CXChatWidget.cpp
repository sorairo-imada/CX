// CXChatWidget.cpp

#include "CXChatWidget.h"
#include "CXPlayerController.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"

void UCXChatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ChatInputBox)
	{
		ChatInputBox->OnTextCommitted.AddUniqueDynamic(this, &ThisClass::OnChatTextCommitted);
	}
}

TSharedPtr<SWidget> UCXChatWidget::GetChatInputSlateWidget() const
{
	if (ChatInputBox == nullptr)
	{
		return TSharedPtr<SWidget>();
	}
	return ChatInputBox->GetCachedWidget();
}

void UCXChatWidget::FocusChatInput()
{
	if (ChatInputBox)
	{
		ChatInputBox->SetKeyboardFocus();
	}
}

void UCXChatWidget::OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	ACXPlayerController* CXPlayerController = GetOwningPlayer<ACXPlayerController>();

	// 채팅창 엔터 이벤트
	if (CommitMethod == ETextCommit::OnEnter)
	{
		const FString Message = Text.ToString().TrimStartAndEnd();
		if (Message.IsEmpty() == false && IsValid(CXPlayerController))
		{
			CXPlayerController->SendChatMessage(Message);
		}

		if (ChatInputBox)
		{
			ChatInputBox->SetText(FText::GetEmpty());
		}
	}

	// 엔터/포커스 이동/ESC 모두 게임 입력으로 복귀
	if (IsValid(CXPlayerController))
	{
		CXPlayerController->ExitChatMode();
	}
}

void UCXChatWidget::AddChatMessage(const FString& SenderName, const FString& Message)
{
	if (ChatLogBox == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	UTextBlock* NewLine = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (NewLine == nullptr)
	{
		return;
	}

	NewLine->SetText(FText::FromString(FString::Printf(TEXT("[%s] %s"), *SenderName, *Message)));
	NewLine->SetAutoWrapText(true);

	FSlateFontInfo FontInfo = NewLine->GetFont();
	FontInfo.Size = ChatFontSize;
	NewLine->SetFont(FontInfo);

	ChatLogBox->AddChild(NewLine);

	// 오래된 로그 제거
	while (ChatLogBox->GetChildrenCount() > MaxChatLines)
	{
		ChatLogBox->RemoveChildAt(0);
	}

	ChatLogBox->ScrollToEnd();
}
