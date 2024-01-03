// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ClimbMontageAnimConfig.generated.h"

USTRUCT(BlueprintType)
struct FMontagePlayInofo
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UAnimMontage* AnimMontageToPlay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D AnimMontageOffSet;
};

UENUM(BlueprintType)
enum class UClimbAction : uint8
{
	ClimbingAction_FallToClimbing,
	ClimbingAction_ClimbUpLand,
	ClimbingAction_JumpUp,
	ClimbingAction_JumpDown,
	ClimbingAction_LandDown,
	ClimbingAction_LeftJump,
	ClimbingAction_SuperLeftJump,
	ClimbingAction_RightJump,
	ClimbingAction_SuperRightJump,
	ClimbingAction_InnerLeft,
	ClimbingAction_InnerRight,
	ClimbingAction_OuterLeft,
	ClimbingAction_OuterRight,
	ClimbingAction_UpVault,
	ClimbingAction_UpTurnVault,
	ClimbingAction_ClimbingToHanging,
	ClimbAction_Climb220,
	ClimbAction_Climb100,
	ClimbAction_Vault220,
	ClimbAction_Vault100,
	ClimbAction_VaultTurn220,
	ClimbAction_VaultTurn100,
	ClimbPipeAction_StartClimbPipe,
	ClimbPipeAction_ClimbUpLand,
	ClimbPipeAction_LandDown,
	ClimbPipeAction_LeftJump,
	ClimbPipeAction_RightJump,
	Hanging_AttachHanging,
	Hanging_InnerLeft,
	Hanging_InnerRight,
	Hanging_OuterLeft,
	Hanging_OuterRight,
	Hanging_ClimbUp,
	Hanging_Turn,
	Hanging_Drop,
	FallToLand_Roll,
	FallToLand_Front,
	FallToLand_LandingGround,
	Walk_Slider,
	Walk_WalkToBalance,
	Walk_WalkToNarrowSpace,
	Walk_WalkToLedgeWalkRight,
	Walk_WalkToLedgeWalkLeft,
	Walk_WalkToZipLine,
	Balance_BalanceUpToWalk,
	Balance_BalanceDownToWalk,
	Balance_BalanceTurnBack,
	NarrowSpace_NarrowSpaceUpToWalk,
	NarrowSpace_NarrowSpaceDownToWalk,
	LedgeWalkRight_UpInsideCorner,
	LedgeWalkRight_DownInsideCorner,
	LedgeWalkRight_UpOutwardCorner,
	LedgeWalkRight_DownOutwardCorner,
	LedgeWalkRight_UpLedgeWalkToWalk,
	LedgeWalkRight_DownLedgeWalkToWalk,
	LedgeWalkLeft_UpInsideCorner,
	LedgeWalkLeft_DownInsideCorner,
	LedgeWalkLeft_UpOutwardCorner,
	LedgeWalkLeft_DownOutwardCorner,
	LedgeWalkLeft_UpLedgeWalkToWalk,
	LedgeWalkLeft_DownLedgeWalkToWalk,
	ZipLine_ZipLineGlidingToWalk,
	ZipLine_RightFallToZipLine,
	ZipLine_LeftFallToZipLine
};
/**
 * 
 */	
UCLASS(BlueprintType)
class CLIMBINGSYSTEM_API UClimbMontageAnimConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionFallToClimbing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionClimbUpLand;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionJumpUp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionJumpDown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionLandDown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionLeftJump;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionSuperLeftJump;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionRightJump;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionSuperRightJump;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionInnerLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionInnerRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionOuterLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionOuterRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionUpVault;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionUpTurnVault;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbingActionClimbingToHanging;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbActionClimb220;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbActionClimb100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbActionVault220;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbActionVault100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbActionVaultTurn220;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbActionVaultTurn100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbPipeStartClimbPipe;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbPipeClimbUpLand;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbPipeLandDown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbPipeLeftJump;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ClimbPipeRightJump;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> HangingAttachHanging;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> HangingInnerLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> HangingInnerRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> HangingOuterLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> HangingOuterRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> HangingClimbUp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> HangingTurn;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> HangingDrop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> FallToLandRoll;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> FallToLandFront;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> FallToLandLandingGround;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> WalkSlider;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> WalkToBalance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> WalkToNarrowSpace;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> WalkToLedgeWalkRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> WalkToLedgeWalkLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> WalkToZipLine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> BalanceUpToWalk;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> BalanceDownToWalk;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> BalanceTurnBack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> NarrowSpaceUpToWalk;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> NarrowSpaceDownToWalk;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> LedgeWalkRightUpInsideCorner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> LedgeWalkRightDownInsideCorner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> LedgeWalkRightUpOutwardCorner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> LedgeWalkRightDownOutwardCorner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> LedgeWalkRightUpLedgeWalkToWalk;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> LedgeWalkRightDownLedgeWalkToWalk;

		UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> LedgeWalkLeftUpInsideCorner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> LedgeWalkLeftDownInsideCorner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> LedgeWalkLeftUpOutwardCorner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> LedgeWalkLeftDownOutwardCorner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> LedgeWalkLeftUpLedgeWalkToWalk;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> LedgeWalkLeftDownLedgeWalkToWalk;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> ZipLineGlidingToWalk;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> RightFallToZipLine;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	TArray<FMontagePlayInofo> LeftFallToZipLine;
	
	bool GetMontagePlayInofoByClimbAction(UClimbAction ClimbAction, FMontagePlayInofo& outMontagePlayInofo);
};
