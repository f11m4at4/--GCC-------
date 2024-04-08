// Fill out your copyright notice in the Description page of Project Settings.


#include "NetPlayerAnimInstance.h"
#include <NetTPSCharacter.h>

void UNetPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	// NetTPSCharacter �� �ν��Ͻ��� �ʱ�ȭ
	player = Cast<ANetTPSCharacter>(TryGetPawnOwner());
}

void UNetPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (player)
	{
		// speed �� ���ϱ�
		speed = FVector::DotProduct(player->GetVelocity(), player->GetActorForwardVector());
		// direction �� ���ϱ�
		direction = FVector::DotProduct(player->GetVelocity(), player->GetActorRightVector());

		// ȸ���� ����
		pitchAngle = -player->GetBaseAimRotation().GetNormalized().Pitch;
		pitchAngle = FMathf::Clamp(pitchAngle, -60, 60);

		bHasPistol = player->bHasPistol;
		// ���� ���� ����
		isDead = player->isDead;


	}
}

void UNetPlayerAnimInstance::PlayFireAnimation()
{
	// ���� ���� �ְ�, ����� ��Ÿ�� ���� �� 
	if (bHasPistol && fireMontage)
	{
		// �ִϸ��̼� ���
		Montage_Play(fireMontage, 2);
	}
}

void UNetPlayerAnimInstance::PlayReloadAnimation()
{
	// ���� �����ϰ� ������, ���ε��Ÿ�ְ� ���� ��
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
