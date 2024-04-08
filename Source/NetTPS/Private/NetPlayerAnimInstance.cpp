// Fill out your copyright notice in the Description page of Project Settings.


#include "NetPlayerAnimInstance.h"
#include <NetTPSCharacter.h>

void UNetPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// NetTPSCharacter 의 인스턴스를 초기화
	player = Cast<ANetTPSCharacter>(TryGetPawnOwner());
}

void UNetPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (player)
	{
		// speed 값 구하기
		speed = FVector::DotProduct(player->GetVelocity(), player->GetActorForwardVector());
		// direction 값 구하기
		direction = FVector::DotProduct(player->GetVelocity(), player->GetActorRightVector());

		// 회전값 적용
		pitchAngle = -player->GetBaseAimRotation().GetNormalized().Pitch;
		pitchAngle = FMathf::Clamp(pitchAngle, -60, 60);

		bHasPistol = player->bHasPistol;
		// 죽음 여부 적용
		isDead = player->isDead;


	}
}

void UNetPlayerAnimInstance::PlayFireAnimation()
{
	// 총을 갖고 있고, 재생할 몽타주 있을 때 
	if (bHasPistol && fireMontage)
	{
		// 애니메이션 재생
		Montage_Play(fireMontage, 2);
	}
}

void UNetPlayerAnimInstance::PlayReloadAnimation()
{
	// 총을 소유하고 있을때, 리로드몽타주가 있을 때
	if (bHasPistol && reloadMontage)
	{
		Montage_Play(reloadMontage);
	}
}

void UNetPlayerAnimInstance::AnimNotify_OnReloadFinish()
{
	player->InitAmmoUI();
}

void UNetPlayerAnimInstance::AnimNotify_DieEnd()
{
	if (player && player->IsLocallyControlled())
	{
		player->DieProcess();
	}
}
