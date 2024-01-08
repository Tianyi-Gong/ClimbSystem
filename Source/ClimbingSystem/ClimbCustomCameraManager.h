// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "ClimbCustomCameraInterface.h"
#include "ClimbCustomCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class CLIMBINGSYSTEM_API AClimbCustomCameraManager : public APlayerCameraManager, public IClimbCustomCameraInterface
{
	GENERATED_BODY()
public:
	AClimbCustomCameraManager();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* CameraMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float CameraTrachRadius = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Camera, meta = (AllowPrivateAccess = "true"))
	bool bDrawDebug = false;

	virtual void BeginPlay() override;

	void INT_OnPossess_Implementation(APawn* ControlledPawn);
protected:
	virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime) override;

	virtual bool NativeUpdateCamera(AActor* CameraTarget, FVector& NewCameraLocation, FRotator& NewCameraRotation, float& NewCameraFOV);

	float GetCameraBehaviorParam(FName CurveName);
	FVector CalculateAxisIndependentLag(FVector CurrentLocation, FVector TargetLocation, FRotator CameraRotation, FVector LagSpeeds);

	UAnimInstance* CameraAnimInstace;

	APawn* ControlPawn;
	FTransform PivotTarget;
	float FOV;
	bool RightShoulder;

	FRotator TargetCameraRotation;
	FVector TargetCameraLocation;

	FRotator DebugViewRotation = FRotator(0, -5, 180);
	FVector DebugViewOffset = FVector(350, 0, 50);
	FTransform SmoothedPivotTarget;
	FVector PivotLocation;
};
