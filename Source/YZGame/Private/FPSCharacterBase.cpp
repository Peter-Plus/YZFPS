#include "FPSCharacterBase.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"

AFPSCharacterBase::AFPSCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFPSCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	CachedCamera = FindComponentByClass<UCameraComponent>();
	if (CachedCamera == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AFPSCharacterBase: No CameraComponent found"));
	}

	LastCapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

void AFPSCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (CachedCamera == nullptr) return;

	// 1. 检测 Capsule 半高变化（蹲下/起身/任何原因），立刻补偿相机相对 Z 让绝对位置不动
	const float CurrentHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float HalfHeightDelta = LastCapsuleHalfHeight - CurrentHalfHeight;
	if (FMath::Abs(HalfHeightDelta) > KINDA_SMALL_NUMBER)
	{
		FVector Loc = CachedCamera->GetRelativeLocation();
		Loc.Z += HalfHeightDelta;
		CachedCamera->SetRelativeLocation(Loc);
		LastCapsuleHalfHeight = CurrentHalfHeight;
	}

	// 2. 平滑追到目标相对 Z
	const FVector CurrentLoc = CachedCamera->GetRelativeLocation();
	const float NewZ = FMath::FInterpTo(CurrentLoc.Z, CameraRelativeZ, DeltaTime, EyeHeightInterpSpeed);
	CachedCamera->SetRelativeLocation(FVector(CurrentLoc.X, CurrentLoc.Y, NewZ));
}