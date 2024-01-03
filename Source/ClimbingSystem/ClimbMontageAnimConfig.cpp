// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbMontageAnimConfig.h"

bool UClimbMontageAnimConfig::GetMontagePlayInofoByClimbAction(UClimbAction ClimbAction, FMontagePlayInofo& outMontagePlayInofo)
{
	FMontagePlayInofo MontagePlayInofo;

	TArray<FMontagePlayInofo> MontagePlayInofoList;

	switch (ClimbAction)
	{
	case UClimbAction::ClimbingAction_FallToClimbing:
		MontagePlayInofoList = ClimbingActionFallToClimbing;
		break;
	case UClimbAction::ClimbingAction_ClimbUpLand:
		MontagePlayInofoList = ClimbingActionClimbUpLand;
		break;
	case UClimbAction::ClimbingAction_JumpUp:
		MontagePlayInofoList = ClimbingActionJumpUp;
		break;
	case UClimbAction::ClimbingAction_JumpDown:
		MontagePlayInofoList = ClimbingActionJumpDown;
		break;
	case UClimbAction::ClimbingAction_LandDown:
		MontagePlayInofoList = ClimbingActionLandDown;
		break;
	case UClimbAction::ClimbingAction_LeftJump:
		MontagePlayInofoList = ClimbingActionLeftJump;
		break;
	case UClimbAction::ClimbingAction_SuperLeftJump:
		MontagePlayInofoList = ClimbingActionSuperLeftJump;
		break;
	case UClimbAction::ClimbingAction_RightJump:
		MontagePlayInofoList = ClimbingActionRightJump;
		break;
	case UClimbAction::ClimbingAction_SuperRightJump:
		MontagePlayInofoList = ClimbingActionSuperRightJump;
		break;
	case UClimbAction::ClimbingAction_InnerLeft:
		MontagePlayInofoList = ClimbingActionInnerLeft;
		break;
	case UClimbAction::ClimbingAction_InnerRight:
		MontagePlayInofoList = ClimbingActionInnerRight;
		break;
	case UClimbAction::ClimbingAction_OuterLeft:
		MontagePlayInofoList = ClimbingActionOuterLeft;
		break;
	case UClimbAction::ClimbingAction_OuterRight:
		MontagePlayInofoList = ClimbingActionOuterRight;
		break;
	case UClimbAction::ClimbingAction_UpVault:
		MontagePlayInofoList = ClimbingActionUpVault;
		break;
	case UClimbAction::ClimbingAction_UpTurnVault:
		MontagePlayInofoList = ClimbingActionUpTurnVault;
		break;
	case UClimbAction::ClimbingAction_ClimbingToHanging:
		MontagePlayInofoList = ClimbingActionClimbingToHanging;
		break;
	case UClimbAction::ClimbAction_Climb220:
		MontagePlayInofoList = ClimbActionClimb220;
		break;
	case UClimbAction::ClimbAction_Climb100:
		MontagePlayInofoList = ClimbActionClimb100;
		break;
	case UClimbAction::ClimbAction_Vault220:
		MontagePlayInofoList = ClimbActionVault220;
		break;
	case UClimbAction::ClimbAction_Vault100:
		MontagePlayInofoList = ClimbActionVault100;
		break;
	case UClimbAction::ClimbAction_VaultTurn220:
		MontagePlayInofoList = ClimbActionVaultTurn220;
		break;
	case UClimbAction::ClimbAction_VaultTurn100:
		MontagePlayInofoList = ClimbActionVaultTurn100;
		break;
	case UClimbAction::ClimbPipeAction_StartClimbPipe:
		MontagePlayInofoList = ClimbPipeStartClimbPipe;
		break;
	case UClimbAction::ClimbPipeAction_ClimbUpLand:
		MontagePlayInofoList = ClimbPipeClimbUpLand;
		break;
	case UClimbAction::ClimbPipeAction_LandDown:
		MontagePlayInofoList = ClimbPipeLandDown;
		break;
	case UClimbAction::ClimbPipeAction_LeftJump:
		MontagePlayInofoList = ClimbPipeLeftJump;
		break;
	case UClimbAction::ClimbPipeAction_RightJump:
		MontagePlayInofoList = ClimbPipeRightJump;
		break;
	case UClimbAction::Hanging_AttachHanging:
		MontagePlayInofoList = HangingAttachHanging;
		break;
	case UClimbAction::Hanging_InnerLeft:
		MontagePlayInofoList = HangingInnerLeft;
		break;
	case UClimbAction::Hanging_InnerRight:
		MontagePlayInofoList = HangingInnerRight;
		break;
	case UClimbAction::Hanging_OuterLeft:
		MontagePlayInofoList = HangingOuterLeft;
		break;
	case UClimbAction::Hanging_OuterRight:
		MontagePlayInofoList = HangingOuterRight;
		break;
	case UClimbAction::Hanging_ClimbUp:
		MontagePlayInofoList = HangingClimbUp;
		break;
	case UClimbAction::Hanging_Turn:
		MontagePlayInofoList = HangingTurn;
		break;
	case UClimbAction::Hanging_Drop:
		MontagePlayInofoList = HangingDrop;
		break;
	case UClimbAction::FallToLand_Roll:
		MontagePlayInofoList = FallToLandRoll;
		break;
	case UClimbAction::FallToLand_Front:
		MontagePlayInofoList = FallToLandFront;
		break;
	case UClimbAction::FallToLand_LandingGround:
		MontagePlayInofoList = FallToLandLandingGround;
		break;
	case UClimbAction::Walk_Slider:
		MontagePlayInofoList = WalkSlider;
		break;
	case UClimbAction::Walk_WalkToBalance:
		MontagePlayInofoList = WalkToBalance;
		break;
	case UClimbAction::Walk_WalkToNarrowSpace:
		MontagePlayInofoList = WalkToNarrowSpace;
		break;
	case UClimbAction::Walk_WalkToLedgeWalkRight:
		MontagePlayInofoList = WalkToLedgeWalkRight;
		break;
	case UClimbAction::Walk_WalkToLedgeWalkLeft:
		MontagePlayInofoList = WalkToLedgeWalkLeft;
		break;
	case UClimbAction::Walk_WalkToZipLine:
		MontagePlayInofoList = WalkToZipLine;
		break;
	case UClimbAction::Balance_BalanceUpToWalk:
		MontagePlayInofoList = BalanceUpToWalk;
		break;
	case UClimbAction::Balance_BalanceDownToWalk:
		MontagePlayInofoList = BalanceDownToWalk;
		break;
	case UClimbAction::Balance_BalanceTurnBack:
		MontagePlayInofoList = BalanceTurnBack;
		break;
	case UClimbAction::NarrowSpace_NarrowSpaceUpToWalk:
		MontagePlayInofoList = NarrowSpaceUpToWalk;
		break;
	case UClimbAction::NarrowSpace_NarrowSpaceDownToWalk:
		MontagePlayInofoList = NarrowSpaceDownToWalk;
		break;
	case UClimbAction::LedgeWalkRight_UpInsideCorner:
		MontagePlayInofoList = LedgeWalkRightUpInsideCorner;
		break;
	case UClimbAction::LedgeWalkRight_DownInsideCorner:
		MontagePlayInofoList = LedgeWalkRightDownInsideCorner;
		break;
	case UClimbAction::LedgeWalkRight_UpOutwardCorner:
		MontagePlayInofoList = LedgeWalkRightUpOutwardCorner;
		break;
	case UClimbAction::LedgeWalkRight_DownOutwardCorner:
		MontagePlayInofoList = LedgeWalkRightDownOutwardCorner;
		break;
	case UClimbAction::LedgeWalkRight_UpLedgeWalkToWalk:
		MontagePlayInofoList = LedgeWalkRightUpLedgeWalkToWalk;
		break;
	case UClimbAction::LedgeWalkRight_DownLedgeWalkToWalk:
		MontagePlayInofoList = LedgeWalkRightDownLedgeWalkToWalk;
		break;
	case UClimbAction::LedgeWalkLeft_UpInsideCorner:
		MontagePlayInofoList = LedgeWalkLeftUpInsideCorner;
		break;
	case UClimbAction::LedgeWalkLeft_DownInsideCorner:
		MontagePlayInofoList = LedgeWalkLeftDownInsideCorner;
		break;
	case UClimbAction::LedgeWalkLeft_UpOutwardCorner:
		MontagePlayInofoList = LedgeWalkLeftUpOutwardCorner;
		break;
	case UClimbAction::LedgeWalkLeft_DownOutwardCorner:
		MontagePlayInofoList = LedgeWalkLeftDownOutwardCorner;
		break;
	case UClimbAction::LedgeWalkLeft_UpLedgeWalkToWalk:
		MontagePlayInofoList = LedgeWalkLeftUpLedgeWalkToWalk;
		break;
	case UClimbAction::LedgeWalkLeft_DownLedgeWalkToWalk:
		MontagePlayInofoList = LedgeWalkLeftDownLedgeWalkToWalk;
		break;
	case UClimbAction::ZipLine_ZipLineGlidingToWalk:
		MontagePlayInofoList = ZipLineGlidingToWalk;
		break;
	case UClimbAction::ZipLine_RightFallToZipLine:
		MontagePlayInofoList = RightFallToZipLine;
		break;
	case UClimbAction::ZipLine_LeftFallToZipLine:
		MontagePlayInofoList = LeftFallToZipLine;
		break;
	default:
		break;
	}
	
	if(MontagePlayInofoList.Num() == 0)
	{	
		return false;
	}

	int RandomIndex = FMath::RandRange(0, MontagePlayInofoList.Num() - 1);

	outMontagePlayInofo = MontagePlayInofoList[RandomIndex];

	return true;
}
