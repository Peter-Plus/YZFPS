// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/AudioComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

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
	UpdateShakeChannels(DeltaTime);
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

	SetOwner(PickerPawn);
	OnPickedUp(PickerPawn);
}

void AWeaponBase::Fire()
{
	if (WeaponMesh == nullptr) return;
	UWorld* World = GetWorld();
	if (World == nullptr) return;
	// 调试日志：打印 Fire 时三个通道的强度
	UE_LOG(LogTemp, Warning, TEXT("Fire | Speed=%.2f | FireCh=%.2f | Crouch=%.2f | OwnerVel=%.2f"),
		SpeedChannel.Strength,
		FireChannel.Strength,
		CrouchChannel.Strength,
		GetOwner() ? GetOwner()->GetVelocity().Size() : -1.0f);

	//1.获取枪口位置方向
	const FVector MuzzleLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName);
	//2.算射击方向
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC == nullptr) return;
	int32 ViewportX = 0, ViewportY = 0;
	PC->GetViewportSize(ViewportX, ViewportY);

	// 三通道各自随机偏移（X 和 Y 独立）
	auto RandomOffset = [](float Strength) -> FVector2D
		{
			return FVector2D(
				FMath::RandRange(-Strength, Strength),
				FMath::RandRange(-Strength, Strength)
			);
		};

	const FVector2D TotalOffset =
		RandomOffset(SpeedChannel.Strength) +
		RandomOffset(FireChannel.Strength) +
		RandomOffset(CrouchChannel.Strength);

	const float CenterScreenX = ViewportX * 0.5f;
	const float CenterScreenY = ViewportY * 0.5f;
	const float AimedScreenX = CenterScreenX + TotalOffset.X;
	const float AimedScreenY = CenterScreenY + TotalOffset.Y;

	//3. Deproject 偏移屏幕中心 → 拿到瞄准方向，构造前方 5m 瞄准点 ===
	FVector AimedWorldPos;
	FVector AimedWorldDir;
	if (!PC->DeprojectScreenPositionToWorld(AimedScreenX, AimedScreenY, AimedWorldPos, AimedWorldDir))
	{
		return;
	}
	const FVector Aim5mPoint = AimedWorldPos + AimedWorldDir * 500.0f;  // 前方5m魔法距离


	//4. Deproject 屏幕中心 → 射线起点 ===
	FVector CenterWorldPos;
	FVector CenterWorldDir;
	if (!PC->DeprojectScreenPositionToWorld(CenterScreenX, CenterScreenY, CenterWorldPos, CenterWorldDir))
	{
		return;
	}

	//5. 实际射线方向 = 屏幕中心 → 5m 瞄准点 ===
	const FVector RayDir = (Aim5mPoint - CenterWorldPos).GetSafeNormal();

	//6. 射线起点屏幕中心、终点起点 + 方向 * TraceDistance ===
	const FVector TraceStart = CenterWorldPos;
	const FVector TraceEnd = CenterWorldPos + RayDir * TraceDistance;

	//7. 射线检测 ===
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = World->LineTraceSingleByChannel(
		Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams
	);

	//8.如果命中，设命中点为终点，否则用射线检测终点
	const FVector ImpactPoint = bHit ? Hit.ImpactPoint : TraceEnd;

	//9.播放音效和枪口火焰
	FireAudio->Activate(true);
	MuzzleParticle->Activate(true);

	//10.通知蓝图，生成子弹设置速度
	OnFireBullet(MuzzleLocation, ImpactPoint);

	//11.开火脉冲——直接加，不平滑
	FireChannel.Strength = FMath::Min(
		FireChannel.Strength + FireChannel.PulseAmount,
		FireChannel.MaxStrength
	);
}

void AWeaponBase::AddCrouchPulse()
{
	CrouchChannel.Strength = FMath::Min(
		CrouchChannel.Strength + CrouchChannel.PulseAmount,
		CrouchChannel.MaxStrength
	);
}

void AWeaponBase::UpdateShakeChannels(float DeltaTime)
{
	// SpeedChannel：追当前 Owner 的速度对应的目标强度
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn != nullptr)
	{
		const float CurrentSpeed = OwnerPawn->GetVelocity().Size();
		const float TargetStrength = FMath::Min(
			CurrentSpeed * SpeedToStrengthRatio,
			SpeedChannel.MaxStrength
		);
		SpeedChannel.Strength = FMath::FInterpTo(
			SpeedChannel.Strength, TargetStrength, DeltaTime, SpeedChannel.SmoothSpeed
		);
	}

	// FireChannel：追 0（衰减）
	FireChannel.Strength = FMath::FInterpTo(
		FireChannel.Strength, 0.0f, DeltaTime, FireChannel.SmoothSpeed
	);

	// CrouchChannel：追 0（衰减）
	CrouchChannel.Strength = FMath::FInterpTo(
		CrouchChannel.Strength, 0.0f, DeltaTime, CrouchChannel.SmoothSpeed
	);

	// 调试：屏幕上实时显示三个通道的强度
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(
			1, 0.0f, FColor::Yellow,
			FString::Printf(TEXT("Speed: %.2f"), SpeedChannel.Strength)
		);
		GEngine->AddOnScreenDebugMessage(
			2, 0.0f, FColor::Cyan,
			FString::Printf(TEXT("Fire:  %.2f"), FireChannel.Strength)
		);
		GEngine->AddOnScreenDebugMessage(
			3, 0.0f, FColor::Green,
			FString::Printf(TEXT("Crouch:%.2f"), CrouchChannel.Strength)
		);
	}
}

