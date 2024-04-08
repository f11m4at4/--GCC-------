// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "NetTPSCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ANetTPSCharacter : public ACharacter
{
	GENERATED_BODY()

	// ���� ���̱� ���� ������Ʈ
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* gunComp;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

public:
	ANetTPSCharacter();
	

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
			

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	UInputAction* takePistolAction;

	// �ʿ�Ӽ� : �Ѽ�������, �������� ��, �� �˻� ����
	UPROPERTY(Replicated)
	bool bHasPistol = false;
	// �������� ��
	UPROPERTY()
	AActor* ownedPistol = nullptr;
	//�� �˻� ����
	UPROPERTY()
	float distanceToGun = 200;

	// ���忡 ��ġ�� �ѵ�
	UPROPERTY()
	TArray<AActor*> pistolActors;

	void TakePistol(const FInputActionValue& Value);
	// ���� gunComp �� ���̴� �Լ�
	void AttachPistol(const AActor* pistolActor);

public: // �ѳ���
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	UInputAction* releaseAction;
	void ReleasePistol(const FInputActionValue& Value);
	// ���� ������Ʈ���� �и�
	void DetachPistol(const AActor* pistolActor);

public: // �ѽ��
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	UInputAction* fireAction;
	void Fire(const FInputActionValue& Value);

	// ȿ�� ��ƼŬ
	UPROPERTY(EditDefaultsOnly, Category="Gun")
	class UParticleSystem* gunEffect;

public:
	// mainUIWidget ���� ���� ������� �ν��Ͻ�
	UPROPERTY()
	class UMainUI* mainUI;

	// UI �ʱ�ȭ �Լ�
	void InitUIWidget();

public: // �Ѿ� ���
	// �ִ� �Ѿ˰���
	UPROPERTY(EditAnywhere, Category="Bullet")
	int32 MaxBulletCount = 10;
	// ���� �Ѿ˰���
	UPROPERTY(Replicated)
	int32 bulletCount = MaxBulletCount;

public: // ������
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* reloadAction;
	void ReloadPistol(const FInputActionValue& Value);

	// �Ѿ� UI �ʱ�ȭ �Լ�
	void InitAmmoUI();

	// ������ ������ ���
	bool isReloading = false;

public:
	// �÷��̾�ü��
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HP")
	float MaxHP = 3.0f;
	// ����ü��
	UPROPERTY(ReplicatedUsing=OnRep_HP, BlueprintReadOnly, Category="HP")
	float hp = MaxHP;
	UFUNCTION()
	void OnRep_HP();

	__declspec(property(get = GetHP, put = SetHP)) float HP;
	float GetHP();
	void SetHP(float value);

	UPROPERTY(VisibleAnywhere)
	class UWidgetComponent* hpUIComp;

	// �ǰ�ó�� �Լ�
	void DamageProcess();

	// ��������
	bool isDead = false;

public: // ��Ʈ��ũ ���õ� ���
	virtual void Tick( float DeltaSeconds ) override;
	// ��Ʈ��ũ ���·α� ����Լ�
	void PrintNetLog();

public:
	// �����
	UFUNCTION(Server, Reliable)
	void ServerRPC_TakePistol();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_TakePistol(const AActor* pistolActor);

	// �ѳ���
	UFUNCTION(Server, Reliable)
	void ServerRPC_ReleasePistol();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_ReleasePistol(const AActor* pistolActor);

public:
	// �ѽ��
	UFUNCTION(Server, Reliable)
	void ServerRPC_Fire();
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastRPC_Fire(bool bHit, const FHitResult& hitInfo);

public:
	// ������
	UFUNCTION(Server, Reliable)
	void ServerRPC_Reload();
	UFUNCTION(Client, Reliable)
	void ClientRPC_Reload();

public:
	// ī�޶����ũ
	UPROPERTY(EditDefaultsOnly, Category="Damage")
	TSubclassOf<class UCameraShakeBase> damageCameraShake;

	// ����ó��
	void DieProcess();

public:
	// PlayerController �κ��� Possessed ���� �� ȣ��Ǵ� �Լ�
	virtual void PossessedBy(AController* NewController) override;

public:
	// ------------------ ä�� ----------------
	UFUNCTION(Server, Reliable)
	void ServerRPC_SendMsg(const FString& msg);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_SendMsg(const FString& msg);
};

