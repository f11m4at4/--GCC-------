// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetTPSCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include <Kismet/GameplayStatics.h>
#include <NetPlayerAnimInstance.h>
#include "MainUI.h"
#include <Components/WidgetComponent.h>
#include <HealthBar.h>
#include <NetTPS.h>
#include <Net/UnrealNetwork.h>
#include <Components/HorizontalBox.h>
#include "NetPlayerController.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ANetTPSCharacter

ANetTPSCharacter::ANetTPSCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bUseControllerDesiredRotation = true; // Character moves in the direction of input...	
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetRelativeLocation(FVector(0, 40, 60));
	CameraBoom->TargetArmLength = 150.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	
	// 총컴포넌트 추가해주기
	gunComp = CreateDefaultSubobject<USceneComponent>(TEXT("GunComp"));
	gunComp->SetupAttachment(GetMesh(), TEXT("GunPosition"));
	gunComp->SetRelativeLocation(FVector(-15.756924, 2.778371, 4.000000));
	gunComp->SetRelativeRotation(FRotator(10.000000, 79.999999, 0.000000));

	// health bar component
	hpUIComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBar"));
	hpUIComp->SetupAttachment(GetMesh());
}

void ANetTPSCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// 총검색
	TArray<AActor*> allActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), allActors);
	for (auto tempPistol : allActors)
	{
		// 총만 추려내고 싶다.
		if (tempPistol->GetName().Contains("BP_Pistol"))
		{
			pistolActors.Add(tempPistol);
		}
	}

	// Client 에서 메인 캐릭터 일때 호출됐으면 좋겠다.
	if (IsLocallyControlled() && HasAuthority() == false)
	{
		InitUIWidget();
	}
}

void ANetTPSCharacter::InitUIWidget()
{
	PRINTLOG(TEXT("[%s] Begin"), Controller ? TEXT("PLAYER") : TEXT("No Player"));
	// Player 가 제어중이 아니라면 처리하지 않는다.
	auto pc = Cast<ANetPlayerController>(Controller);
	if (pc == nullptr)
	{
		return;
	}

	if (pc->mainUIWidget)
	{
		if (pc->mainUI == nullptr)
		{
			pc->mainUI = Cast<UMainUI>(CreateWidget(GetWorld(), pc->mainUIWidget));
		}
		mainUI = pc->mainUI;
		mainUI->AddToViewport();
		mainUI->ShowCrosshair(false);

		// 체력초기화
		hp = MaxHP;
		mainUI->hp = 1.0f;
		// 총알 모두 제거
		mainUI->RemoveAllAmmo();
		
		// 총알 초기화
		bulletCount = MaxBulletCount;
		for (int i=0;i<bulletCount;i++)
		{
			mainUI->AddBullet();
		}

		// 내 머리위에있는 체력바는 안보이게 처리하자
		if (hpUIComp)
		{
			hpUIComp->SetVisibility(false);
		}
	}
}

// 서버에서만 실행이 되는 함수
void ANetTPSCharacter::PossessedBy(AController* NewController)
{
	PRINTLOG(TEXT("Begin"));
	Super::PossessedBy(NewController);

	if (IsLocallyControlled())
	{
		InitUIWidget();
	}
	PRINTLOG(TEXT("End"));
}


void ANetTPSCharacter::ServerRPC_SendMsg_Implementation(const FString& msg)
{
	// Server
	MulticastRPC_SendMsg(msg);
}

void ANetTPSCharacter::MulticastRPC_SendMsg_Implementation(const FString& msg)
{
	// Client
	// MainUI 를 가져온다.
	auto pc = Cast<ANetPlayerController>(GetWorld()->GetFirstPlayerController());
	if (pc)
	{
		// MainUI 에 있는 ReceiveMsg 함수를 호출해주자.
		pc->mainUI->ReceiveMsg(msg);
	}
}

// 총잡기 함수
void ANetTPSCharacter::TakePistol(const FInputActionValue& Value)
{
	// 총을 소유하고 있지 않다면 일정범위 안에 있는 총을 잡고싶다.
	// 필요속성 : 총소유여부, 소유중인 총, 총 검색 범위

	// 1. 총을 소유하고 있다면?
	if (bHasPistol)
	{
		//  -> 총잡기를 수행하면 안된다.
		return;
	}
	
	ServerRPC_TakePistol();
}


