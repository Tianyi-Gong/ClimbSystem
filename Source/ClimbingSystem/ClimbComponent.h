// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "IAnimInt.h"
#include "IZipSystem.h"
#include "Animation/AnimInstance.h"
#include "MotionWarpingComponent.h"
#include "ClimbMontageAnimConfig.h"
#include "ClimbComponent.generated.h"

UENUM(BlueprintType)
enum class UJumpState : uint8
{
	Idle,
	Presse,
	Release
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class CLIMBINGSYSTEM_API UClimbComponent : public UActorComponent, public IIZipSystem
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UClimbComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	void JumpPressed();
	void JumpReleased();

	void CrouchPressed();
	void CrouchReleased();

	UFUNCTION()
	void OnModeModeChangeEvent(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);

	FVector2D MovementInput;
	UJumpState JumpState;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Debug)
	bool bDrawDebug;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AnimConfig, meta = (AllowPrivateAccess = "true"))
	UClimbMontageAnimConfig* ClimbMontageAnimConfig;

	UFUNCTION(BlueprintCallable)
	UClimbState GetClimbState();

	void INT_FinishZiplineGliding_Implementation();

private:
	void HandleJumpInput(float DeltaTime);
	void HandleDefaultMoveInput();
	void HandleClimbMoveInput();
	void HandleClimbPipeMoveInput();
	void HandleHangingMoveInput();
	void HandleBalanceMoveInput();
	void HandleNarrowSpaceMoveInput();
	void HandleLedgeWalkMoveInput();
	void HandleZipLineInput();

	void DefaultObstacleCheck(float DeltaTime);

	void ObstacleCheckDefault(float DeltaTime);
	void ObstacleCheckDefaultByInput();

	void ObstacleCheckClimbing(float DeltaTime);
	void ObstacleCheckClimbPipe(float DeltaTime);
	void ObstacleCheckHanging(float DeltaTime);
	void ObstacleCheckBalance(float DeltaTime);
	void ObstacleCheckNarrowSpace(float DeltaTime);
	void ObstacleCheckLedgeWalk(float DeltaTime, bool IsRightWalk);

	bool ObstacleDetectionDefault(float MinDistance, float MaxDistance, const FVector& Velocity, FVector& Location, FVector& Normal);
	bool HangingObstacleDetectionDefault(float MinDistance, float MaxDistance, const FVector& Velocity, FVector& Location, FVector& Normal);

	bool ObstacleDetectionClimbing(float Distance, FVector& Location, FVector& Normal);
	bool ObstacleDetectionHanging(float Distance, FVector& Location, FVector& Normal);
	bool FloorDectectionBalance(FVector& Location,FVector& Normal);
	bool ObstacleDetectionNarrowSpace(FVector& Location, FVector& Normal);
	bool ObstacleDetectionLedgeWalk(bool IsRightWalk, FVector& Location, FVector& Normal);

	bool ClimbUpCheck(float DeltaTime);
	bool ClimbRightCheck(float DeltaTime);
	bool ClimbLeftCheck(float DeltaTime);
	bool ClimbDownCheck(float DeltaTime);

	bool ClimbPipeUpCheck(float DeltaTime);
	bool ClimbPipeDownCheck(float DeltaTime);
	bool ClimbPipeRightCheck(float DeltaTime);
	bool ClimbPipeLeftCheck(float DeltaTime);

	bool HangingUpCheck(float DeltaTime);
	bool HangingDownCheck(float DeltaTime);
	bool HangingRightCheck(float DetalTime);
	bool HangingLeftCheck(float DetalTime);

	bool BalanceUpCheck(float DeltaTime);
	bool BalanceDownCheck(float DeltaTime);

	bool NarrowSpaceUpCheck(float DeltaTime);
	bool NarrowSpaceDownCheck(float DeltaTime);

	bool LedgeWalkUpCheck(float DeltaTime, bool IsRightWalk);
	bool LedgeWalkDownCheck(float DeltaTime, bool IsRightWalk);

	void DefaultFloorCheck(float DeltaTime);
	void DefaultNarrowSpaceCheck(float DeltaTime);

	bool ClimbRightJumpCheck();
	bool ClimbLeftJumpCheck();
	bool ClimbDownJumpCheck();
	bool ClimbUpJumpCheck();

	bool ClimbRightCornerInnerCheck();
	bool ClimbLeftCornerInnerCheck();
	bool HangingRightCornerInnerCheck();
	bool HangingLeftCornerInnerCheck();

	bool ClimbRightCornerOuterCheck();
	bool ClimbLeftCornerOuterCheck();
	bool HangingRightCornerOuterCheck();
	bool HangingLeftCornerOuterCheck();

	bool ClimbUpActionCheck();
	bool ClimbLandingCheck();
	bool ClimbingToHangingCheck();

	bool HangingClimbUpCheck();
	bool HangingTurnCheck();
	bool HangingDropCheck();

	bool ClimbPipeLandUpCheck();
	bool ClimbPipeLandDownCheck();
	bool ClimbPipeRightJumpCheck();
	bool ClimbPipeLeftJumpCheck();

	bool BalanceUpToWalkCheck();
	bool BalanceDownToWalkCheck();
	bool BalanceTurnBackCheck();

	bool NarrowSpaceUpToWalkCheck();
	bool NarrowSpaceDownToWalkCheck();

	bool LedgeWalkUpInsideCornerCheck(bool IsRightWalk);
	bool LedgeWalkDownInsideCornerCheck(bool IsRightWalk);
	bool LedgeWalkUpOutwardCornerCheck(bool IsRightWalk);
	bool LedgeWalkDownOutwardCornerCheck(bool IsRightWalk);
	bool LedgeWalkUpLedgeWalkToWalkCheck(bool IsRightWalk);
	bool LedgeWalkDownLedgeWalkToWalkCheck(bool IsRightWalk);

	FVector GetFootLocation() const;
	FVector GetTopLocation() const;

	void FindClimbingRotationUp();
	void FindClimbingRotationRight();
	void FindClimbingRotationLeft();
	void FindClimbingRotationDown();
	void FindClimbingRotationIdle();
	void FindBalanceRotationIdle(bool UseNormal = false);

	//void FindNarrowSpaceRotationUp();
	void FindNarrowSpaceRotationIdle();

	void FindLedgeWalkRotationIdle(bool IsRightWalk);

	void HandleClimbLerpTransfor(float DeltaTime);
	void HandleClimbPipeLerpTransfor(float DeltaTime);
	void HandleHangingLerpTransfor(float DeltaTime);
	void HandleBalanceLerpTransfor(float DeltaTime);
	void HandleNarrowSpaceLerpTransfor(float DeltaTime);
	void HandleLedgeWalkLerpTransfor(float DeltaTime);

	void SetUpDefaultState();
	void SetUpClimbingState();
	void SetUpClimbingPipeState();
	void SetUpHangingState();
	void SetUpBalanceState();
	void SetUpNarrowSpaceState();
	void SetUpLedgeWalkState(bool IsRightWalk);
	void SetUpZipLineState();

	bool ObstacleEndDetectionUp(float Distance, FVector& Location);
	bool ObstacleEndDetectionRight(float Distance, FVector& Location);
	bool ObstacleEndDetectionLeft(float Distance, FVector& Location);
	bool ObstacleEndDetectionDown(float Distance, FVector& Location);

	bool HangingEndDetectionRight(float Distance,FVector& Locatoon);
	bool HangingEndDetectionLeft(float Distance, FVector& Locatoon);

	bool BalanceEndDectionUp(FVector& Location,FVector& Normal);
	bool BalanceEndDectionDown(FVector& Location, FVector& Normal);

	bool NarrowSpaceEndDectionUp(FVector& Location, FVector& Normal);
	bool NarrowSpaceEndDectionDown(FVector& Location, FVector& Normal);

	bool LedgeWalkEndDectionUp(bool IsRightWalk, FVector& Location, FVector& Normal);
	bool LedgeWalkEndDectionDown(bool IsRightWalk, FVector& Location, FVector& Normal);

	bool FindMontagePlayInofoByClimbAction(UClimbAction ClimbAction, FMontagePlayInofo& outMontagePlayInofo);

	void HangingRemapInputVector();
	void BalanceRemapInputVector();

	UClimbState ClimbState = UClimbState::Default;

	bool bComponentInitalize = false;

	ACharacter* OwnerCharacter;
	UAnimInstance* ClimbingAnimInstance;
	UInputComponent* ClimbingInputComponent;
	UCharacterMovementComponent* ClimbingMovementComponent;
	UMotionWarpingComponent* MotionWarpingComponent;

	FVector ObstacleLocation;
	FVector ObstacleEndLocation;
	FVector ObstacleNormalDir;

	FVector ClimbingUpVector;
	FVector ClimbingRightVector;
	FVector ClimbingForwardVector;
	FRotator ClimbingRotation;

	FVector FloorLocation;
	FVector FloorNormalDir = FVector::UpVector;
	FVector FloorEndLocation;
	FVector FloorEndNormalDir;

	FRotator BalanceRotation;
	FRotator NarrowSpaceRotation;

	FRotator LedgeWalkRotation;

	AActor* ZipLineObj;
	float ZipLineTraceIntervalTime;
};
