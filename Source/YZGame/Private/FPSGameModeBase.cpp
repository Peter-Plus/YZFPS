// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSGameModeBase.h"
#include "EnemyBase.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"

AFPSGameModeBase::AFPSGameModeBase()
{
	//GameMode不需要每帧tick
	PrimaryActorTick.bCanEverTick = false;
}

void AFPSGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	//在场景中招第一个带“spawn”tag的Actor
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsWithTag(this, FName("Spawn"), FoundActors);
	if (FoundActors.Num() > 0)
	{
		CenterPoint = FoundActors[0];
	}

	//通知蓝图子类，游戏开始了，可以创建UI
	OnGameStart();
}

void AFPSGameModeBase::CreateEnemy()
{
	// 去重保护：已经在等待生成中就直接返回
	if (bIsSpawning)
	{
		UE_LOG(LogTemp, Verbose, TEXT("CreateEnemy: already spawning, skip"));
		return;
	}

	// 已达到目标数量也不再生成
	if (Count >= MaxKillCount)
	{
		return;
	}

	bIsSpawning = true;

	//通知蓝图显示刷怪提示
	OnSpawnHint();

	FTimerHandle SpawnTimer;
	GetWorldTimerManager().SetTimer(
		SpawnTimer,
		this,
		&AFPSGameModeBase::DoSpawnEnemy,
		SpawnDelay,
		false
	);
}

void AFPSGameModeBase::DoSpawnEnemy()
{
	//边界检查
	if (EnemyClassList.Num() == 0 || CenterPoint == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("DoSpawnEnemy:EnemyClassList or CenterPoint not configured"));
		return;
	}

	//在cp周围找到一个可达导航点
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	if (NavSys == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("DoSpawnEnemy:No navigation system in level"));
		return;
	}

	FNavLocation RandomNavLocation;
	const bool bFoundLocation = NavSys->GetRandomReachablePointInRadius(
		CenterPoint->GetActorLocation(),
		SpawnRadius,
		RandomNavLocation
	);

	if (!bFoundLocation)
	{
		UE_LOG(LogTemp, Warning, TEXT("DoSpawnEnemy:Failed to get randomNaviPoint"));
		return;
	}

	//抬高100，避免陷入地面
	FVector SpawnLocation = RandomNavLocation.Location + FVector(0.0f, 0.0f, 100.0f);

	//从EnemyClassList随机一个类
	const int32 RandomIndex = FMath::RandRange(0, EnemyClassList.Num() - 1);
	TSubclassOf<AEnemyBase> EnemyClass = EnemyClassList[RandomIndex];
	if (EnemyClass == nullptr) return;

	//SpawnActor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AEnemyBase* NewEnemy = GetWorld()->SpawnActor<AEnemyBase>(
		EnemyClass,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	//添加到列表，计数+1
	if (NewEnemy != nullptr)
	{
		EnemyList.AddUnique(NewEnemy);
		Count++;
	}

	bIsSpawning = false;
}

void AFPSGameModeBase::SetEnemyDie(AEnemyBase* Enemy)
{
	if (Enemy == nullptr)
	{
		return;
	}
	EnemyList.Remove(Enemy);
	//判断数量->胜利/继续生成
	if (Count < MaxKillCount)
	{
		CreateEnemy();
	}
	else
	{
		ShowGameOver(FText::FromString(TEXT("Victiory")));
	}
}

void AFPSGameModeBase::PlayBloodScreen()
{
	OnPlayBloodScreen();
}

void AFPSGameModeBase::ShowGameOver(const FText& Message)
{
	OnShowGameOver(Message);
}
