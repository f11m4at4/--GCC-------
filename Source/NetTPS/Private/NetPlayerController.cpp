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
	// 1. 이전에 사용중인 Pawn (NetTPSCharacter) 은 기억을 해두자
	auto player = GetPawn();
	// 2. UnPossess
	UnPossess();
	// 3. 이전 사용중인 Pawn 을 메모리에서 제거해주자
	player->Destroy();
	// 4. 새롭게 다시 spawn -> RestartPlayer
	gm->RestartPlayer(this);
}

