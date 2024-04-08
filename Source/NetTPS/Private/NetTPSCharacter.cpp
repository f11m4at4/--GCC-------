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

	
	// ��������Ʈ �߰����ֱ�
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

	// �Ѱ˻�
	TArray<AActor*> allActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), allActors);
	for (auto tempPistol : allActors)
	{
		// �Ѹ� �߷����� �ʹ�.
		if (tempPistol->GetName().Contains("BP_Pistol"))
		{
			pistolActors.Add(tempPistol);
		}
	}

	// Client ���� ���� ĳ���� �϶� ȣ������� ���ڴ�.
	if (IsLocallyControlled() && HasAuthority() == false)
	{
		InitUIWidget();
	}
}

void ANetTPSCharacter::InitUIWidget()
{
	PRINTLOG(TEXT("[%s] Begin"), Controller ? TEXT("PLAYER") : TEXT("No Player"));
	// Player �� �������� �ƴ϶�� ó������ �ʴ´�.
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

		// ü���ʱ�ȭ
		hp = MaxHP;
		mainUI->hp = 1.0f;
		// �Ѿ� ��� ����
		mainUI->RemoveAllAmmo();
		
		// �Ѿ� �ʱ�ȭ
		bulletCount = MaxBulletCount;
		for (int i=0;i<bulletCount;i++)
		{
			mainUI->AddBullet();
		}

		// �� �Ӹ������ִ� ü�¹ٴ� �Ⱥ��̰� ó������
		if (hpUIComp)
		{
			hpUIComp->SetVisibility(false);
		}
	}
}

// ���������� ������ �Ǵ� �Լ�
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
	// MainUI �� �����´�.
	auto pc = Cast<ANetPlayerController>(GetWorld()->GetFirstPlayerController());
	if (pc)
	{
		// MainUI �� �ִ� ReceiveMsg �Լ��� ȣ��������.
		pc->mainUI->ReceiveMsg(msg);
	}
}

// ����� �Լ�
void ANetTPSCharacter::TakePistol(const FInputActionValue& Value)
{
	// ���� �����ϰ� ���� �ʴٸ� �������� �ȿ� �ִ� ���� ���ʹ�.
	// �ʿ�Ӽ� : �Ѽ�������, �������� ��, �� �˻� ����

	// 1. ���� �����ϰ� �ִٸ�?
	if (bHasPistol)
	{
		//  -> ����⸦ �����ϸ� �ȵȴ�.
		return;
	}
	
	ServerRPC_TakePistol();
}


