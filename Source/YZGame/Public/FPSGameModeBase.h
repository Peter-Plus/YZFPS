// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FPSGameModeBase.generated.h"

class AEnemyBase;

/**
 * 
 */
UCLASS()
class YZGAME_API AFPSGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPSGameModeBase();

protected:
	virtual void BeginPlay() override;
	
private:
	//是否正在等待生成敌人（防止 CreateEnemy 被重复触发）
	bool bIsSpawning = false;

public:
	//当前累积生成的敌人数量（包括死亡的）
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Enemy")
	int32 Count = 0;

	//最多生成多少个敌人 （蓝图可在 Class Defaults 配置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Enemy")
	int32 MaxKillCount = 3;

	//生成敌人前的提示等待时间
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Enemy")
	float SpawnRadius = 2500.f;

	//生成敌人前的提示等待时间
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Enemy")
	float SpawnDelay = 2.f;

	//可以生成的敌人蓝图类列表 （蓝图可在 Class Defaults 配置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Enemy")
	TArray<TSubclassOf<AEnemyBase>> EnemyClassList;

	//当前场景中存活的敌人列表
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Enemy")
	TArray<AEnemyBase*> EnemyList;

	//敌人生成中心点
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Enemy")
	AActor* CenterPoint = nullptr;

	//生成一个敌人，先调OnSpawnHint,等待spawndelay后再真正生成
	UFUNCTION(BlueprintCallable, Category = "FPS|Enemy")
	void CreateEnemy();

	//通知敌人已死亡，从列表移除并判断是否胜利或继续刷怪
	UFUNCTION(BlueprintCallable, Category = "FPS|Enemy")
	void SetEnemyDie(AEnemyBase* Enemy);

	//玩家受击时调用（玩家蓝图调用），转发给血屏UI
	UFUNCTION(BlueprintCallable, Category = "FPS|UI")
	void PlayBloodScreen();

	//显示游戏结束面板（玩家死亡时会调用）
	UFUNCTION(BlueprintCallable, Category = "FPS|UI")
	void ShowGameOver(const FText& Message);

	//==============================蓝图实现的UI钩子==================================

	//游戏开始：创建mainUI、addToViewport、设置输入模式
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|UI")
	void OnGameStart();
	
	//生成敌人前的提示
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|UI")
	void OnSpawnHint();

	//播放受击血屏动画
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|UI")
	void OnPlayBloodScreen();

	//显示游戏结束面板
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|UI")
	void OnShowGameOver(const FText& Message);

private:
	//真正生成敌人，在createEnemy里延迟delay后调用
	void DoSpawnEnemy();

};
