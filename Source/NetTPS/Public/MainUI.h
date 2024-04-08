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

	// 크로스헤어 보일지여부 처리
	void ShowCrosshair(bool isShow);

public:
	// 총알 위젯이 추가될 패널
	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (BindWidget))
	class UUniformGridPanel* BulletPanel;
	// 총알 위젯클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Bullet")
	TSubclassOf<class UUserWidget> bulletUIFactory;

	// 총알위젯 추가하는 함수
	void AddBullet();
	// 총알제거 함수
	void PopBullet(int32 index);

	// 모든 총알UI 제거
	void RemoveAllAmmo();

public:
	// 체력
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="HP")
	float hp = 1.0f;

public:
	// DamageUI 애니메이션
	UPROPERTY(EditDefaultsOnly, meta=(BindWidgetAnim), Transient, Category="MySettings")
	class UWidgetAnimation* DamageAnim;
	// 피격처리 애니메이션 재생 
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

// ---------------- 채팅 ----------------------
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
