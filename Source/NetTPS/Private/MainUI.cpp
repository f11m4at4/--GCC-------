// Fill out your copyright notice in the Description page of Project Settings.


#include "MainUI.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include <Components/UniformGridPanel.h>
#include <Components/HorizontalBox.h>
#include <NetPlayerController.h>
#include <Components/EditableText.h>
#include <NetTPSCharacter.h>
#include <Components/ScrollBox.h>
#include <ChatWidget.h>
#include <Components/TextBlock.h>

void UMainUI::ShowCrosshair(bool isShow)
{
	if (isShow)
	{
		img_crosshair->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		img_crosshair->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UMainUI::AddBullet()
{
	auto bulletWidget = CreateWidget(GetWorld(), bulletUIFactory);
	BulletPanel->AddChildToUniformGrid(bulletWidget, 0, BulletPanel->GetChildrenCount());
}

void UMainUI::PopBullet(int32 index)
{
	BulletPanel->RemoveChildAt(index);
}

void UMainUI::RemoveAllAmmo()
{
	BulletPanel->ClearChildren();
}

void UMainUI::PlayDamageAnimation()
{
	PlayAnimation(DamageAnim);
}

void UMainUI::NativeConstruct()
{
	Super::NativeConstruct();

	// retry ��ư�� ó���Լ� ��������ֱ�
	btn_retry->OnClicked.AddDynamic(this, &UMainUI::OnRetry);
	btn_send->OnClicked.AddDynamic(this, &UMainUI::SendMsg);
}

void UMainUI::OnRetry()
{
	// �������� UI �� �Ⱥ��̵��� ó��
	GameoverUI->SetVisibility(ESlateVisibility::Hidden);
	// ������ ������ ��û
	auto pc = Cast<ANetPlayerController>(GetWorld()->GetFirstPlayerController());
	if (pc)
	{
		pc->SetShowMouseCursor(false);
		pc->ServerRPC_RespawnPlayer();
	}
}

void UMainUI::SendMsg()
{
	// ����ڰ� �Է��� �޽����� ������ �����ϰ�ʹ�.
	// �Է��� �޽���
	FString msg = edit_input->GetText().ToString();
	edit_input->SetText(FText::GetEmpty());
	// msg �� ���� �޽����� ������쿡�� ������ ��������
	if (msg.IsEmpty() == false)
	{
		// PlayerController �� �ʿ��ϴ�.
		auto pc = Cast<ANetPlayerController>(GetWorld()->GetFirstPlayerController());
		auto player = Cast<ANetTPSCharacter>(pc->GetPawn());
		player->ServerRPC_SendMsg(msg);
	}
}

void UMainUI::ReceiveMsg(const FString& msg)
{
	// Client ���� ����ȴ�.
	auto msgWidget = CreateWidget<UChatWidget>(GetWorld(), chatWidget);
	msgWidget->txt_msg->SetText(FText::FromString(msg));
	scroll_msgList->AddChild(msgWidget);
	scroll_msgList->ScrollToEnd();
}
