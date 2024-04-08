// Fill out your copyright notice in the Description page of Project Settings.


#include "NetPlayerController.h"
#include "NetTPSGameMode.h"

void ANetPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		gm = Cast<ANetTPSGameMode>(GetWorld()->GetAuthGameMode());
	}
}


void ANetPlayerController::ServerRPC_RespawnPlayer_Implementation()
{
	// 1. ������ ������� Pawn (NetTPSCharacter) �� ����� �ص���
	auto player = GetPawn();
	// 2. UnPossess
	UnPossess();
	// 3. ���� ������� Pawn �� �޸𸮿��� ����������
	player->Destroy();
	// 4. ���Ӱ� �ٽ� spawn -> RestartPlayer
	gm->RestartPlayer(this);
}

