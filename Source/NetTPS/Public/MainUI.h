// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainUI.generated.h"

/**
 * 
 */
UCLASS()
class NETTPS_API UMainUI : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category="UI", meta=(BindWidget))
	class UImage* img_crosshair;

	// ũ�ν���� ���������� ó��
	void ShowCrosshair(bool isShow);

public:
	// �Ѿ� ������ �߰��� �г�
	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	class UUniformGridPanel* BulletPanel;
	// �Ѿ� ����Ŭ����
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Bullet")
	TSubclassOf<class UUserWidget> bulletUIFactory;

	// �Ѿ����� �߰��ϴ� �Լ�
	void AddBullet();
	// �Ѿ����� �Լ�
	void PopBullet(int32 index);

	// ��� �Ѿ�UI ����
	void RemoveAllAmmo();

public:
	// ü��
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="HP")
	float hp = 1.0f;

public:
	// DamageUI �ִϸ��̼�
	UPROPERTY(EditDefaultsOnly, meta=(BindWidgetAnim), Transient, Category="MySettings")
	class UWidgetAnimation* DamageAnim;
	// �ǰ�ó�� �ִϸ��̼� ��� 
	void PlayDamageAnimation();

public:
	// gameover UI
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UHorizontalBox* GameoverUI;
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UButton* btn_retry;
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UButton* btn_exit;

public:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void OnRetry();

// ---------------- ä�� ----------------------
public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UChatWidget> chatWidget;
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UScrollBox* scroll_msgList;
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UEditableText* edit_input;
	UPROPERTY(BlueprintReadWrite, meta=(BindWidget))
	class UButton* btn_send;

	UFUNCTION()
	void SendMsg();

	void ReceiveMsg(const FString& msg);
};