void ANetTPSCharacter::ServerRPC_TakePistol_Implementation()
{
	// 2. 월드에 있는 모든 총을 조사한다.
	for (auto pistolActor : pistolActors)
	{
		// 3. 다른 사용자가 이미 총을 소유하고 있다면?
		if (pistolActor->GetOwner() != nullptr)
		{
			// -> 검사할 필요가 없다.
			continue;
		}

		// 4. 총과 나와의 거리를 구한다.
		float distance = FVector::Dist(GetActorLocation(), pistolActor->GetActorLocation());
		// 5. 만약 거리가 범위안에 들어오지 않았다면
		if (distance > distanceToGun)
		{
			// 총잡기 수행할 필요가 없다.
			continue;
		}

		// 6. 소유중인 총을 등록
		ownedPistol = pistolActor;
		// 7. 총의 소유자를 자신으로 등록
		ownedPistol->SetOwner(this);
		// 8. 총 소유 상태로 변경
		bHasPistol = true;

		MulticastRPC_TakePistol(pistolActor);
		break;
	}
}

void ANetTPSCharacter::MulticastRPC_TakePistol_Implementation(const AActor* pistolActor)
{
	AttachPistol(pistolActor);
}


void ANetTPSCharacter::AttachPistol(const AActor* pistolActor)
{
	// gunComp 에 총을 붙이기
	auto meshComp = pistolActor->GetComponentByClass<UStaticMeshComponent>();
	meshComp->SetSimulatePhysics(false);
	meshComp->AttachToComponent(gunComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	// 내가 직접 컨트롤 하고 있을 때 플레이어 컨트롤러가 있을 때
	// -> automousproxy 일때만
	if (IsLocallyControlled() && mainUI)
	{
		mainUI->ShowCrosshair(true);
	}
}

void ANetTPSCharacter::ReleasePistol(const FInputActionValue& Value)
{
	// 총을 잡고 있지 않다면
	if (!bHasPistol || isReloading || IsLocallyControlled() == false)
	{
		// -> 아무 처리 하지 않는다.
		return;
	}

	ServerRPC_ReleasePistol();	
}

void ANetTPSCharacter::ServerRPC_ReleasePistol_Implementation()
{
	// 총을 소유하고 있을 때
	if (ownedPistol)
	{
		MulticastRPC_ReleasePistol(ownedPistol);
		// 미소유로 설정
		bHasPistol = false;
		ownedPistol->SetOwner(nullptr);
		ownedPistol = nullptr;
	}
}

void ANetTPSCharacter::MulticastRPC_ReleasePistol_Implementation(const AActor* pistolActor)
{
	DetachPistol(pistolActor);
}

void ANetTPSCharacter::DetachPistol(const AActor* pistolActor)
{
	auto meshComp = pistolActor->GetComponentByClass<UStaticMeshComponent>();
	meshComp->SetSimulatePhysics(true);
	meshComp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);

	if (IsLocallyControlled() && mainUI)
	{
		mainUI->ShowCrosshair(false);
	}
}

void ANetTPSCharacter::Fire(const FInputActionValue& Value)
{
	// 총을 들고 있지 않다면 처리 하지 않는다.
	if (bHasPistol == false || bulletCount <= 0 || isReloading)
	{
		return;
	}

	ServerRPC_Fire();

	
}

void ANetTPSCharacter::ServerRPC_Fire_Implementation()
{
	// 총쏘기
	FHitResult hitInfo;
	FVector startPos = FollowCamera->GetComponentLocation();
	FVector endPos = startPos + FollowCamera->GetForwardVector() * 10000;
	FCollisionQueryParams param;
	param.AddIgnoredActor(this);
	bool bHit = GetWorld()->LineTraceSingleByChannel(hitInfo, startPos, endPos, ECC_Visibility, param);
	if (bHit)
	{
		// 맞은 대상이 상대방일 경우 데미지 처리
		auto otherPlayer = Cast<ANetTPSCharacter>(hitInfo.GetActor());
		if (otherPlayer)
		{
			otherPlayer->DamageProcess();
		}
	}

	// 총알제거
	bulletCount--;

	MulticastRPC_Fire(bHit, hitInfo);
}

void ANetTPSCharacter::MulticastRPC_Fire_Implementation(bool bHit, const FHitResult& hitInfo)
{
	if (bHit)
	{
		// 맞은 부위에 파트클표시
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), gunEffect, hitInfo.Location, FRotator::ZeroRotator, true);
	}

	if (mainUI)
	{
		mainUI->PopBullet(bulletCount);
	}

	// 애니메이션 재생
	auto anim = Cast<UNetPlayerAnimInstance>(GetMesh()->GetAnimInstance());
	anim->PlayFireAnimation();
}


void ANetTPSCharacter::ReloadPistol(const FInputActionValue& Value)
{
	// 총을 소지하고 있지 않으면, 재장전 중이라면
	if (!bHasPistol || isReloading)
	{
		// 처리하지 않는다.
		return;
	}

	// 재장전 애니메이션을 재생
	auto anim = Cast<UNetPlayerAnimInstance>(GetMesh()->GetAnimInstance());
	anim->PlayReloadAnimation();

	isReloading = true;
}

