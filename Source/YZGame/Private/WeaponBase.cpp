// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

// Sets default values
AWeaponBase::AWeaponBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 用一个空的 SceneComponent 作为 Root，让 WeaponMesh 可以自由偏移
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(Root);

	//触发球
	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(WeaponMesh);
	PickupSphere->SetSphereRadius(80.f);

	//开火音效
	FireAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FireAudio"));
	FireAudio->SetupAttachment(WeaponMesh);
	FireAudio->bAutoActivate = false;

	//枪口火焰
	MuzzleParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("MuzzleParticle"));
	MuzzleParticle->SetupAttachment(WeaponMesh, MuzzleSocketName);
	MuzzleParticle->bAutoActivate = false;

}

// Called when the game starts or when spawned
void AWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	//保险设置特效音效不激活
	FireAudio->Deactivate();
	MuzzleParticle->Deactivate();

	//绑定拾取重叠事件
	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeaponBase::OnPickupSphereBeginOverlap);
	
}

// Called every frame
void AWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsEquipped)
	{
		AddActorWorldRotation(FRotator(0.0f, IdleRotateSpeed, 0.0f));
	}
}

void AWeaponBase::OnPickupSphereBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
) {
	//已装备不再触发
	if (bIsEquipped) return;
	//只对pawn响应
	APawn* PickerPawn = Cast<APawn>(OtherActor);
	if (PickerPawn == nullptr) return;

	bIsEquipped = true;
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetActorRelativeRotation(FRotator::ZeroRotator);
	if (WeaponMesh != nullptr) WeaponMesh->SetRelativeLocation(FVector::ZeroVector);

	OnPickedUp(PickerPawn);
}

void AWeaponBase::Fire()
{
	UWorld* World = GetWorld();
	if (World == nullptr) return;
	//1.获取枪口位置方向
	const FVector MuzzleLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName);
	//2.算射击方向
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC == nullptr) return;
	int32 ViewportX = 0, ViewportY = 0;
	PC->GetViewportSize(ViewportX, ViewportY);

	FVector ScreenWorldLocation;
	FVector ScreenWorldDirection;
	const bool bDeprojected = PC->DeprojectScreenPositionToWorld(
		ViewportX * 0.5f,
		ViewportY * 0.5f,
		ScreenWorldLocation,
		ScreenWorldDirection
	);

	if (!bDeprojected) return;

	//3.射线起点-屏幕中心，终点是远处
	const FVector TraceStart = ScreenWorldLocation;
	const FVector TraceEnd = ScreenWorldLocation + ScreenWorldDirection * TraceDistance;

	//4.做射线检测
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);

	//5.如果命中，设命中点为终点，否则用射线检测终点
	const FVector ImpactPoint = bHit ? Hit.ImpactPoint : TraceEnd;

	//6.播放音效和枪口火焰
	FireAudio->Activate(true);
	MuzzleParticle->Activate(true);

	//7.通知蓝图，生成子弹设置速度
	OnFireBullet(MuzzleLocation, ImpactPoint);
}

