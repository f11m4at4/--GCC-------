// Fill out your copyright notice in the Description page of Project Settings.


#include "NetActor.h"
#include <NetTPS.h>
#include <NetTPSCharacter.h>
#include <EngineUtils.h>
#include <Net/UnrealNetwork.h>

// Sets default values
ANetActor::ANetActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	meshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh Comp"));
	RootComponent = meshComp;
	meshComp->SetRelativeScale3D(FVector(0.5f));

	// ��Ʈ��ũ ����ȭ �ɼ� Ȱ��ȭ
	// -> ���� ���ø����̼�
	bReplicates = true;
}

// Called when the game starts or when spawned
void ANetActor::BeginPlay()
{
	Super::BeginPlay();
	
	mat = meshComp->CreateDynamicMaterialInstance(0);

	// �����͸� ���������(�����ϴ°�) ������ �������� ó���� ��
	if (HasAuthority())
	{
		FTimerHandle handle;

		GetWorldTimerManager().SetTimer(handle, FTimerDelegate::CreateLambda([&]
			{
				FLinearColor matColor = FLinearColor(FMath::RandRange(0.0f, 0.3f),
				FMath::RandRange(0.0f, 0.3f),
				FMath::RandRange(0.0f, 0.3f), 1);

				//OnRep_ChangeMatColor();
				ServerRPC_ChangeColor(matColor);
			}), 1, true);
	}
}

void ANetActor::OnRep_ChangeMatColor()
{
	if (mat)
	{
		mat->SetVectorParameterValue(TEXT("FloorColor"), matColor);
	}
}


void ANetActor::ServerRPC_ChangeColor_Implementation(const FLinearColor newColor)
{
	ClientRPC_ChangeColor(newColor);
}

void ANetActor::ClientRPC_ChangeColor_Implementation(const FLinearColor newColor)
{
	if (mat)
	{
		mat->SetVectorParameterValue(TEXT("FloorColor"), newColor);
	}
}

// Called every frame
void ANetActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	PrintNetLog();

	FindOwner();

	// �����ϰ��
	if (HasAuthority())
	{
		// ȸ�� ó�� 
		AddActorLocalRotation(FRotator(0, 50 * DeltaTime, 0));
		// ȸ�� �������͸� �������
		rotYaw = GetActorRotation().Yaw;
	}
}

void ANetActor::OnRep_RotYaw()
{
	// �����κ��� ���� ȸ�������� ȸ������ȭ ó���ϱ�
	FRotator newRot = GetActorRotation();
	newRot.Yaw = rotYaw;
	SetActorRotation(newRot);
}

void ANetActor::PrintNetLog()
{
	const FString conStr = GetNetConnection() != nullptr ? TEXT("Valid Connection") : TEXT("Invalid Connection");
	const FString ownerName = GetOwner() != nullptr ? GetOwner()->GetName() : TEXT("No Owner");

	const FString logStr = FString::Printf(TEXT("Connection : %s\nOwner Name : %s\nLocal Role : %s\nRemote Role : %s"), *conStr, *ownerName, *LOCALROLE, *REMOTEROLE);

	DrawDebugString(GetWorld(), GetActorLocation(), logStr, nullptr, FColor::White, 0, true, 1);
}

void ANetActor::FindOwner()
{
	// ���������� ó��
	if (HasAuthority())
	{
		// ���尡��� Pawn �� Owner �� ����, �� �����ȿ� ������ ��
		AActor* newOwner = nullptr;
		float minDist = searchDistance;

		for (TActorIterator<ANetTPSCharacter> it(GetWorld()); it; ++it)
		{
			AActor* otherActor = *it;
			float dist = GetDistanceTo(otherActor);

			if (dist < minDist)
			{
				minDist = dist;
				newOwner = otherActor;
			}
		}
		// Owner ����
		if (GetOwner() != newOwner)
		{
			SetOwner(newOwner);
		}
	}

	// ���� �ð�ȭ
	DrawDebugSphere(GetWorld(), GetActorLocation(), searchDistance, 30, FColor::Yellow, false, 0, 0, 1);
}

void ANetActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetActor, rotYaw);
	DOREPLIFETIME(ANetActor, matColor);
}