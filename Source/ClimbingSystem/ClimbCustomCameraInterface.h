// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameFramework/Pawn.h"
#include "IAnimInt.h"
#include "ClimbCustomCameraInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UClimbCustomCameraInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CLIMBINGSYSTEM_API IClimbCustomCameraInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = CameraSystem)
	void INT_OnPossess(APawn* ControlledPawn);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = CameraBehavior)
	void INT_SetControllerData(APawn* ControlledPawn, APlayerController* PlayerController);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = CameraCharacter)
	void INT_PawnIsSupportClimbCustomCamera(bool& IsSupportClimbCustomCamera);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = CameraCharacter)
	void INT_GetCameraParameters(FTransform& PivotTarget, float& FOV, bool& RightShoulder);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = CameraCharacter)
	void INT_GetLimitCameraRotation(bool& IsLimitCameraRotation , FRotator& MinRotation, FRotator& MaxRotation);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = CameraCharacter)
	void INT_GetClimbState(UClimbState& ClimbState);
};
