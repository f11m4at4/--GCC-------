// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NetPlayerAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class NETTPS_API UNetPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	// ���� �����ϰ� �ִ��� ����
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MyAnimSettings")
	bool bHasPistol = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MyAnimSettings")
	float direction;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MyAnimSettings")
	float speed;

	UPROPERTY()
	class ANetTPSCharacter* player;

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:
	// �ѽ�⿡�� ����� ��Ÿ��
	UPROPERTY(EditDefaultsOnly, Category="Anim")
	class UAnimMontage* fireMontage;
	// �ѽ�� �ִϸ��̼� ����Լ�
	void PlayFireAnimation();

	// ȸ���� ��ﺯ��
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MyAnimSettings")
	float pitchAngle;

public: // ������
	// ������ ��Ÿ
	UPROPERTY(EditDefaultsOnly, Category="Anim")
	class UAnimMontage* reloadMontage;
	// ������ �ִϸ��̼� ����ϴ� �Լ�
	void PlayReloadAnimation();

	// ������ �ִϸ��̼� ��Ƽ�����̺�Ʈ
	UFUNCTION()
	void AnimNotify_OnReloadFinish();

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MyAnimSettings")
	bool isDead = false;

	UFUNCTION()
	void AnimNotify_DieEnd();
};