void ANetTPSCharacter::ServerRPC_TakePistol_Implementation()
{
	// 2. ���忡 �ִ� ��� ���� �����Ѵ�.
	for (auto pistolActor : pistolActors)
	{
		// 3. �ٸ� ����ڰ� �̹� ���� �����ϰ� �ִٸ�?
		if (pistolActor->GetOwner() != nullptr)
		{
			// -> �˻��� �ʿ䰡 ����.
			continue;
		}

		// 4. �Ѱ� ������ �Ÿ��� ���Ѵ�.
		float distance = FVector::Dist(GetActorLocation(), pistolActor->GetActorLocation());
		// 5. ���� �Ÿ��� �����ȿ� ������ �ʾҴٸ�
		if (distance > distanceToGun)
		{
			// ����� ������ �ʿ䰡 ����.
			continue;
		}

		// 6. �������� ���� ���
		ownedPistol = pistolActor;
		// 7. ���� �����ڸ� �ڽ����� ���
		ownedPistol->SetOwner(this);
		// 8. �� ���� ���·� ����
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
	// gunComp �� ���� ���̱�
	auto meshComp = pistolActor->GetComponentByClass<UStaticMeshComponent>();
	meshComp->SetSimulatePhysics(false);
	meshComp->AttachToComponent(gunComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	// ���� ���� ��Ʈ�� �ϰ� ���� �� �÷��̾� ��Ʈ�ѷ��� ���� ��
	// -> automousproxy �϶���
	if (IsLocallyControlled() && mainUI)
	{
		mainUI->ShowCrosshair(true);
	}
}

void ANetTPSCharacter::ReleasePistol(const FInputActionValue& Value)
{
	// ���� ��� ���� �ʴٸ�
	if (!bHasPistol || isReloading || IsLocallyControlled() == false)
	{
		// -> �ƹ� ó�� ���� �ʴ´�.
		return;
	}

	ServerRPC_ReleasePistol();	
}

void ANetTPSCharacter::ServerRPC_ReleasePistol_Implementation()
{
	// ���� �����ϰ� ���� ��
	if (ownedPistol)
	{
		MulticastRPC_ReleasePistol(ownedPistol);
		// �̼����� ����
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
	// ���� ��� ���� �ʴٸ� ó�� ���� �ʴ´�.
	if (bHasPistol == false || bulletCount <= 0 || isReloading)
	{
		return;
	}

	ServerRPC_Fire();

	
}

void ANetTPSCharacter::ServerRPC_Fire_Implementation()
{
	// �ѽ��
	FHitResult hitInfo;
	FVector startPos = FollowCamera->GetComponentLocation();
	FVector endPos = startPos + FollowCamera->GetForwardVector() * 10000;
	FCollisionQueryParams param;
	param.AddIgnoredActor(this);
	bool bHit = GetWorld()->LineTraceSingleByChannel(hitInfo, startPos, endPos, ECC_Visibility, param);
	if (bHit)
	{
		// ���� ����� ������ ��� ������ ó��
		auto otherPlayer = Cast<ANetTPSCharacter>(hitInfo.GetActor());
		if (otherPlayer)
		{
			otherPlayer->DamageProcess();
		}
	}

	// �Ѿ�����
	bulletCount--;

	MulticastRPC_Fire(bHit, hitInfo);
}

void ANetTPSCharacter::MulticastRPC_Fire_Implementation(bool bHit, const FHitResult& hitInfo)
{
	if (bHit)
	{
		// ���� ������ ��ƮŬǥ��
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), gunEffect, hitInfo.Location, FRotator::ZeroRotator, true);
	}

	if (mainUI)
	{
		mainUI->PopBullet(bulletCount);
	}

	// �ִϸ��̼� ���
	auto anim = Cast<UNetPlayerAnimInstance>(GetMesh()->GetAnimInstance());
	anim->PlayFireAnimation();
}


void ANetTPSCharacter::ReloadPistol(const FInputActionValue& Value)
{
	// ���� �����ϰ� ���� ������, ������ ���̶��
	if (!bHasPistol || isReloading)
	{
		// ó������ �ʴ´�.
		return;
	}

	// ������ �ִϸ��̼��� ���
	auto anim = Cast<UNetPlayerAnimInstance>(GetMesh()->GetAnimInstance());
	anim->PlayReloadAnimation();

	isReloading = true;
}

void ANetTPSCharacter::ServerRPC_Reload_Implementation()
{
	// �Ѿ� ������ �ʱ�ȭ�ϰ�
	bulletCount = MaxBulletCount;
	ClientRPC_Reload();
}

void ANetTPSCharacter::ClientRPC_Reload_Implementation()
{
	// ���� �Ѿ� UI �������� ��� ����
	if (mainUI)
	{
		mainUI->RemoveAllAmmo();
		// �Ѿ� UI �ٽ� ����
		for (int i = 0; i < MaxBulletCount; i++)
		{
			mainUI->AddBullet();
		}
	}

	// ������ �Ϸ���� ó��
	isReloading = false;
}

void ANetTPSCharacter::DieProcess()
{
	auto pc = Cast<APlayerController>(Controller);
	// ȭ�鿡 ���콺�� ���̽����� ó������
	pc->SetShowMouseCursor(true);
	// ȭ���� ȸ��ó���ϵ���
	GetFollowCamera()->PostProcessSettings.ColorSaturation = FVector4(0, 0, 0, 1);

	// Die UI ǥ��
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
		// ����ִ� ���� ����߸����� ����
		ReleasePistol(FInputActionValue());
		// �浹ü ��Ȱ��ȭ
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// ĳ���� �̵��� ���ϵ��� ó��
		GetCharacterMovement()->DisableMovement();
	}

	// UI �� �Ҵ��� �ۼ�Ʈ ���
	float percent = hp / MaxHP;
	if (mainUI)
	{
		mainUI->hp = percent;

		// �ǰ�ó��ȿ�� ���
		mainUI->PlayDamageAnimation();

		// ī�޶����ũ ȿ�����
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
	// ü���� ���ҽ�Ų��.
	HP--;
	
}

void ANetTPSCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	//PrintNetLog();

	// ������ó��
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