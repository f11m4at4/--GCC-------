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

	// 네트워크 동기화 옵션 활성화
	// -> 액터 리플리케이션
	bReplicates = true;
}

// Called when the game starts or when spawned
void ANetActor::BeginPlay()
{
	Super::BeginPlay();
	
	mat = meshComp->CreateDynamicMaterialInstance(0);

	// 데이터를 만드는행위(변경하는것) 무조건 서버에서 처리할 것
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

	// 서버일경우
	if (HasAuthority())
	{
		// 회전 처리 
		AddActorLocalRotation(FRotator(0, 50 * DeltaTime, 0));
		// 회전 값데이터를 기억하자
		rotYaw = GetActorRotation().Yaw;
	}
}

void ANetActor::OnRep_RotYaw()
{
	// 서버로부터 받은 회전값으로 회전동기화 처리하기
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
	// 서버에서만 처리
	if (HasAuthority())
	{
		// 가장가까운 Pawn 을 Owner 로 설정, 단 범위안에 들어왔을 때
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
		// Owner 설정
		if (GetOwner() != newOwner)
		{
			SetOwner(newOwner);
		}
	}

	// 영역 시각화
	DrawDebugSphere(GetWorld(), GetActorLocation(), searchDistance, 30, FColor::Yellow, false, 0, 0, 1);
}

void ANetActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetActor, rotYaw);
	DOREPLIFETIME(ANetActor, matColor);
}