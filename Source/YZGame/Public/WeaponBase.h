// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponBase.generated.h"

class USkeletalMeshComponent;
class USphereComponent;
class UAudioComponent;
class UParticleSystemComponent;
class UPrimitiveComponent;

UCLASS()
class YZGAME_API AWeaponBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeaponBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//武器骨骼模型
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	USkeletalMeshComponent* WeaponMesh;

	//拾取触发体
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	USphereComponent* PickupSphere;

	//开火音效
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	UAudioComponent* FireAudio;

	//开火火焰特效
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Components")
	UParticleSystemComponent* MuzzleParticle;

	//是否已经装备
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	bool bIsEquipped = false;

	//子弹类
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Config")
	TSubclassOf<AActor> BulletClass;

	//射线检测最大距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Config")
	float TraceDistance = 20000.0f;

	/** 待机时每帧绕 Z 轴的旋转角度（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Config")
	float IdleRotateSpeed = 3.0f;

	/** 武器骨骼上"枪口"的插槽名 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Config")
	FName MuzzleSocketName = FName("ShootPoint");

	//开火，玩家蓝图调用
	UFUNCTION(BlueprintCallable, Category = "Weapon|Combat")
	void Fire();

	//================================蓝图钩子=============================================

	//武器被拾取时调用
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Pickup")
	void OnPickedUp(APawn* PickerPawn);

	//开火生成子弹，设置速度
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Combat")
	void OnFireBullet(const FVector& MuzzleLocation, const FVector& ImpactPoint);

private:
	UFUNCTION()
	void OnPickupSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);
};
