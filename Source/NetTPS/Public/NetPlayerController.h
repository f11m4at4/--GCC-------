// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NetPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class NETTPS_API ANetPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	class ANetTPSGameMode* gm;

public:
	virtual void BeginPlay() override;

public:
	// 리스폰 함수
	UFUNCTION(Server, Reliable)
	void ServerRPC_RespawnPlayer();

public:
	// 사용할 위젯클래스
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<class UMainUI> mainUIWidget;
	// mainUIWidget 으로 부터 만들어진 인스턴스
	UPROPERTY()
	class UMainUI* mainUI;
};
