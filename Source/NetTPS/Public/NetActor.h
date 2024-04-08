// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetActor.generated.h"

UCLASS()
class NETTPS_API ANetActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANetActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* meshComp;

	void PrintNetLog();

	// Owner ���� ����
	UPROPERTY(EditAnywhere)
	float searchDistance = 200;

	void FindOwner();

public:
	// ȸ�� �� ����ȭ ����
	//UPROPERTY(Replicated)
	UPROPERTY(ReplicatedUsing=OnRep_RotYaw)
	float rotYaw = 0;

	UFUNCTION()
	void OnRep_RotYaw();

public:
	// ���� �Ӽ���
	UPROPERTY()
	class UMaterialInstanceDynamic* mat;
	// ������ ����ȭ�� ����
	UPROPERTY(ReplicatedUsing=OnRep_ChangeMatColor)
	FLinearColor matColor;
	UFUNCTION()
	void OnRep_ChangeMatColor();

public:// RPC
	UFUNCTION(Server, Reliable)
	void ServerRPC_ChangeColor(const FLinearColor newColor);
	UFUNCTION(NetMulticast, Unreliable)
	void ClientRPC_ChangeColor(const FLinearColor newColor);
};