void ANetTPSCharacter::ServerRPC_Reload_Implementation()
{
	// 총알 개수를 초기화하고
	bulletCount = MaxBulletCount;
	ClientRPC_Reload();
}

void ANetTPSCharacter::ClientRPC_Reload_Implementation()
{
	// 기존 총알 UI 위젯들을 모두 제거
	if (mainUI)
	{
		mainUI->RemoveAllAmmo();
		// 총알 UI 다시 세팅
		for (int i = 0; i < MaxBulletCount; i++)
		{
			mainUI->AddBullet();
		}
	}

	// 재장전 완료상태 처기
	isReloading = false;
}

void ANetTPSCharacter::DieProcess()
{
	auto pc = Cast<APlayerController>(Controller);
	// 화면에 마우스가 보이스도록 처리하자
	pc->SetShowMouseCursor(true);
	// 화면을 회색처리하도록
	GetFollowCamera()->PostProcessSettings.ColorSaturation = FVector4(0, 0, 0, 1);

	// Die UI 표시
	mainUI->GameoverUI->SetVisibility(ESlateVisibility::Visible);
}

void ANetTPSCharacter::InitAmmoUI()
{
	ServerRPC_Reload();
	
}

void ANetTPSCharacter::OnRep_HP()
{
	if (HP <= 0)
	{
		isDead = true;
		// 잡고있는 총은 떨어뜨리도록 하자
		ReleasePistol(FInputActionValue());
		// 충돌체 비활성화
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// 캐릭터 이동은 못하도록 처리
		GetCharacterMovement()->DisableMovement();
	}

	// UI 에 할당할 퍼센트 계산
	float percent = hp / MaxHP;
	if (mainUI)
	{
		mainUI->hp = percent;

		// 피격처리효과 재생
		mainUI->PlayDamageAnimation();

		// 카메라셰이크 효과재생
		if (damageCameraShake)
		{
			auto pc = Cast<APlayerController>(Controller);
			pc->ClientStartCameraShake(damageCameraShake);
		}
	}
	else
	{
		auto hpUI = Cast<UHealthBar>(hpUIComp->GetWidget());
		hpUI->hp = percent;
	}
}

float ANetTPSCharacter::GetHP()
{
	return hp;
}

void ANetTPSCharacter::SetHP(float value)
{
	hp = value;
	OnRep_HP();
}

void ANetTPSCharacter::DamageProcess()
{
	// 체력을 감소시킨다.
	HP--;
	
}

void ANetTPSCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	//PrintNetLog();

	// 빌보딩처리
	if (hpUIComp && hpUIComp->GetVisibleFlag())
	{
		FVector camLoc = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->GetCameraLocation();
		FVector direction = camLoc - hpUIComp->GetComponentLocation();
		direction.Z = 0;
		hpUIComp->SetWorldRotation(direction.GetSafeNormal().ToOrientationRotator());
	}

}

void ANetTPSCharacter::PrintNetLog()
{
	const FString conStr = GetNetConnection() != nullptr ? TEXT("Valid Connection") : TEXT("Invalid Connection");
	const FString ownerName = GetOwner() != nullptr ? GetOwner()->GetName() : TEXT("No Owner");

	const FString logStr = FString::Printf(TEXT("Connection : %s\nOwner Name : %s\nLocal Role : %s\nRemote Role : %s"), *conStr, *ownerName, *LOCALROLE, *REMOTEROLE);

	DrawDebugString(GetWorld(), GetActorLocation(), logStr, nullptr, FColor::White, 0, true, 1);
}


//////////////////////////////////////////////////////////////////////////
// Input

void ANetTPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANetTPSCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANetTPSCharacter::Look);

		EnhancedInputComponent->BindAction(takePistolAction, ETriggerEvent::Started, this, &ANetTPSCharacter::TakePistol);
		EnhancedInputComponent->BindAction(releaseAction, ETriggerEvent::Started, this, &ANetTPSCharacter::ReleasePistol);
		EnhancedInputComponent->BindAction(fireAction, ETriggerEvent::Started, this, &ANetTPSCharacter::Fire);

		EnhancedInputComponent->BindAction(reloadAction, ETriggerEvent::Started, this, &ANetTPSCharacter::ReloadPistol);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ANetTPSCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ANetTPSCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ANetTPSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetTPSCharacter, bHasPistol);
	DOREPLIFETIME(ANetTPSCharacter, bulletCount);
	DOREPLIFETIME(ANetTPSCharacter, hp);
}