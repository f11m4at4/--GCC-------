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

	// 총을 붙이기 위한 컴포넌트
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

	// 필요속성 : 총소유여부, 소유중인 총, 총 검색 범위
	UPROPERTY(Replicated)
	bool bHasPistol = false;
	// 소유중인 총
	UPROPERTY()
	AActor* ownedPistol = nullptr;
	//총 검색 범위
	UPROPERTY()
	float distanceToGun = 200;

	// 월드에 배치된 총들
	UPROPERTY()
	TArray<AActor*> pistolActors;

	void TakePistol(const FInputActionValue& Value);
	// 총을 gunComp 에 붙이는 함수
	void AttachPistol(const AActor* pistolActor);

public: // 총놓기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	UInputAction* releaseAction;
	void ReleasePistol(const FInputActionValue& Value);
	// 총을 컴포넌트에서 분리
	void DetachPistol(const AActor* pistolActor);

public: // 총쏘기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	UInputAction* fireAction;
	void Fire(const FInputActionValue& Value);

	// 효과 파티클
	UPROPERTY(EditDefaultsOnly, Category="Gun")
	class UParticleSystem* gunEffect;

public:
	// mainUIWidget 으로 부터 만들어진 인스턴스
	UPROPERTY()
	class UMainUI* mainUI;

	// UI 초기화 함수
	void InitUIWidget();

public: // 총알 멤버
	// 최대 총알개수
	UPROPERTY(EditAnywhere, Category="Bullet")
	int32 MaxBulletCount = 10;
	// 남은 총알개수
	UPROPERTY(Replicated)
	int32 bulletCount = MaxBulletCount;

public: // 재장전
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* reloadAction;
	void ReloadPistol(const FInputActionValue& Value);

	// 총알 UI 초기화 함수
	void InitAmmoUI();

	// 재장전 중인지 기억
	bool isReloading = false;

public:
	// 플레이어체력
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HP")
	float MaxHP = 3.0f;
	// 현재체력
	UPROPERTY(ReplicatedUsing=OnRep_HP, BlueprintReadOnly, Category="HP")
	float hp = MaxHP;
	UFUNCTION()
	void OnRep_HP();

	__declspec(property(get = GetHP, put = SetHP)) float HP;
	float GetHP();
	void SetHP(float value);

	UPROPERTY(VisibleAnywhere)
	class UWidgetComponent* hpUIComp;

	// 피격처리 함수
	void DamageProcess();

	// 죽음여부
	bool isDead = false;

public: // 네트워크 관련된 멤버
	virtual void Tick( float DeltaSeconds ) override;
	// 네트워크 상태로그 출력함수
	void PrintNetLog();

public:
	// 총잡기
	UFUNCTION(Server, Reliable)
	void ServerRPC_TakePistol();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_TakePistol(const AActor* pistolActor);

	// 총놓기
	UFUNCTION(Server, Reliable)
	void ServerRPC_ReleasePistol();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_ReleasePistol(const AActor* pistolActor);

public:
	// 총쏘기
	UFUNCTION(Server, Reliable)
	void ServerRPC_Fire();
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastRPC_Fire(bool bHit, const FHitResult& hitInfo);

public:
	// 재장전
	UFUNCTION(Server, Reliable)
	void ServerRPC_Reload();
	UFUNCTION(Client, Reliable)
	void ClientRPC_Reload();

public:
	// 카메라셰이크
	UPROPERTY(EditDefaultsOnly, Category="Damage")
	TSubclassOf<class UCameraShakeBase> damageCameraShake;

	// 죽음처리
	void DieProcess();

public:
	// PlayerController 로부터 Possessed 됐을 때 호출되는 함수
	virtual void PossessedBy(AController* NewController) override;

public:
	// ------------------ 채팅 ----------------
	UFUNCTION(Server, Reliable)
	void ServerRPC_SendMsg(const FString& msg);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastRPC_SendMsg(const FString& msg);
};

