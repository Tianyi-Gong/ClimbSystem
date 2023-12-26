// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IAnimInt.generated.h"

UENUM(BlueprintType)
enum class UClimbState : uint8
{
	Default,
	Climbing,
	ClimbingPipe,
	Hanging,
	Balance,
	NarrowSpace,
	LedgeWalkRight,
	LedgeWalkLeft
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UIAnimInt : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CLIMBINGSYSTEM_API IIAnimInt
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void INT_Direction(const float Direction);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void INT_ChangeClimbPosture(UClimbState State);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void INT_Input(FVector2D input);
};
