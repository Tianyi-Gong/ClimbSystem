// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "IAnimInt.h"
#include "ClimbCharacterAnimInstance.generated.h"

UENUM(BlueprintType)
enum class EClimbingDirection : uint8
{
	Direction_Up,
	Direction_Down,
	Direction_Left,
	Direction_Right,
	Direction_UpLeft,
	Direction_UpRight,
	Direction_DownLeft,
	Direction_DownRight
};
/**
 * 
 */
UCLASS()
class CLIMBINGSYSTEM_API UClimbCharacterAnimInstance : public UAnimInstance, public IIAnimInt
{
	GENERATED_BODY()
};
