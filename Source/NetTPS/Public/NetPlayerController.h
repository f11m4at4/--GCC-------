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
	// ������ �Լ�
	UFUNCTION(Server, Reliable)
	void ServerRPC_RespawnPlayer();

public:
	// ����� ����Ŭ����
	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<class UMainUI> mainUIWidget;
	// mainUIWidget ���� ���� ������� �ν��Ͻ�
	UPROPERTY()
	class UMainUI* mainUI;
};
