// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPSCharacterBase.generated.h"

class UCameraComponent;

UCLASS()
class YZGAME_API AFPSCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:
	AFPSCharacterBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Camera")
	float CameraRelativeZ = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Camera")
	float EyeHeightInterpSpeed = 8.0f;

private:
	UPROPERTY()
	UCameraComponent* CachedCamera = nullptr;

	float LastCapsuleHalfHeight = 0.0f;
};