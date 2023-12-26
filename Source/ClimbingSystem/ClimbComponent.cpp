// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Components/CapsuleComponent.h"
#include "TraceBlueprintFunctionLibrary.h"

float GHangingTraceOffsetZ = 24;

static float GBalanceTraceScale = 0.75;
static FAutoConsoleVariableRef CVarBalanceTraceScale(
	TEXT("Clamb.BalanceTraceScale"),
	GBalanceTraceScale,
	TEXT("Balance Trace Scale"),
	ECVF_Default
);

static float GNarrowSpaceTraceLength = 15;
static FAutoConsoleVariableRef CVarNarrowSpaceTraceLength(
	TEXT("Clamb.NarrowSpaceTraceLength"),
	GNarrowSpaceTraceLength,
	TEXT("Narrow Space Trace Length"),
	ECVF_Default
);

// Sets default values for this component's properties
UClimbComponent::UClimbComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UClimbComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter != nullptr)
	{
		ClimbingAnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
		ClimbingInputComponent = OwnerCharacter->InputComponent;

		ClimbingMovementComponent = Cast<UCharacterMovementComponent>(OwnerCharacter->GetMovementComponent());
		ClimbingMovementComponent->bCanWalkOffLedgesWhenCrouching = true;

		OwnerCharacter->MovementModeChangedDelegate.AddDynamic(this,&UClimbComponent::OnModeModeChangeEvent);
	}
	else
	{
		return;
	}

	if (UEnhancedInputComponent* ClimbingEnhancedInputComponent = CastChecked<UEnhancedInputComponent>(ClimbingInputComponent))
	{
		ClimbingEnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &UClimbComponent::Move);
		ClimbingEnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &UClimbComponent::JumpPressed);
		ClimbingEnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &UClimbComponent::JumpReleased);
		ClimbingEnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &UClimbComponent::CrouchPressed);
		ClimbingEnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &UClimbComponent::CrouchReleased);
	}
	else
	{
		return;
	}

	MotionWarpingComponent = OwnerCharacter->FindComponentByClass<UMotionWarpingComponent>();
	if(MotionWarpingComponent == nullptr)
	{
		MotionWarpingComponent = Cast<UMotionWarpingComponent>(OwnerCharacter->AddComponentByClass(UMotionWarpingComponent::StaticClass(), false, FTransform(), false));
	}

	bComponentInitalize = true;
}

void UClimbComponent::Move(const FInputActionValue& Value)
{
	MovementInput = Value.Get<FVector2D>();
}

void UClimbComponent::JumpPressed()
{
	JumpState = UJumpState::Presse;
}

void UClimbComponent::JumpReleased()
{
	JumpState = UJumpState::Release;
}

void UClimbComponent::CrouchPressed()
{
	if(!bComponentInitalize)
		return;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	FVector CharacterVelociy = ClimbingMovementComponent->Velocity;
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	float CharacterHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();

	if(ClimbingMovementComponent->MovementMode == EMovementMode::MOVE_Walking &&
	   ClimbState == UClimbState::Default)
	{
		FMontagePlayInofo MontagePlayInofo;
		if (!FindMontagePlayInofoByClimbAction(UClimbAction::Walk_Slider, MontagePlayInofo))
			return;

		FVector SliderDownPathTraceStart = GetFootLocation() + FVector::UpVector * 40;
		FVector SliderDownPathTraceEnd = SliderDownPathTraceStart + CharacterForwardVector * 500;
		FHitResult SliderDownPathTraceResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, SliderDownPathTraceStart, SliderDownPathTraceEnd, 35 ,{}, bDrawDebug, FColor::Orange, FColor::Green, 5);
		
		FVector CharacterStandUpLocation;
		if(SliderDownPathTraceResult.bBlockingHit)
		{
			CharacterStandUpLocation = FVector(SliderDownPathTraceResult.Location.X,SliderDownPathTraceResult.Location.Y, CharacterLocation.Z);
		}
		else
		{
			CharacterStandUpLocation = FVector(SliderDownPathTraceEnd.X, SliderDownPathTraceEnd.Y, CharacterLocation.Z);
		}

		FVector CharacterStandUpTraceStart = CharacterStandUpLocation + FVector::UpVector * (CharacterHalfHeight - CharacterRadius);
		FVector CharacterStandUpTraceEnd = CharacterStandUpLocation - FVector::UpVector * (CharacterHalfHeight - CharacterRadius);
		FHitResult CharacterStandUpTraceResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, CharacterStandUpTraceStart, CharacterStandUpTraceEnd, 35, {}, bDrawDebug, FColor::Orange, FColor::Green, 5);

		if(!CharacterStandUpTraceResult.bBlockingHit)
			OwnerCharacter->Crouch();

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				OwnerCharacter->UnCrouch();
			});

		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}
}

void UClimbComponent::CrouchReleased()
{

}

void UClimbComponent::OnModeModeChangeEvent(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	if(!bComponentInitalize)
		return;

	EMovementMode CurrentMoveMode = ClimbingMovementComponent->MovementMode;

	if(CurrentMoveMode == MOVE_Walking &&
	   PrevMovementMode == MOVE_Falling)
	{
		FVector CurrentVelocity = ClimbingMovementComponent->Velocity;
		FVector PrevVelocity = ClimbingMovementComponent->GetLastUpdateVelocity();

		//GEngine->AddOnScreenDebugMessage(1, 5, FColor::Blue, FString::Format(TEXT("XY = {0} Y = {1}"),{ CurrentVelocity.Size2D(), PrevVelocity.Z }));

		if (PrevVelocity.Z >= -1300)
			return;

		bool UseForwarAction = CurrentVelocity.Size2D() > 150;
		bool UseForwarRollAction = CurrentVelocity.Size2D() > 350;

		FMontagePlayInofo MontagePlayInofo;
		if (!FindMontagePlayInofoByClimbAction(UseForwarAction ?
											   (UseForwarRollAction ? UClimbAction::FallToLand_Roll : UClimbAction::FallToLand_Front) :
											   UClimbAction::FallToLand_LandingGround, MontagePlayInofo))
		{
			return;
		}
			

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);
	}
}

// Called every frame
void UClimbComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// ...
	switch (ClimbState)
	{
		case UClimbState::Default:
		{
			DefaultObstacleCheck(DeltaTime);

			HandleJumpInput(DeltaTime);	

			if (ClimbState != UClimbState::Default)
				break;

			HandleDefaultMoveInput();
		}
		break;

		case UClimbState::Climbing:
		{
			ObstacleCheckClimbing(DeltaTime);

			if(ClimbState != UClimbState::Climbing)
				break;

			HandleClimbLerpTransfor(DeltaTime);
			HandleClimbMoveInput();
		}
		break;

		case UClimbState::ClimbingPipe:
		{
			ObstacleCheckClimbPipe(DeltaTime);

			if (ClimbState != UClimbState::ClimbingPipe)
				break;

			HandleClimbPipeLerpTransfor(DeltaTime);
			HandleClimbPipeMoveInput();
		}
		break;

		case UClimbState::Hanging:
		{
			HangingRemapInputVector();
			ObstacleCheckHanging(DeltaTime);
			
			if (ClimbState != UClimbState::Hanging)
				break;
			
			HandleHangingLerpTransfor(DeltaTime);
			HandleHangingMoveInput();
		}
		break;

		case UClimbState::Balance:
		{
			BalanceRemapInputVector();
			ObstacleCheckBalance(DeltaTime);

			if (ClimbState != UClimbState::Balance)
				break;

			HandleBalanceLerpTransfor(DeltaTime);
			HandleBalanceMoveInput();
		}
		break;

		case UClimbState::NarrowSpace:
		{
			ObstacleCheckNarrowSpace(DeltaTime);

			if(ClimbState != UClimbState::NarrowSpace)
				break;

			HandleNarrowSpaceLerpTransfor(DeltaTime);
			HandleNarrowSpaceMoveInput();
		}
		break;

		case UClimbState::LedgeWalkLeft:
		case UClimbState::LedgeWalkRight:
		{
			bool IsRightWalk = (ClimbState == UClimbState::LedgeWalkRight);
			ObstacleCheckLedgeWalk(DeltaTime, IsRightWalk);

			if (ClimbState == UClimbState::LedgeWalkRight || ClimbState == UClimbState::LedgeWalkLeft)
			{
				HandleLedgeWalkLerpTransfor(DeltaTime);
				HandleLedgeWalkMoveInput();
			}
		}
		break;

		default:
		break;
	}

	MovementInput = FVector2D::ZeroVector;
}

void UClimbComponent::HandleJumpInput(float DeltaTime)
{
	switch (JumpState)
	{
	case UJumpState::Presse:
	{
		if (ClimbingAnimInstance->IsAnyMontagePlaying())
			break;

		ObstacleCheckDefaultByInput();
		OwnerCharacter->Jump();
		JumpState = UJumpState::Idle;
	}
	break;

	case UJumpState::Release:
	{
		if (ClimbingAnimInstance->IsAnyMontagePlaying())
			break;

		OwnerCharacter->StopJumping();
		JumpState = UJumpState::Idle;
	}
	break;

	case UJumpState::Idle:
	default:
		break;
	}
}

void UClimbComponent::HandleDefaultMoveInput()
{
	if(ClimbingAnimInstance->IsAnyMontagePlaying())
		return;
	
	const FRotator Rotation = OwnerCharacter->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	// get forward vector
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	// get right vector 
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// add movement 
	OwnerCharacter->AddMovementInput(ForwardDirection, MovementInput.Y);
	OwnerCharacter->AddMovementInput(RightDirection, MovementInput.X);
}

void UClimbComponent::ObstacleCheckDefault(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if(!bComponentInitalize)
		return;

	FVector CurrentVelocity = ClimbingMovementComponent->Velocity;
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	
	if(UKismetMathLibrary::VSizeXY(CurrentVelocity) > 150 ||
	   FVector2D::DotProduct(MovementInput,FVector2D(0,1)) > 0)
	{
		FVector DectionLocation;
		FVector DectionNormal;

		bool DectionResult = ObstacleDetectionDefault(50,100, CurrentVelocity,DectionLocation,DectionNormal);

		//Check obstacle is climbable(steep enough) and Find the obstacle
		if(DectionResult)
		{
			ObstacleLocation = DectionLocation;
			ObstacleNormalDir = DectionNormal;

			bool ObstacleEndDetectionResult = ObstacleEndDetectionUp(150, ObstacleEndLocation);
			
			FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
			float ObstacleToPlayerAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(ObstacleNormalDir.GetSafeNormal(), -CharacterForwardVector));

			if (ObstacleEndDetectionResult &&
				ObstacleToPlayerAngle <= 45)
			{
				SetUpClimbingState();

				return;
			}
		}


		bool HangingDectionResult = HangingObstacleDetectionDefault(100, 200, CurrentVelocity, DectionLocation, DectionNormal);
		if(HangingDectionResult)
		{
			float TraceRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
			float TraceDistance = 2* (OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - TraceRadius);
			FVector HangingCheckStart = DectionLocation + FVector::DownVector * (30 + TraceRadius);
			FVector HangingCheckEnd = HangingCheckStart + FVector::DownVector * TraceDistance;
			
			FHitResult HangingCheckHitResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, HangingCheckStart, HangingCheckEnd, TraceRadius, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

			if(!HangingCheckHitResult.bBlockingHit)
			{	
				FVector FirstDectionHanglocation = DectionLocation;
				FVector LastDectionHanglocation = DectionLocation;

				for (size_t i = 1; i < 11; i++)
				{
					FVector TraceStart = FirstDectionHanglocation + DectionNormal * 10 + i * 2 * FVector::DownVector;
					FVector TraceEnd = TraceStart + DectionNormal * - 10;

					FHitResult TraceResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd,{},bDrawDebug, FColor::Red, FColor::Green);

					if(TraceResult.bBlockingHit)
					{
						LastDectionHanglocation = TraceResult.ImpactPoint;
					}
					else
					{
						break;
					}
				}

				FVector HangTargetLocation = (FirstDectionHanglocation + LastDectionHanglocation) / 2;
				DectionLocation = HangTargetLocation;

				FMontagePlayInofo MontagePlayInofo;
				if (!FindMontagePlayInofoByClimbAction(UClimbAction::Hanging_AttachHanging, MontagePlayInofo))
					return;

				FTransform MotionWarpingTransform;

				FRotator MotionWarpingRotation = UKismetMathLibrary::MakeRotFromX(-DectionNormal);
				MotionWarpingRotation.Pitch = 0;

				MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

				FVector MotionWarpingLocation = HangTargetLocation +
												DectionNormal * MontagePlayInofo.AnimMontageOffSet.X +
												FVector::UpVector * MontagePlayInofo.AnimMontageOffSet.Y;

				MotionWarpingTransform.SetLocation(MotionWarpingLocation);

				if (bDrawDebug)
					DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

				FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
				MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

				ClimbingMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying);

				ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

				FOnMontageBlendingOutStarted BlendingOutDelegate;
				BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
					{
						SetUpHangingState();
						ObstacleDetectionHanging(50, ObstacleLocation, ObstacleNormalDir);
						FindClimbingRotationIdle();
					});

				ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
			}
		}
	}
}

void UClimbComponent::ObstacleCheckDefaultByInput()
{
	if (!bComponentInitalize)
		return;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if(ClimbState != UClimbState::Default)
		return;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FRotator CharacterRotation = OwnerCharacter->GetActorRotation();
	FVector CharacterUpVector = OwnerCharacter->GetActorUpVector();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterFootLocation = GetFootLocation();

	float CharacterCapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float CharacterCapsuleRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();

	bool ObstacleCheckResult = false;

	float SlopeTrace = 50;

	float CheckBoxHalfSize = 10;
	float CheckBoxSize = CheckBoxHalfSize * 2;

	//Slope detection
	FVector SlopeTraceStart = CharacterLocation + CharacterUpVector* CharacterCapsuleHalfHeight* -0.50 + CharacterForwardVector * CharacterCapsuleRadius;
	FVector SlopeTraceEnd = SlopeTraceStart + CharacterUpVector * SlopeTrace + CharacterForwardVector * SlopeTrace;

	FHitResult SlopeTraceHitResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, SlopeTraceStart, SlopeTraceEnd, 10,TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green,5);

	//Pipe detection
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector CharacterRightLocation = CharacterLocation + CharacterRightVector * CharacterCapsuleRadius;
	FVector CharacterLeftLocation = CharacterLocation + CharacterRightVector * CharacterCapsuleRadius * -1;

	if(SlopeTraceHitResult.bBlockingHit)
	{
		FVector PipeLeftTraceStart = FVector(CharacterLeftLocation.X, CharacterLeftLocation.Y, SlopeTraceHitResult.ImpactPoint.Z);
		FVector PipeLeftTraceEnd = PipeLeftTraceStart + CharacterForwardVector * SlopeTrace * 4;

		FVector PipeRightTraceStart = FVector(CharacterRightLocation.X, CharacterRightLocation.Y, SlopeTraceHitResult.ImpactPoint.Z);
		FVector PipeRightTraceEnd = PipeRightTraceStart + CharacterForwardVector * SlopeTrace * 4;

		FHitResult PipeLeftTraceHitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, PipeLeftTraceStart, PipeLeftTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 5);
		FHitResult PipeRightTraceHitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, PipeRightTraceStart, PipeRightTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 5);

		bool IsPipe = false;

		if (PipeLeftTraceHitResult.bBlockingHit &&
			PipeRightTraceHitResult.bBlockingHit &&
			PipeLeftTraceHitResult.Normal == PipeRightTraceHitResult.Normal)
		{
			FVector v1 = (PipeLeftTraceHitResult.Location - SlopeTraceHitResult.ImpactPoint).GetSafeNormal();
			FVector v2 = (PipeRightTraceHitResult.Location - SlopeTraceHitResult.ImpactPoint).GetSafeNormal();
			float AngleV1ToV2 = UKismetMathLibrary::DegAcos(FVector::DotProduct(v1, v2));
			if (AngleV1ToV2 <= 130)
			{
				IsPipe = true;
			}
		}

		if (!IsPipe)
		{
			int TraceStartIndex = 0;
			bool LastDetectionResult = false;

			FVector ObstacleDetectionLocation;
			FVector ObstacleDetectionNormal;
			float ObstacleDetectionHight = 0;
			//Obstacle Hight Check
			for (int i = TraceStartIndex; i < 20; ++i)
			{
				FVector ObstacleTraceStart = SlopeTraceStart + FVector::UpVector * CheckBoxSize * i;
				FVector ObstacleTraceEnd = LastDetectionResult ?
					(FVector(ObstacleDetectionLocation.X, ObstacleDetectionLocation.Y, ObstacleTraceStart.Z) + CharacterForwardVector * CheckBoxHalfSize) :
					(ObstacleTraceStart + CharacterForwardVector * SlopeTrace);

				FRotator BoxRotation = LastDetectionResult ?
					UKismetMathLibrary::MakeRotFromX(-ObstacleDetectionNormal) :
					FRotator(CharacterRotation.Pitch, CharacterRotation.Yaw, 0);

				FHitResult ObstacleHitResult = UTraceBlueprintFunctionLibrary::BoxTrace(OwnerCharacter, ObstacleTraceStart, ObstacleTraceEnd, BoxRotation, CheckBoxHalfSize, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 5);

				if (ObstacleHitResult.bBlockingHit)
				{
					ObstacleDetectionLocation = ObstacleHitResult.ImpactPoint;

					if (!LastDetectionResult)
					{
						ObstacleDetectionNormal = ObstacleHitResult.Normal;

						float AngleWithObstacle = UKismetMathLibrary::DegAcos(FVector::DotProduct(ObstacleDetectionNormal.GetSafeNormal(), -CharacterForwardVector));
						if (AngleWithObstacle >= 45)
						{
							return;
						}
					}

					ObstacleDetectionHight = ObstacleDetectionLocation.Z - CharacterFootLocation.Z;

					LastDetectionResult = true;
				}
				else
				{
					if (LastDetectionResult == true)
					{
						LastDetectionResult = false;

						//Check Character can Climb To Target 
						FVector ObstacleClimbTraceStart = ObstacleDetectionLocation + ObstacleDetectionNormal * (CharacterCapsuleRadius + 25) + FVector::UpVector * CharacterCapsuleHalfHeight;
						FVector ObstacleClimbTraceEnd = ObstacleClimbTraceStart + FVector::DownVector * (ObstacleDetectionLocation.Z - CharacterLocation.Z);

						FHitResult ObstacleClimbHitResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, ObstacleClimbTraceStart, ObstacleClimbTraceEnd, CharacterCapsuleRadius, TArray<AActor*>(), bDrawDebug, FColor::Green, FColor::Red, 5);

						if (ObstacleClimbHitResult.bBlockingHit)
						{
							continue;
						}

						//Check Obstacle is Wall?
						FVector ObstacleWallCheckTraceStart = ObstacleDetectionLocation + ObstacleDetectionNormal * (CharacterCapsuleRadius + 50) * -1 + FVector::UpVector * CharacterCapsuleHalfHeight;
						FVector ObstacleWallCheckTraceEnd = ObstacleDetectionLocation + ObstacleDetectionNormal * (CharacterCapsuleRadius + 50) * -1 + FVector::DownVector * CharacterCapsuleHalfHeight * 0.5;

						FHitResult ObstacleWallCheckHitResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, ObstacleWallCheckTraceStart, ObstacleWallCheckTraceEnd, CharacterCapsuleRadius, TArray<AActor*>(), bDrawDebug, FColor::Orange, FColor::Green, 5);

						bool ObstacleIsWall = !ObstacleWallCheckHitResult.bBlockingHit;

						if (!ObstacleIsWall)
						{
							//Check Character can stand on the Obstacle
							FVector CharacterCanStandTraceStart = ObstacleDetectionLocation + ObstacleDetectionNormal * (CharacterCapsuleRadius) * -1 + FVector::UpVector * (CharacterCapsuleHalfHeight * 2);
							FVector CharacterCanStandTraceEnd = CharacterCanStandTraceStart + FVector::DownVector * (CharacterCapsuleHalfHeight * 2 - CharacterCapsuleRadius - 15);;

							FHitResult CharacterCanStandHitResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, CharacterCanStandTraceStart, CharacterCanStandTraceEnd, CharacterCapsuleRadius, TArray<AActor*>(), bDrawDebug, FColor::Green, FColor::Red, 5);

							if (!CharacterCanStandHitResult.bBlockingHit)
							{
								if (ObstacleDetectionHight >= 60 &&
									ObstacleDetectionHight <= 300)
								{
									bool UseHighAnim = (ObstacleDetectionHight >= 150);

									FMontagePlayInofo MontagePlayInofo;
									if (!FindMontagePlayInofoByClimbAction(UseHighAnim ? UClimbAction::ClimbAction_Climb220 : UClimbAction::ClimbAction_Climb100, MontagePlayInofo))
										continue;

									FTransform MotionWarpingTransform;

									FRotator MotionWarpingRotation = UKismetMathLibrary::MakeRotFromX(-ObstacleDetectionNormal);
									MotionWarpingRotation.Pitch = 0;

									MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

									FVector MotionWarpingLocation = ObstacleDetectionLocation +
										ObstacleDetectionNormal * MontagePlayInofo.AnimMontageOffSet.X +
										CharacterUpVector * MontagePlayInofo.AnimMontageOffSet.Y;

									MotionWarpingTransform.SetLocation(MotionWarpingLocation);

									if (bDrawDebug)
										DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

									FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
									MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

									ClimbingMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying);

									ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

									FOnMontageBlendingOutStarted BlendingOutDelegate;
									BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
										{
											ClimbingMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
										});

									ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);

									break;
								}
							}
						}
						else
						{
							if (ObstacleDetectionHight >= 60 &&
								ObstacleDetectionHight <= 300)
							{
								//Check Character vault the top of Obstacle
								FVector CharacterCanVaultTraceStart = ObstacleDetectionLocation + ObstacleDetectionNormal * (CharacterCapsuleRadius) * -1 + FVector::UpVector * (CharacterCapsuleHalfHeight);
								FVector CharacterCanVaultTraceEnd = CharacterCanVaultTraceStart + FVector::DownVector * (CharacterCapsuleHalfHeight - CharacterCapsuleRadius - 15);

								FHitResult CharacterCanPassHitResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, CharacterCanVaultTraceStart, CharacterCanVaultTraceEnd, CharacterCapsuleRadius, TArray<AActor*>(), bDrawDebug, FColor::Green, FColor::Red, 5);

								if (!CharacterCanPassHitResult.bBlockingHit)
								{
									//Check vault point floor
									FVector CharacterVaultPointFloorTraceStart = ObstacleDetectionLocation + ObstacleDetectionNormal * (CharacterCapsuleRadius + 50) * -1;
									FVector CharacterVaultPointFloorTraceEnd = CharacterVaultPointFloorTraceStart + FVector::DownVector * (ObstacleDetectionHight + CharacterCapsuleHalfHeight * 2);

									FHitResult CharacterVaultPointFloorHitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, CharacterVaultPointFloorTraceStart, CharacterVaultPointFloorTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Yellow, FColor::Red, 5);

									bool CanVault = CharacterVaultPointFloorHitResult.bBlockingHit || CharacterVaultPointFloorHitResult.Distance >= 150;
									bool UseHighAnim = (ObstacleDetectionHight >= 150);

									FMontagePlayInofo MontagePlayInofo;
									if (!FindMontagePlayInofoByClimbAction(CanVault ?
										(UseHighAnim ? UClimbAction::ClimbAction_Vault220 : UClimbAction::ClimbAction_Vault100) :
										(UseHighAnim ? UClimbAction::ClimbAction_VaultTurn220 : UClimbAction::ClimbAction_VaultTurn100),
										MontagePlayInofo))
									{
										continue;
									}

									FTransform MotionWarpingTransform;

									FRotator MotionWarpingRotation = UKismetMathLibrary::MakeRotFromX(-ObstacleDetectionNormal);
									MotionWarpingRotation.Pitch = 0;

									MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

									FVector MotionWarpingLocation = ObstacleDetectionLocation +
										ObstacleDetectionNormal * MontagePlayInofo.AnimMontageOffSet.X +
										CharacterUpVector * MontagePlayInofo.AnimMontageOffSet.Y;

									MotionWarpingTransform.SetLocation(MotionWarpingLocation);

									if (bDrawDebug)
										DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

									FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
									MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

									if (!CanVault)
									{
										FTransform MotionWarpingStartTransform;

										FRotator MotionWarpingStartRotation = UKismetMathLibrary::MakeRotFromX(-ObstacleDetectionNormal);
										MotionWarpingStartRotation.Pitch = 0;

										MotionWarpingStartTransform.SetRotation(MotionWarpingStartRotation.Quaternion());

										FMotionWarpingTarget MotionWarpingStartTarget = FMotionWarpingTarget("ClimbRotation", MotionWarpingStartTransform);

										MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingStartTarget);

										FVector CharacterVaultEndPointTraceStart = ObstacleDetectionLocation + ObstacleDetectionNormal * (CharacterCapsuleRadius) * -1 + FVector::DownVector * (CharacterCapsuleHalfHeight + CharacterCapsuleRadius);
										FVector CharacterVaultEndPointTraceEnd = CharacterVaultEndPointTraceStart + ObstacleDetectionNormal * (CharacterCapsuleRadius + 10);

										FHitResult  CharacterVaultEndPointHitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, CharacterVaultEndPointTraceStart, CharacterVaultEndPointTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Yellow, FColor::Red, 5);

										FTransform MotionWarpingEndTransform;
										FRotator MotionWarpingEndRotation = UKismetMathLibrary::MakeRotFromX(-CharacterVaultEndPointHitResult.Normal);
										MotionWarpingEndRotation.Pitch = 0;

										MotionWarpingEndTransform.SetRotation(MotionWarpingEndRotation.Quaternion());

										FVector MotionWarpingEndLocation = CharacterVaultEndPointHitResult.Location + CharacterVaultEndPointHitResult.Normal * CharacterCapsuleRadius;
										MotionWarpingEndTransform.SetLocation(MotionWarpingEndLocation);

										if (bDrawDebug)
											DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingEndLocation, 10, 32, FColor::Cyan, false, 3);

										FMotionWarpingTarget MotionWarpingEndTarget = FMotionWarpingTarget("ClimbEndTarget", MotionWarpingEndTransform);
										MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingEndTarget);

										SetUpClimbingState();
									}

									ClimbingMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying);

									ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

									if (CanVault)
									{
										FOnMontageBlendingOutStarted BlendingOutDelegate;
										BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
											{
												ClimbingMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
											});

										ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
									}
									else
									{
										FOnMontageBlendingOutStarted BlendingOutDelegate;
										BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
											{
												ObstacleDetectionClimbing(150, ObstacleLocation, ObstacleNormalDir);
												FindClimbingRotationIdle();
											});

										ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
									}
									break;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			FVector PipeLocation = SlopeTraceHitResult.GetActor()->GetActorLocation();
			FVector TraceDirection = PipeLeftTraceHitResult.Normal;

			FVector PipeTraceEnd = FVector(PipeLocation.X, PipeLocation.Y, SlopeTraceHitResult.ImpactPoint.Z);
			FVector PipeTraceStart = PipeTraceEnd + TraceDirection * 50;

			FHitResult PipeTraceHitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, PipeTraceStart, PipeTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 5);

			if (!PipeTraceHitResult.bBlockingHit)
				return;

			FMontagePlayInofo MontagePlayInofo;
			if (!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbPipeAction_StartClimbPipe, MontagePlayInofo))
				return;

			FTransform MotionWarpingTransform;

			FRotator MotionWarpingRotation = UKismetMathLibrary::MakeRotFromX(-PipeTraceHitResult.Normal);

			MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

			FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
			MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

			FTransform MotionWarpingEndTransform;

			FVector MotionWarpingLocation = PipeTraceHitResult.Location +
				PipeTraceHitResult.Normal * MontagePlayInofo.AnimMontageOffSet.X +
				FVector::UpVector * MontagePlayInofo.AnimMontageOffSet.Y;
			MotionWarpingEndTransform.SetLocation(MotionWarpingLocation);

			if (bDrawDebug)
				DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

			FMotionWarpingTarget MotionWarpingEndTarget = FMotionWarpingTarget("ClimbEndTarget", MotionWarpingEndTransform);
			MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingEndTarget);

			SetUpClimbingPipeState();

			ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

			FOnMontageBlendingOutStarted BlendingOutDelegate;
			BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
				{
					ObstacleDetectionClimbing(150, ObstacleLocation, ObstacleNormalDir);
					FindClimbingRotationIdle();
				});

			ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);

		}
	}
}

bool UClimbComponent::ObstacleDetectionDefault(float MinDistance, float MaxDistance, const FVector& Velocity, FVector& Location, FVector& Normal)
{
	float Distance = FMath::GetMappedRangeValueClamped(FVector2f(0, MaxDistance / 5), FVector2f(MinDistance, MaxDistance), Velocity.Length());

	FVector TraceStart = OwnerCharacter->GetActorLocation();
	FVector TraceEnd = OwnerCharacter->GetActorForwardVector() * Distance + TraceStart;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if(HitResult.bBlockingHit)
	{
		Location = HitResult.Location;
		Normal = HitResult.Normal;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::HangingObstacleDetectionDefault(float MinDistance, float MaxDistance, const FVector& Velocity, FVector& Location, FVector& Normal)
{
	float CharacterHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float Distance = FMath::GetMappedRangeValueClamped(FVector2f(0, MaxDistance / 5), FVector2f(MinDistance, MaxDistance), Velocity.Size2D());
	float Height = FMath::GetMappedRangeValueClamped(FVector2f(-5 * CharacterHalfHeight, 5 * CharacterHalfHeight), FVector2f(-(CharacterHalfHeight - 10), (CharacterHalfHeight - 10)), Velocity.Z);

	//FVector TraceStart = GetFootLocation() + FVector::UpVector * 10;
	FVector TraceStart = OwnerCharacter->GetActorLocation() + FVector::UpVector * Height;
	FVector TraceEnd = OwnerCharacter->GetActorForwardVector() * Distance + TraceStart;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, TraceStart, TraceEnd, 5,{OwnerCharacter}, bDrawDebug, FColor::Red, FColor::Green);
	//FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, { OwnerCharacter }, bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.ImpactPoint;
		
		Normal = FVector(HitResult.Normal.X, HitResult.Normal.Y,0).GetSafeNormal();;
	}

	return HitResult.bBlockingHit;
}

void UClimbComponent::ObstacleCheckClimbing(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if (!bComponentInitalize)
		return;

	FVector DectionLocation;
	FVector DectionNormal;

	bool DetectionResult = ObstacleDetectionClimbing(150, DectionLocation, DectionNormal);

	if(DetectionResult)
	{
		ObstacleNormalDir = FMath::VInterpTo(ObstacleNormalDir, DectionNormal, DeltaTime, 10).GetSafeNormal();
		ObstacleLocation = DectionLocation;

		bool VerticalCheck = (ClimbUpCheck(DeltaTime) || ClimbDownCheck(DeltaTime));
		bool HorizontalCheck = (ClimbRightCheck(DeltaTime) || ClimbLeftCheck(DeltaTime));

		if(VerticalCheck || HorizontalCheck)
		{
			return;
		}
		else
		{
			FindClimbingRotationIdle();
		}
	}
	else
	{
		SetUpDefaultState();
	}
}

void UClimbComponent::ObstacleCheckClimbPipe(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if (!bComponentInitalize)
		return;

	FVector DectionLocation;
	FVector DectionNormal;

	bool DetectionResult = ObstacleDetectionClimbing(150, DectionLocation, DectionNormal);

	if (DetectionResult)
	{
		ObstacleNormalDir = FMath::VInterpTo(ObstacleNormalDir, DectionNormal, DeltaTime, 10).GetSafeNormal();
		ObstacleLocation = DectionLocation;

		bool VerticalCheck = (ClimbPipeUpCheck(DeltaTime) || ClimbPipeDownCheck(DeltaTime));
		bool HorizontalCheck = (ClimbPipeRightCheck(DeltaTime) || ClimbPipeLeftCheck(DeltaTime));

		if(VerticalCheck || HorizontalCheck)
		{
			return;
		}
		else
		{
			FindClimbingRotationIdle();
		}
	}
	else
	{
		SetUpDefaultState();
	}
}

void UClimbComponent::ObstacleCheckHanging(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if (!bComponentInitalize)
		return;

	FVector DectionLocation;
	FVector DectionNormal;

	bool DetectionResult = ObstacleDetectionHanging(50, DectionLocation, DectionNormal);

	if(DetectionResult)
	{
		ObstacleNormalDir = FMath::VInterpTo(ObstacleNormalDir, DectionNormal, DeltaTime, 10).GetSafeNormal();
		ObstacleLocation = DectionLocation;

		bool VerticalCheck = (HangingUpCheck(DeltaTime) || HangingDownCheck(DeltaTime));
		bool HorizontalCheck = (HangingLeftCheck(DeltaTime) || HangingRightCheck(DeltaTime));

		if (VerticalCheck || HorizontalCheck)
		{
			return;
		}
		else
		{
			FindClimbingRotationIdle();
		}
	}
	else
	{
		SetUpDefaultState();
	}
}

void UClimbComponent::ObstacleCheckBalance(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if (!bComponentInitalize)
		return;

	FVector DectionLocation;
	FVector DectionNormal;

	bool DetectionResult = FloorDectectionBalance(DectionLocation,DectionNormal);

	if(DetectionResult)
	{
		FloorNormalDir = FMath::VInterpTo(FloorNormalDir, DectionNormal, DeltaTime, 10).GetSafeNormal();
		FloorLocation = DectionLocation;

		bool VerticalCheck = (BalanceUpCheck(DeltaTime) || BalanceDownCheck(DeltaTime));

		if(VerticalCheck)
		{
			return;
		}
		else
		{
			FindBalanceRotationIdle();
		}
	}
	else
	{
		SetUpDefaultState();
	}
}

void UClimbComponent::ObstacleCheckNarrowSpace(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if (!bComponentInitalize)
		return;

	FVector DectionLocation;
	FVector DectionNormal;

	bool DetectionResult = ObstacleDetectionNarrowSpace(DectionLocation,DectionNormal);

	if(DetectionResult)
	{
		ObstacleLocation = DectionLocation;
		ObstacleNormalDir = FMath::VInterpTo(ObstacleNormalDir, DectionNormal, DeltaTime, 10).GetSafeNormal();

		bool VerticalCheck = (NarrowSpaceUpCheck(DeltaTime) || NarrowSpaceDownCheck(DeltaTime));

		if(VerticalCheck)
		{
			return;
		}
		else
		{
			FindNarrowSpaceRotationIdle();
		}
	}
	else
	{
		SetUpDefaultState();
	}
}

void UClimbComponent::ObstacleCheckLedgeWalk(float DeltaTime, bool IsRightWalk)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if (!bComponentInitalize)
		return;

	FVector DectionLocation;
	FVector DectionNormal;

	if(bool DetectionResult = ObstacleDetectionLedgeWalk(IsRightWalk, DectionLocation, DectionNormal))
	{
		ObstacleLocation = DectionLocation;
		ObstacleNormalDir = FMath::VInterpTo(ObstacleNormalDir, DectionNormal, DeltaTime, 10).GetSafeNormal();

		bool VerticalCheck = (LedgeWalkUpCheck(DeltaTime, IsRightWalk) || LedgeWalkDownCheck(DeltaTime, IsRightWalk));

		if(!VerticalCheck)
		{
			FindLedgeWalkRotationIdle(IsRightWalk);
		}
	}
	else
	{
		SetUpDefaultState();
	}
}

bool UClimbComponent::ObstacleDetectionClimbing(float Distance, FVector& Location, FVector& Normal)
{
	FVector TreceStart = OwnerCharacter->GetActorLocation();
	FVector TraceEnd = OwnerCharacter->GetActorForwardVector() * Distance + TreceStart;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TreceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Yellow,FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.Location;
		Normal = HitResult.Normal;
	}
	else
	{
		Location = FVector::ZeroVector;
		Normal = FVector::ZeroVector;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::ObstacleDetectionHanging(float Distance, FVector& Location, FVector& Normal)
{
	FVector TreceStart = GetTopLocation() + FVector::UpVector * GHangingTraceOffsetZ + OwnerCharacter->GetActorForwardVector() * -10;
	FVector TraceEnd = OwnerCharacter->GetActorForwardVector() * Distance + TreceStart;

	//FHitResult HitResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, TreceStart, TraceEnd, 5,TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TreceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.ImpactPoint;
		Normal = HitResult.ImpactNormal;
	}
	else
	{
		Location = FVector::ZeroVector;
		Normal = FVector::ZeroVector;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::FloorDectectionBalance(FVector& Location, FVector& Normal)
{
	FVector TraceStart = GetFootLocation() + FVector::UpVector * 20;
	FVector TraceEnd = TraceStart + FVector::DownVector * 30;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, TraceStart, TraceEnd, 10,TArray<AActor*>(), bDrawDebug, FColor::Yellow, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.Location;
		Normal = HitResult.Normal;
	}
	else
	{
		Location = FVector::ZeroVector;
		Normal = FVector::ZeroVector;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::ObstacleDetectionNarrowSpace(FVector& Location, FVector& Normal)
{
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();

	FVector TraceStart = OwnerCharacter->GetActorLocation();
	FVector TraceEnd = TraceStart + CharacterRightVector * CharacterRadius;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.ImpactPoint;
		Normal = HitResult.ImpactNormal;
	}
	else
	{
		Location = FVector::ZeroVector;
		Normal = FVector::ZeroVector;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::ObstacleDetectionLedgeWalk(bool IsRightWalk, FVector& Location, FVector& Normal)
{
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector TraceVector = IsRightWalk ? CharacterRightVector : -CharacterRightVector;
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();

	FVector TraceStart = OwnerCharacter->GetActorLocation();
	FVector TraceEnd = TraceStart + TraceVector * CharacterRadius;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.ImpactPoint;
		Normal = HitResult.ImpactNormal;
	}
	else
	{
		Location = FVector::ZeroVector;
		Normal = FVector::ZeroVector;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::ClimbUpCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if(FVector2D::DotProduct(MovementInput , FVector2D(0,1)) > 0)
	{
		bool DetectionResult = ObstacleEndDetectionUp(150, ObstacleEndLocation);

		if(!DetectionResult)
		{
			bool FindAction = ClimbUpActionCheck();
			if(!FindAction)
			{
				FindAction = ClimbUpJumpCheck();
			}
		}

		if(DetectionResult)
		{
			FindClimbingRotationUp();
		}
		else
		{
			FindClimbingRotationIdle();
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool UClimbComponent::ClimbRightCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(1, 0)) > 0)
	{
		bool DetectionResult = ObstacleEndDetectionRight(150, ObstacleEndLocation);

		if(!DetectionResult)
		{
			bool FindAction = false;

			FindAction = ClimbRightJumpCheck();

			if(!FindAction)
			{
				FindAction = ClimbRightCornerInnerCheck();
			}
		}
		else
		{
			bool FindAction = false;

			FindAction = ClimbRightCornerOuterCheck();
		}

		if(DetectionResult)
		{
			FindClimbingRotationRight();
		}
		else
		{
			FindClimbingRotationIdle();
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool UClimbComponent::ClimbLeftCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(-1, 0)) > 0)
	{
		bool DetectionResult = ObstacleEndDetectionLeft(150, ObstacleEndLocation);

		if(!DetectionResult)
		{	
			bool FindAction = false;

			FindAction = ClimbLeftJumpCheck();

			if(!FindAction)
			{
				FindAction = ClimbLeftCornerInnerCheck();
			}
		}
		else
		{
			bool FindAction = false;

			FindAction = ClimbLeftCornerOuterCheck();
		}

		if(DetectionResult)
		{
			FindClimbingRotationLeft();
		}
		else
		{
			FindClimbingRotationIdle();
		}

		return true;
	}

	return false;
}

bool UClimbComponent::ClimbDownCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(0, -1)) > 0)
	{
		bool DetectionResult = ObstacleEndDetectionDown(150, ObstacleEndLocation);

		if (!DetectionResult)
		{
			bool FindAction = ClimbDownJumpCheck();

			if (!FindAction)
			{
				FindAction = ClimbingToHangingCheck();
			}
		}

		if(DetectionResult)
		{
			bool FindAction = ClimbLandingCheck();

			if(!FindAction)
			{
				FindClimbingRotationDown();
			}
		}
		else
		{
			FindClimbingRotationIdle();
		}

		return true;
	}
	return false;
}

bool UClimbComponent::ClimbPipeUpCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(0, 1)) > 0)
	{
		bool DetectionResult = ObstacleEndDetectionUp(150, ObstacleEndLocation);

		if(!DetectionResult)
		{
			DetectionResult = ClimbPipeLandUpCheck();
		}

		if (DetectionResult)
		{
			FindClimbingRotationUp();
		}
		else
		{
			FindClimbingRotationIdle();
		}

		return true;
	}

	return false;
}

bool UClimbComponent::ClimbPipeDownCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(0, -1)) > 0)
	{
		bool DetectionResult = ObstacleEndDetectionDown(150, ObstacleEndLocation);

		if (DetectionResult)
		{
			bool FindAction = ClimbPipeLandDownCheck();

			if (!FindAction)
			{
				FindClimbingRotationDown();
			}
		}
		else
		{
			FindClimbingRotationIdle();
		}

		return true;
	}

	return false;
}

bool UClimbComponent::ClimbPipeRightCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(1, 0)) > 0)
	{
		bool FindAction = ClimbPipeRightJumpCheck();

		if(!FindAction)
		{
			FindClimbingRotationIdle();
		}

		return true;
	}

	return false;
}

bool UClimbComponent::ClimbPipeLeftCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(-1, 0)) > 0)
	{
		bool FindAction = ClimbPipeLeftJumpCheck();

		if (!FindAction)
		{
			FindClimbingRotationIdle();
		}

		return true;
	}

	return false;
}

bool UClimbComponent::HangingUpCheck(float DelaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(0, 1)) > 0)
	{
		bool FindAction = HangingClimbUpCheck();

		if(!FindAction)
		{
			FindAction = HangingTurnCheck();
		}

		if(!FindAction)
		{
			FindClimbingRotationIdle();
		}
		return true;
	}

	return false;
}

bool UClimbComponent::HangingDownCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(0, -1)) > 0)
	{
		bool FindAction = HangingDropCheck();

		if (!FindAction)
		{
			FindClimbingRotationIdle();
		}
		return true;
	}

	return false;
}

bool UClimbComponent::HangingRightCheck(float DetalTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(1, 0)) > 0)
	{
		bool DetectionResult = HangingEndDetectionRight(100, ObstacleEndLocation);

		if(!DetectionResult)
		{
			bool FindAction = HangingRightCornerInnerCheck();
		}
		else
		{
			bool FindAction = HangingRightCornerOuterCheck();
		}

		if (DetectionResult)
		{
			FindClimbingRotationRight();
		}
		else
		{
			FindClimbingRotationIdle();
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool UClimbComponent::HangingLeftCheck(float DetalTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(-1, 0)) > 0)
	{
		bool DetectionResult = HangingEndDetectionLeft(100, ObstacleEndLocation);

		if (!DetectionResult)
		{
			bool FindAction = HangingLeftCornerInnerCheck();
		}
		else
		{
			bool FindAction = HangingLeftCornerOuterCheck();
		}

		if (DetectionResult)
		{
			FindClimbingRotationLeft();
		}
		else
		{
			FindClimbingRotationIdle();
		}

		return true;
	}
	else
	{
		return false;
	}
}

bool UClimbComponent::BalanceUpCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(0, 1)) > 0)
	{
		bool DetectionResult = BalanceEndDectionUp(FloorEndLocation, FloorEndNormalDir);

		if(DetectionResult)
		{
			bool FindAction = BalanceUpToWalkCheck();
		}
		else
		{
			if(FloorEndLocation != FVector::ZeroVector ||
			   FloorEndNormalDir != FVector::ZeroVector)
			{
				FindBalanceRotationIdle(FloorEndLocation == FVector::ZeroVector);
			}
			else
			{
				FindBalanceRotationIdle();
			}
			
		}

		return true;
	}

	return false;
}

bool UClimbComponent::BalanceDownCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(0, -1)) > 0)
	{
		bool DetectionResult = BalanceEndDectionDown(FloorEndLocation, FloorEndNormalDir);

		if (DetectionResult)
		{
			bool FindAction = BalanceDownToWalkCheck();

			if(!FindAction)
			{
				FindAction = BalanceTurnBackCheck();
			}
		}
		else
		{
			if (FloorEndLocation != FVector::ZeroVector ||
				FloorEndNormalDir != FVector::ZeroVector)
			{
				FindBalanceRotationIdle(FloorEndLocation == FVector::ZeroVector);
			}
			else
			{
				FindBalanceRotationIdle();
			}
		}
		return true;
	}


	return false;
}

bool UClimbComponent::NarrowSpaceUpCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(0, 1)) > 0)
	{
		FVector DectionEndNoramlDir;
		bool DetectionResult = NarrowSpaceEndDectionUp(ObstacleEndLocation, DectionEndNoramlDir);

		if(!DetectionResult)
		{
			NarrowSpaceUpToWalkCheck();
		}

		FindNarrowSpaceRotationIdle();

		return true;
	}

	return false;
}

bool UClimbComponent::NarrowSpaceDownCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(0, -1)) > 0)
	{
		FVector DectionEndNoramlDir;
		bool DetectionResult = NarrowSpaceEndDectionDown(ObstacleEndLocation, DectionEndNoramlDir);

		if(!DetectionResult)
		{
			NarrowSpaceDownToWalkCheck();
		}

		FindNarrowSpaceRotationIdle();

		return true;
	}
	return false;
}

bool UClimbComponent::LedgeWalkUpCheck(float DeltaTime, bool IsRightWalk)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(0, 1)) > 0)
	{
		FVector DectionEndNoramlDir;
		bool DetectionResult = LedgeWalkEndDectionUp(IsRightWalk, ObstacleEndLocation, DectionEndNoramlDir);

		if(!DetectionResult)
		{
			bool FindAction = LedgeWalkUpInsideCornerCheck(IsRightWalk);
			if(!FindAction)
			{
				FindAction = LedgeWalkUpOutwardCornerCheck(IsRightWalk);
			}
		}
		else
		{
			LedgeWalkUpLedgeWalkToWalkCheck(IsRightWalk);
		}

		FindLedgeWalkRotationIdle(IsRightWalk);

		return true;
	}


	return false;
}

bool UClimbComponent::LedgeWalkDownCheck(float DeltaTime, bool IsRightWalk)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return false;

	if (FVector2D::DotProduct(MovementInput, FVector2D(0, -1)) > 0)
	{
		FVector DectionEndNoramlDir;
		bool DetectionResult = LedgeWalkEndDectionDown(IsRightWalk, ObstacleEndLocation, DectionEndNoramlDir);

		if(!DetectionResult)
		{
			bool FindAction = LedgeWalkDownInsideCornerCheck(IsRightWalk);
			if(!FindAction)
			{
				FindAction = LedgeWalkDownOutwardCornerCheck(IsRightWalk);
			}
		}
		else
		{
			LedgeWalkDownLedgeWalkToWalkCheck(IsRightWalk);
		}

		FindLedgeWalkRotationIdle(IsRightWalk);

		return true;
	}

	return false;
}

bool UClimbComponent::ClimbRightJumpCheck()
{
	bool CanRightJump = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanRightJump;
	
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();

	TArray<FVector2D> TraceDataArray;
	TraceDataArray.Add(FVector2D(200, 150));
	TraceDataArray.Add(FVector2D(350, 150));

	TArray<UClimbAction> ClimbActionList = {
	UClimbAction::ClimbingAction_RightJump,
	UClimbAction::ClimbingAction_SuperRightJump
	};

	for (int i = 0; i < TraceDataArray.Num(); i++)
	{
		FVector2D& TraceData = TraceDataArray[i];

		FVector RightJumpTraceStart = CharacterLocation + CharacterRightVector * TraceData.X;
		FVector RightJumpTraceEnd = RightJumpTraceStart + CharacterForwardVector * TraceData.Y;

		FHitResult RightJumpHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, RightJumpTraceStart, RightJumpTraceEnd, 10, TArray<AActor*>(), bDrawDebug, FColor::Blue, FColor::Green, 3);

		float RightJumpAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(RightJumpHit.ImpactNormal.GetSafeNormal(), -CharacterForwardVector));

		if (RightJumpHit.bBlockingHit &&
			RightJumpAngle <= 5)
		{
			FVector RightJumpPlayerTraceStart = CharacterLocation;
			FVector RightJumpPlayerTraceEnd = RightJumpTraceStart;

			FHitResult RightJumpPlayerHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, RightJumpPlayerTraceStart, RightJumpPlayerTraceEnd, 10, TArray<AActor*>(), bDrawDebug, FColor::Yellow, FColor::Green, 3);

			if (!RightJumpPlayerHit.bBlockingHit)
			{
				FMontagePlayInofo MontagePlayInofo;

				if (!FindMontagePlayInofoByClimbAction(ClimbActionList[i], MontagePlayInofo))
				{
					continue;
				}

				FTransform MotionWarpingTransform;

				FRotator MotionWarpingRotation;
				MotionWarpingRotation.Yaw = OwnerCharacter->GetActorRotation().Yaw;

				MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

				FVector MotionWarpingLocation = RightJumpHit.ImpactPoint + 
												CharacterRightVector * MontagePlayInofo.AnimMontageOffSet.X +
												RightJumpHit.ImpactNormal * MontagePlayInofo.AnimMontageOffSet.Y;

				MotionWarpingTransform.SetLocation(MotionWarpingLocation);

				if (bDrawDebug)
					DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

				FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
				MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

				ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

				CanRightJump = true;
				break;
			}
		}
	}

	return CanRightJump;
}

bool UClimbComponent::ClimbLeftJumpCheck()
{
	bool CanLeftJump = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanLeftJump;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();

	TArray<FVector2D> TraceDataArray;
	TraceDataArray.Add(FVector2D(-200, 150));
	TraceDataArray.Add(FVector2D(-350, 150));

	TArray<UClimbAction> ClimbActionList = {
		UClimbAction::ClimbingAction_LeftJump,
		UClimbAction::ClimbingAction_SuperLeftJump
	};

	for (int i = 0; i < TraceDataArray.Num(); i++)
	{
		FVector2D& TraceData = TraceDataArray[i];

		FVector LeftJumpTraceStart = CharacterLocation + CharacterRightVector * TraceData.X;
		FVector LeftJumpTraceEnd = LeftJumpTraceStart + CharacterForwardVector * TraceData.Y;

		FHitResult LeftJumpHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, LeftJumpTraceStart, LeftJumpTraceEnd, 10, TArray<AActor*>(), bDrawDebug, FColor::Blue, FColor::Green, 3);

		float LeftJumpAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(LeftJumpHit.ImpactNormal.GetSafeNormal(), -CharacterForwardVector));

		if (LeftJumpHit.bBlockingHit &&
			LeftJumpAngle <= 5)
		{
			FVector LeftJumpPlayerTraceStart = CharacterLocation;
			FVector LeftJumpPlayerTraceEnd = LeftJumpTraceStart;

			FHitResult LeftJumpPlayerHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, LeftJumpPlayerTraceStart, LeftJumpPlayerTraceEnd, 10, TArray<AActor*>(), bDrawDebug, FColor::Yellow, FColor::Green, 3);

			if (!LeftJumpPlayerHit.bBlockingHit)
			{
				FMontagePlayInofo MontagePlayInofo;// = MontagePlayInfoArray[i];

				if(!FindMontagePlayInofoByClimbAction(ClimbActionList[i], MontagePlayInofo))
				{
					continue;
				}

				FTransform MotionWarpingTransform;

				FRotator MotionWarpingRotation;
				MotionWarpingRotation.Yaw = OwnerCharacter->GetActorRotation().Yaw;

				MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

				FVector MotionWarpingLocation = LeftJumpHit.ImpactPoint +
												CharacterRightVector * MontagePlayInofo.AnimMontageOffSet.X +
												LeftJumpHit.ImpactNormal * MontagePlayInofo.AnimMontageOffSet.Y;


				MotionWarpingTransform.SetLocation(MotionWarpingLocation);

				if (bDrawDebug)
					DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

				FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
				MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

				ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

				CanLeftJump = true;
				break;
			}
		}
	}

	return CanLeftJump;
}

bool UClimbComponent::ClimbDownJumpCheck()
{
	bool FindClimbDownJump = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindClimbDownJump;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterUpVector = OwnerCharacter->GetActorUpVector();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();

	FVector ClimbDownJumpTraceStart = GetFootLocation() + CharacterUpVector * -250;
	FVector ClimbDownJumpTraceEnd = ClimbDownJumpTraceStart + CharacterForwardVector * 150;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, ClimbDownJumpTraceStart, ClimbDownJumpTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 3);

	if(HitResult.bBlockingHit)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbingAction_JumpDown, MontagePlayInofo))
			return false;

		FindClimbDownJump = true;
		
		FTransform MotionWarpingTransform;

		FRotator MotionWarpingRotation;
		MotionWarpingRotation.Yaw = OwnerCharacter->GetActorRotation().Yaw;

		MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

		FVector MotionWarpingLocation = HitResult.ImpactPoint + 
										ObstacleNormalDir * MontagePlayInofo.AnimMontageOffSet.X +
										CharacterUpVector * MontagePlayInofo.AnimMontageOffSet.Y;
		

		MotionWarpingTransform.SetLocation(MotionWarpingLocation);

		if (bDrawDebug)
			DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

		FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
		MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);
	}

	return FindClimbDownJump;
}

bool UClimbComponent::ClimbUpJumpCheck()
{
	bool FindClimbUpJump = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindClimbUpJump;

	FVector JumpUpCheckStart = GetTopLocation() + OwnerCharacter->GetActorUpVector() * 220;
	FVector JumpUpCheckEnd = JumpUpCheckStart + OwnerCharacter->GetActorForwardVector() * 150;

	FHitResult JumpUpCheckHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, JumpUpCheckStart, JumpUpCheckEnd, 10, TArray<AActor*>(), bDrawDebug, FColor::Blue, FColor::Green, 3);

	FVector JumpUpPlayerCheckStart = GetTopLocation();
	FVector JumpUpPlayerCheckEnd = JumpUpCheckStart;

	FHitResult JumpUpPlayerCheckHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, JumpUpPlayerCheckStart, JumpUpPlayerCheckEnd, 10, TArray<AActor*>(), bDrawDebug, FColor::Cyan, FColor::Green, 3);


	if (JumpUpCheckHit.bBlockingHit &&
		!JumpUpPlayerCheckHit.bBlockingHit)
	{
		FMontagePlayInofo MontagePlayInofo;

		if(!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbingAction_JumpUp, MontagePlayInofo))
			return false;
		
		FindClimbUpJump = true;

		FTransform MotionWarpingTransform;

		FRotator MotionWarpingRotation;
		MotionWarpingRotation.Yaw = OwnerCharacter->GetActorRotation().Yaw;

		MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

		FVector MotionWarpingLocation = JumpUpCheckHit.ImpactPoint + ObstacleNormalDir * MontagePlayInofo.AnimMontageOffSet.X;
		MotionWarpingLocation.Z = MotionWarpingLocation.Z -
								  MontagePlayInofo.AnimMontageOffSet.Y - 
								  OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 
								  OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();

		MotionWarpingTransform.SetLocation(MotionWarpingLocation);

		if (bDrawDebug)
			DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

		FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
		MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);
	}

	return FindClimbUpJump;
}

bool UClimbComponent::ClimbRightCornerInnerCheck()
{
	bool CanRightCornerInner = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanRightCornerInner;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();

	FVector RightCornerInnerTraceEnd = CharacterLocation + CharacterForwardVector * 85;
	FVector RightCornerInnerTraceStart = RightCornerInnerTraceEnd + CharacterRightVector * 85;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, RightCornerInnerTraceStart, RightCornerInnerTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green,3);

	float CornerInnerAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(HitResult.ImpactNormal.GetSafeNormal(), CharacterRightVector));
	if(HitResult.bBlockingHit &&
		CornerInnerAngle <= 5)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbingAction_InnerRight, MontagePlayInofo))
			return false;

		CanRightCornerInner = true;
		
		FVector AdjustLocation = HitResult.ImpactPoint +
								 CharacterRightVector * -55 -
								 CharacterForwardVector * (50 + OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius());

		OwnerCharacter->SetActorLocation(AdjustLocation,true);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				ObstacleDetectionClimbing(150, ObstacleLocation, ObstacleNormalDir);
				FindClimbingRotationIdle();
			});

		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return CanRightCornerInner;
}

bool UClimbComponent::ClimbLeftCornerInnerCheck()
{
	bool CanLeftCornerInner = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanLeftCornerInner;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();

	FVector LeftCornerInnerTraceEnd = CharacterLocation + CharacterForwardVector * 85;
	FVector LeftCornerInnerTraceStart = LeftCornerInnerTraceEnd + CharacterRightVector * -85;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, LeftCornerInnerTraceStart, LeftCornerInnerTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 3);
	
	float CornerInnerAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(HitResult.ImpactNormal.GetSafeNormal(), CharacterRightVector * -1));

	if (HitResult.bBlockingHit &&
		CornerInnerAngle <= 5)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbingAction_InnerLeft, MontagePlayInofo))
			return false;

		CanLeftCornerInner = true;

		FVector AdjustLocation = HitResult.ImpactPoint + CharacterRightVector * 55 -
								 CharacterForwardVector * (50 + OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius());

		OwnerCharacter->SetActorLocation(AdjustLocation, true);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				ObstacleDetectionClimbing(150, ObstacleLocation, ObstacleNormalDir);
				FindClimbingRotationIdle();
			});

		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return CanLeftCornerInner;
}

bool UClimbComponent::HangingRightCornerInnerCheck()
{
	bool CanRightCornerInner = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanRightCornerInner;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();

	FVector RightCornerInnerTraceEnd = GetTopLocation() +
									   CharacterForwardVector * 50 + 
									   FVector::UpVector * GHangingTraceOffsetZ;
	FVector RightCornerInnerTraceStart = RightCornerInnerTraceEnd + CharacterRightVector * 85;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, RightCornerInnerTraceStart, RightCornerInnerTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 3);

	float CornerInnerAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(HitResult.ImpactNormal.GetSafeNormal(), CharacterRightVector));
	if (HitResult.bBlockingHit &&
		CornerInnerAngle <= 5)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::Hanging_InnerRight, MontagePlayInofo))
			return false;

		CanRightCornerInner = true;

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				ObstacleDetectionHanging(50, ObstacleLocation, ObstacleNormalDir);
				FindClimbingRotationIdle();
			});

		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return CanRightCornerInner;
}

bool UClimbComponent::HangingLeftCornerInnerCheck()
{
	bool CanLeftCornerInner = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanLeftCornerInner;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterLeftVector = -OwnerCharacter->GetActorRightVector();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();

	FVector RightCornerInnerTraceEnd = GetTopLocation() +
									   CharacterForwardVector * 50 +
									   FVector::UpVector * GHangingTraceOffsetZ;
	FVector RightCornerInnerTraceStart = RightCornerInnerTraceEnd + CharacterLeftVector * 85;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, RightCornerInnerTraceStart, RightCornerInnerTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 3);

	float CornerInnerAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(HitResult.ImpactNormal.GetSafeNormal(), CharacterLeftVector));
	if (HitResult.bBlockingHit &&
		CornerInnerAngle <= 5)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::Hanging_InnerLeft, MontagePlayInofo))
			return false;

		CanLeftCornerInner = true;

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				ObstacleDetectionHanging(50, ObstacleLocation, ObstacleNormalDir);
				FindClimbingRotationIdle();
			});

		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return CanLeftCornerInner;
}

bool UClimbComponent::ClimbRightCornerOuterCheck()
{
	bool FindClimbRightCornerOuter = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindClimbRightCornerOuter;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	float CharacterCapsuleRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();

	FVector ClimbRightCornerOuterTraceStart = CharacterLocation;
	FVector ClimbRightCornerOuterTraceEnd = CharacterLocation + CharacterRightVector * (CharacterCapsuleRadius + 5);

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, ClimbRightCornerOuterTraceStart, ClimbRightCornerOuterTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

	float CornerOuterAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(HitResult.ImpactNormal.GetSafeNormal(), -CharacterRightVector));

	if(HitResult.bBlockingHit &&
	   CornerOuterAngle <= 5)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbingAction_OuterRight, MontagePlayInofo))
			return false;

		FindClimbRightCornerOuter = true;

		FVector AdjustLocation = HitResult.ImpactPoint + 
								 CharacterRightVector * MontagePlayInofo.AnimMontageOffSet.Y;
		OwnerCharacter->SetActorLocation(AdjustLocation, true);

		if(bDrawDebug)
			DrawDebugSphere(OwnerCharacter->GetWorld(), AdjustLocation, 10, 32, FColor::Yellow, false, 3);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				ObstacleDetectionClimbing(150, ObstacleLocation, ObstacleNormalDir);
				FindClimbingRotationIdle();
			});

		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return FindClimbRightCornerOuter;
}

bool UClimbComponent::ClimbLeftCornerOuterCheck()
{
	bool FindClimbLeftCornerOuter = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindClimbLeftCornerOuter;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	float CharacterCapsuleRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();

	FVector ClimbLeftCornerOuterTraceStart = CharacterLocation;
	FVector ClimbLeftCornerOuterTraceEnd = CharacterLocation + CharacterRightVector * (CharacterCapsuleRadius + 5) * -1;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, ClimbLeftCornerOuterTraceStart, ClimbLeftCornerOuterTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

	float CornerOuterAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(HitResult.ImpactNormal.GetSafeNormal(), CharacterRightVector));

	if (HitResult.bBlockingHit &&
		CornerOuterAngle <= 5)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbingAction_OuterLeft, MontagePlayInofo))
			return false;

		FindClimbLeftCornerOuter = true;

		FVector AdjustLocation = HitResult.ImpactPoint +
								 CharacterRightVector * MontagePlayInofo.AnimMontageOffSet.Y;
		OwnerCharacter->SetActorLocation(AdjustLocation, true);

		if (bDrawDebug)
			DrawDebugSphere(OwnerCharacter->GetWorld(), AdjustLocation, 10, 32, FColor::Yellow, false, 3);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				ObstacleDetectionClimbing(150, ObstacleLocation, ObstacleNormalDir);
				FindClimbingRotationIdle();
			});

		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return FindClimbLeftCornerOuter;
}

bool UClimbComponent::HangingRightCornerOuterCheck()
{
	bool FindClimbRightCornerOuter = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindClimbRightCornerOuter;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	float CharacterCapsuleRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();

	FVector ClimbRightCornerOuterTraceStart = GetTopLocation() +
											  FVector::UpVector * GHangingTraceOffsetZ +
											  OwnerCharacter->GetActorForwardVector() * -10;
	FVector ClimbRightCornerOuterTraceEnd = ClimbRightCornerOuterTraceStart + CharacterRightVector * (CharacterCapsuleRadius + 5);

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, ClimbRightCornerOuterTraceStart, ClimbRightCornerOuterTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

	float CornerOuterAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(HitResult.ImpactNormal.GetSafeNormal(), -CharacterRightVector));

	if (HitResult.bBlockingHit &&
		CornerOuterAngle <= 5)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::Hanging_OuterRight, MontagePlayInofo))
			return false;

		FindClimbRightCornerOuter = true;

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				ObstacleDetectionHanging(50, ObstacleLocation, ObstacleNormalDir);
				FindClimbingRotationIdle();
			});

		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return FindClimbRightCornerOuter;
}

bool UClimbComponent::HangingLeftCornerOuterCheck()
{
	bool FindClimbLeftCornerOuter = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindClimbLeftCornerOuter;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterLeftVector = -OwnerCharacter->GetActorRightVector();
	float CharacterCapsuleRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();

	FVector ClimbRightCornerOuterTraceStart = GetTopLocation() +
											  FVector::UpVector * GHangingTraceOffsetZ + 
											  OwnerCharacter->GetActorForwardVector() * -10;
	FVector ClimbRightCornerOuterTraceEnd = ClimbRightCornerOuterTraceStart + CharacterLeftVector * (CharacterCapsuleRadius + 5);

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, ClimbRightCornerOuterTraceStart, ClimbRightCornerOuterTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

	float CornerOuterAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(HitResult.ImpactNormal.GetSafeNormal(), -CharacterLeftVector));

	if (HitResult.bBlockingHit &&
		CornerOuterAngle <= 5)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::Hanging_OuterLeft, MontagePlayInofo))
			return false;

		FindClimbLeftCornerOuter = true;

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				ObstacleDetectionHanging(50, ObstacleLocation, ObstacleNormalDir);
				FindClimbingRotationIdle();
			});

		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return FindClimbLeftCornerOuter;
}

bool UClimbComponent::ClimbUpActionCheck()
{
	bool FindUpLanding = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindUpLanding;

	float ScaledCapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float ScaledCapsuleRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterTopLocation = GetTopLocation();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();

	float CapsuleHeightCheck = ScaledCapsuleHalfHeight * 2;
	float CheckRadius = 20;

	//Check Obstacle is wall
	FVector ObstacleWallCheckTraceStart = CharacterTopLocation + CharacterForwardVector * (2 * ScaledCapsuleRadius + 50) + FVector::UpVector * ScaledCapsuleHalfHeight;
	FVector ObstacleWallCheckTraceEnd = CharacterTopLocation + CharacterForwardVector * (2 * ScaledCapsuleRadius + 50) + FVector::DownVector * ScaledCapsuleHalfHeight * 0.5;

	FHitResult ObstacleWallCheckHitResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, ObstacleWallCheckTraceStart, ObstacleWallCheckTraceEnd, ScaledCapsuleRadius, TArray<AActor*>(), bDrawDebug, FColor::Orange, FColor::Green, 5);

	bool ObstacleIsWall = !ObstacleWallCheckHitResult.bBlockingHit;

	if(!ObstacleIsWall)
	{
		FVector UpperFloorCheckXY = GetTopLocation() + CharacterForwardVector * ScaledCapsuleRadius;
		FVector UpperFloorCheckStart = UpperFloorCheckXY + FVector::UpVector * (CapsuleHeightCheck - CheckRadius);
		FVector UpperFloorCheckEnd = UpperFloorCheckXY + FVector::DownVector * ScaledCapsuleHalfHeight;//for adjust
		FHitResult UpperFloorCheckHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, UpperFloorCheckStart, UpperFloorCheckEnd, CheckRadius, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 3);

		bool CanClimbUp = true;
		float FloorAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(UpperFloorCheckHit.ImpactNormal.GetSafeNormal(), FVector::UpVector));
		if (!UpperFloorCheckHit.bBlockingHit ||
			UpperFloorCheckHit.Distance < (ScaledCapsuleHalfHeight * 2 - ScaledCapsuleRadius * 1.5) || // Hasn't space for put character's capsule
			FloorAngle > OwnerCharacter->GetCharacterMovement()->GetWalkableFloorAngle()
			)
		{
			CanClimbUp = false;
		}

		if (CanClimbUp)
		{
			FMontagePlayInofo MontagePlayInofo;
			if(!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbingAction_ClimbUpLand, MontagePlayInofo))
			{
				return FindUpLanding;
			}

			FindUpLanding = true;

			FVector CharaterUpVector = OwnerCharacter->GetActorUpVector();

			FTransform MotionWarpingTransform;

			FRotator MotionWarpingRotation;
			MotionWarpingRotation.Yaw = OwnerCharacter->GetActorRotation().Yaw;

			MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

			FVector MotionWarpingLocation = UpperFloorCheckHit.ImpactPoint +
									        ObstacleNormalDir * MontagePlayInofo.AnimMontageOffSet.X +
											CharaterUpVector * MontagePlayInofo.AnimMontageOffSet.Y;

			MotionWarpingTransform.SetLocation(MotionWarpingLocation);

			if (bDrawDebug)
				DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

			OwnerCharacter->GetCapsuleComponent()->SetPhysicsLinearVelocity(FVector::ZeroVector);

			FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
			MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

			ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

			FOnMontageBlendingOutStarted BlendingOutDelegate;
			BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
				{
					SetUpDefaultState();
					ClimbingMovementComponent->BrakingDecelerationWalking = 4000;
				});

			FOnMontageEnded MontageEndedDelegate;
			MontageEndedDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
				{
					ClimbingMovementComponent->BrakingDecelerationWalking = 3000;
				});

			ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
			ClimbingAnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontagePlayInofo.AnimMontageToPlay);
		}
	}
	else
	{
		FVector UpperFloorCheckXY = GetTopLocation() + CharacterForwardVector * ScaledCapsuleRadius;
		FVector UpperFloorCheckStart = UpperFloorCheckXY + FVector::UpVector * (ScaledCapsuleHalfHeight - CheckRadius);
		FVector UpperFloorCheckEnd = UpperFloorCheckXY + FVector::DownVector * ScaledCapsuleHalfHeight;//for adjust

		FHitResult UpperFloorCheckHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, UpperFloorCheckStart, UpperFloorCheckEnd, CheckRadius, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 3);

		bool CanPassVault = true;
		//Check Character vault the top of Obstacle
		if(!UpperFloorCheckHit.bBlockingHit ||
			UpperFloorCheckHit.Distance < (ScaledCapsuleHalfHeight - ScaledCapsuleRadius)
		   )
		{
			CanPassVault = false;
		}

		if(CanPassVault)
		{
			// Check vault point floor
			FVector CharacterVaultPointFloorTraceStart = GetTopLocation() + CharacterForwardVector * (2 * ScaledCapsuleRadius + 50);
			FVector CharacterVaultPointFloorTraceEnd = CharacterVaultPointFloorTraceStart + FVector::DownVector * (ScaledCapsuleHalfHeight * 4);

			FHitResult CharacterVaultPointFloorHitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, CharacterVaultPointFloorTraceStart, CharacterVaultPointFloorTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Yellow, FColor::Red, 5);

			bool CanVault = CharacterVaultPointFloorHitResult.bBlockingHit;

			FMontagePlayInofo MontagePlayInofo;

			if(!FindMontagePlayInofoByClimbAction(CanVault?UClimbAction::ClimbingAction_UpVault:UClimbAction::ClimbingAction_UpTurnVault, MontagePlayInofo))
				return FindUpLanding;

			FindUpLanding = true;

			FTransform MotionWarpingTransform;

			FRotator MotionWarpingRotation = UKismetMathLibrary::MakeRotFromX(CharacterForwardVector);
			MotionWarpingRotation.Pitch = 0;

			MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

			FVector MotionWarpingLocation = UpperFloorCheckHit.ImpactPoint +
											CharacterForwardVector * MontagePlayInofo.AnimMontageOffSet.X +
											ClimbingUpVector * MontagePlayInofo.AnimMontageOffSet.Y;

			MotionWarpingTransform.SetLocation(MotionWarpingLocation);

			if (bDrawDebug)
				DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

			FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
			MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

			if(!CanVault)
			{

				FVector CharacterVaultEndPointTraceStart = UpperFloorCheckHit.ImpactPoint + CharacterForwardVector * (ScaledCapsuleRadius * 2 + 10) + FVector::DownVector * (ScaledCapsuleHalfHeight + ScaledCapsuleRadius);
				FVector CharacterVaultEndPointTraceEnd = CharacterVaultEndPointTraceStart + CharacterForwardVector * (ScaledCapsuleRadius * 2 + 50) * -1;

				FHitResult  CharacterVaultEndPointHitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, CharacterVaultEndPointTraceStart, CharacterVaultEndPointTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Yellow, FColor::Red, 5);

				FTransform MotionWarpingEndTransform;

				FRotator MotionWarpinEndRotation = UKismetMathLibrary::MakeRotFromX(-CharacterVaultEndPointHitResult.Normal);
				MotionWarpinEndRotation.Pitch = 0;

				MotionWarpingEndTransform.SetRotation(MotionWarpinEndRotation.Quaternion());

				FMotionWarpingTarget MotionWarpingEndTarget = FMotionWarpingTarget("ClimbEndTarget", MotionWarpingEndTransform);
				MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingEndTarget);
			}

			ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

			if (CanVault)
			{
				FOnMontageBlendingOutStarted BlendingOutDelegate;
				BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
					{
						SetUpDefaultState();
					});

				ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
			}
			else
			{
				FOnMontageBlendingOutStarted BlendingOutDelegate;
				BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
					{
						ObstacleDetectionClimbing(150, ObstacleLocation, ObstacleNormalDir);
						FindClimbingRotationIdle();
					});

				ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
			}
		}
	}

	return FindUpLanding;
}

bool UClimbComponent::ClimbLandingCheck()
{
	bool FindLanding = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindLanding;

	FVector FindFloorTraceStart = GetFootLocation();
	FVector FindFloorTraceEnd = FindFloorTraceStart + OwnerCharacter->GetActorUpVector() * -200;

	FHitResult FindFloorTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, FindFloorTraceStart, FindFloorTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Blue, FColor::Green);

	if (FindFloorTraceCheckHit.bBlockingHit)
	{
		FindLanding = true;

		if (FindFloorTraceCheckHit.Distance >= 150)
		{
			FMontagePlayInofo MontagePlayInofo;

			if(!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbingAction_LandDown, MontagePlayInofo))
				return false;

			FTransform MotionWarpingTransform;

			FRotator MotionWarpingRotation;
			MotionWarpingRotation.Yaw = OwnerCharacter->GetActorRotation().Yaw;

			MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

			FVector MotionWarpingLocation = FindFloorTraceCheckHit.ImpactPoint +
			                                ObstacleNormalDir * MontagePlayInofo.AnimMontageOffSet.X;
			MotionWarpingLocation.Z = MotionWarpingLocation.Z -
									  MontagePlayInofo.AnimMontageOffSet.Y +
									  OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

			MotionWarpingTransform.SetLocation(MotionWarpingLocation);

			if (bDrawDebug)
				DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

			FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
			MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

			ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);
		}

		SetUpDefaultState();
	}
	return FindLanding;
}

bool UClimbComponent::ClimbingToHangingCheck()
{
	bool FindClimbingToHanging = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindClimbingToHanging;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterUpVector = OwnerCharacter->GetActorUpVector();
	float ScaledCapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float ScaledCapsuleRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();

	FVector HangingTargetTraceStart = GetFootLocation() + 
									  CharacterForwardVector * ScaledCapsuleRadius +
									  FVector::DownVector * (ScaledCapsuleRadius + 20);

	FVector HangingTargetTraceEnd = HangingTargetTraceStart + 
									FVector::UpVector * ScaledCapsuleHalfHeight;

	FHitResult HangingTargetHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, HangingTargetTraceStart, HangingTargetTraceEnd, 20, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green,3);

	
	if(HangingTargetHit.bBlockingHit)
	{
		FVector HangingTarget = HangingTargetHit.ImpactPoint;

		//Check the palyer can hanging 
		FVector PlayerHangingCheckStart = HangingTarget +
										  FVector::DownVector * (GHangingTraceOffsetZ + ScaledCapsuleRadius + 5) +
										  CharacterForwardVector * -5;

		FVector PlayerHangingCheckEnd = PlayerHangingCheckStart +
										FVector::DownVector * ( 2 * ScaledCapsuleHalfHeight - ScaledCapsuleRadius);

		FHitResult PlayerHangingHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, PlayerHangingCheckStart, PlayerHangingCheckEnd, ScaledCapsuleRadius, TArray<AActor*>(), bDrawDebug, FColor::Orange, FColor::Green,3);

		if(!PlayerHangingHit.bBlockingHit)
		{
			FMontagePlayInofo MontagePlayInofo;
			if (!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbingAction_ClimbingToHanging, MontagePlayInofo))
				return false;

			FindClimbingToHanging = true;

			FTransform MotionWarpingTransform;

			FRotator MotionWarpingRotation;
			MotionWarpingRotation.Yaw = OwnerCharacter->GetActorRotation().Yaw;

			MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

			FVector MotionWarpingLocation = HangingTarget +
											CharacterForwardVector * MontagePlayInofo.AnimMontageOffSet.X +
											FVector::DownVector * MontagePlayInofo.AnimMontageOffSet.Y;

			MotionWarpingTransform.SetLocation(MotionWarpingLocation);

			if (bDrawDebug)
				DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

			FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
			MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

			ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

			FOnMontageBlendingOutStarted BlendingOutDelegate;
			BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
				{
					ObstacleDetectionHanging(50, ObstacleLocation, ObstacleNormalDir);
					SetUpHangingState();
				});
			ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
		}
	}
	
	return FindClimbingToHanging;
}

bool UClimbComponent::HangingClimbUpCheck()
{
	bool FindHangingClimbUp = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindHangingClimbUp;

	float ScaledCapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float ScaledCapsuleRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterTopLocation = GetTopLocation();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterUpVector = OwnerCharacter->GetActorUpVector();

	float CapsuleHeightCheck = ScaledCapsuleHalfHeight * 2;
	float CheckRadius = 20;

	//Check is Land
	FVector LandTraceStart = CharacterTopLocation +
							 CharacterForwardVector * 2 * ScaledCapsuleRadius +
							 FVector::UpVector * 1 * ScaledCapsuleRadius;

	FVector LandTraceEnd = LandTraceStart +
						   FVector::DownVector * 2 * ScaledCapsuleRadius;

	FHitResult LandFloorCheckHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, LandTraceStart, LandTraceEnd, CheckRadius, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

	if(LandFloorCheckHit.bBlockingHit)
	{
		//Check player can stand up
		FVector StandUpTraceStart = FVector(ObstacleLocation.X,ObstacleLocation.Y, LandFloorCheckHit.ImpactPoint.Z) + 
									CharacterForwardVector * 2 * ScaledCapsuleRadius + 
									FVector::UpVector * (ScaledCapsuleRadius + 5);

		FVector StandUpTraceEnd = StandUpTraceStart +
								  FVector::UpVector * (CapsuleHeightCheck - 2 * ScaledCapsuleRadius);

		FHitResult StandUpCheckHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, StandUpTraceStart, StandUpTraceEnd, ScaledCapsuleRadius, TArray<AActor*>(), bDrawDebug, FColor::Orange, FColor::Green,3);

		if(!StandUpCheckHit.bBlockingHit)
		{
			FMontagePlayInofo MontagePlayInofo;

			if (!FindMontagePlayInofoByClimbAction(UClimbAction::Hanging_ClimbUp, MontagePlayInofo))
				return false;

			FindHangingClimbUp = true;

			FTransform MotionWarpingTransform;

			FRotator MotionWarpingRotation;
			MotionWarpingRotation.Yaw = OwnerCharacter->GetActorRotation().Yaw;

			MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

			FVector MotionWarpingLocation = FVector(ObstacleLocation.X, ObstacleLocation.Y, LandFloorCheckHit.ImpactPoint.Z) +
											ObstacleNormalDir * MontagePlayInofo.AnimMontageOffSet.X +
											CharacterUpVector * MontagePlayInofo.AnimMontageOffSet.Y;

			MotionWarpingTransform.SetLocation(MotionWarpingLocation);

			if (bDrawDebug)
				DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

			FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
			MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

			OwnerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Ignore);

			ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

			FOnMontageBlendingOutStarted BlendingOutDelegate;
			BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
				{
					SetUpDefaultState();
					ClimbingMovementComponent->BrakingDecelerationWalking = 4000;
				});

			FOnMontageEnded MontageEndedDelegate;
			MontageEndedDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
				{
					ClimbingMovementComponent->BrakingDecelerationWalking = 3000;
				});

			ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
			ClimbingAnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontagePlayInofo.AnimMontageToPlay);
		}
	}
	return FindHangingClimbUp;
}

bool UClimbComponent::HangingTurnCheck()
{
	bool FindHangingTurn = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindHangingTurn;

	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	float CharacterHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FVector HangingTurnTraceStart = GetTopLocation() + FVector::UpVector * GHangingTraceOffsetZ + CharacterForwardVector * 100;
	FVector HangingTurnTraceEnd = HangingTurnTraceStart + CharacterForwardVector * -100;

	FHitResult HangingTurnTraceHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, HangingTurnTraceStart, HangingTurnTraceEnd, {}, bDrawDebug, FColor::Orange, FColor::Green);

	if(HangingTurnTraceHit.bBlockingHit)
	{
		FVector HangingTargetTraceStart = HangingTurnTraceHit.Location +
										  CharacterForwardVector * (CharacterRadius + 5);

		FVector HangingTargetTraceEnd = HangingTargetTraceStart + 
										FVector::DownVector * (CharacterHalfHeight * 2);

		FHitResult HangingTargetHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, HangingTargetTraceStart, HangingTargetTraceEnd, CharacterRadius, {}, bDrawDebug, FColor::Orange, FColor::Green,3);
		if( HangingTargetHit.bBlockingHit )
			return false;

		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::Hanging_Turn, MontagePlayInofo))
			return false;

		FindHangingTurn = true;

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				ObstacleDetectionHanging(50, ObstacleLocation, ObstacleNormalDir);
				FindClimbingRotationIdle();
			});

		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return FindHangingTurn;
}

bool UClimbComponent::HangingDropCheck()
{
	bool FindHangingDrop = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindHangingDrop;

	FMontagePlayInofo MontagePlayInofo;
	if (!FindMontagePlayInofoByClimbAction(UClimbAction::Hanging_Drop, MontagePlayInofo))
		return false;

	FindHangingDrop = true;

	SetUpDefaultState();
	ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

	return FindHangingDrop;
}

bool UClimbComponent::ClimbPipeLandUpCheck()
{
	bool FindClimbPipeLandUp = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindClimbPipeLandUp;

	float ScaledCapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float ScaledCapsuleRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterTopLocation = GetTopLocation();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();

	float CapsuleHeightCheck = ScaledCapsuleHalfHeight * 2;
	float CheckRadius = 20;

	FVector UpperFloorCheckXY = GetTopLocation() + CharacterForwardVector * ScaledCapsuleRadius;
	FVector UpperFloorCheckStart = UpperFloorCheckXY + FVector::UpVector * (CapsuleHeightCheck - CheckRadius);
	FVector UpperFloorCheckEnd = UpperFloorCheckXY + FVector::DownVector * ScaledCapsuleHalfHeight;//for adjust
	FHitResult UpperFloorCheckHit = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, UpperFloorCheckStart, UpperFloorCheckEnd, CheckRadius, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 3);

	bool CanClimbUp = true;
	float FloorAngle = UKismetMathLibrary::DegAcos(FVector::DotProduct(UpperFloorCheckHit.ImpactNormal.GetSafeNormal(), FVector::UpVector));

	if (!UpperFloorCheckHit.bBlockingHit ||
		UpperFloorCheckHit.Distance < (ScaledCapsuleHalfHeight * 2 - ScaledCapsuleRadius * 1.5) || // Hasn't space for put character's capsule
		FloorAngle > OwnerCharacter->GetCharacterMovement()->GetWalkableFloorAngle()
		)
	{
		CanClimbUp = false;
	}

	if(CanClimbUp)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbPipeAction_ClimbUpLand, MontagePlayInofo))
			return false;

		FindClimbPipeLandUp = true;

		FVector CharaterUpVector = OwnerCharacter->GetActorUpVector();

		FTransform MotionWarpingTransform;

		FRotator MotionWarpingRotation;
		MotionWarpingRotation.Yaw = OwnerCharacter->GetActorRotation().Yaw;

		MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

		FVector MotionWarpingLocation = UpperFloorCheckHit.ImpactPoint +
										ObstacleNormalDir * MontagePlayInofo.AnimMontageOffSet.X +
										CharaterUpVector * MontagePlayInofo.AnimMontageOffSet.Y;

		MotionWarpingTransform.SetLocation(MotionWarpingLocation);

		if (bDrawDebug)
			DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

		FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
		MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				SetUpDefaultState();
				ClimbingMovementComponent->BrakingDecelerationWalking = 4000;
			});

		FOnMontageEnded MontageEndedDelegate;
		MontageEndedDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				ClimbingMovementComponent->BrakingDecelerationWalking = 3000;
			});

		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
		ClimbingAnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return FindClimbPipeLandUp;
}

bool UClimbComponent::ClimbPipeLandDownCheck()
{
	bool FindClimbPipeLandDown = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindClimbPipeLandDown;

	FVector FindFloorTraceStart = GetFootLocation();
	FVector FindFloorTraceEnd = FindFloorTraceStart + OwnerCharacter->GetActorUpVector() * -200;

	FHitResult FindFloorTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, FindFloorTraceStart, FindFloorTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Blue, FColor::Green);

	if(FindFloorTraceCheckHit.bBlockingHit)
	{
		FindClimbPipeLandDown = true;

		if (FindFloorTraceCheckHit.Distance >= 150)
		{
			FMontagePlayInofo MontagePlayInofo;

			if (!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbPipeAction_LandDown, MontagePlayInofo))
				return false;

			FTransform MotionWarpingTransform;

			FRotator MotionWarpingRotation;
			MotionWarpingRotation.Yaw = OwnerCharacter->GetActorRotation().Yaw;

			MotionWarpingTransform.SetRotation(MotionWarpingRotation.Quaternion());

			FVector MotionWarpingLocation = FindFloorTraceCheckHit.ImpactPoint + 
											ObstacleNormalDir * MontagePlayInofo.AnimMontageOffSet.X;
			MotionWarpingLocation.Z = MotionWarpingLocation.Z - 
									  MontagePlayInofo.AnimMontageOffSet.Y +
									  OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

			MotionWarpingTransform.SetLocation(MotionWarpingLocation);

			if (bDrawDebug)
				DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

			FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
			MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

			ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);
		}
		SetUpDefaultState();
	}

	return FindClimbPipeLandDown;
}

bool UClimbComponent::ClimbPipeRightJumpCheck()
{
	bool FindClimbRightJump = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindClimbRightJump;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();

	float CharacterCapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float CharacterCapsuleRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();

	FVector CharacterRightLocation = CharacterLocation + CharacterRightVector * CharacterCapsuleRadius;

	FVector PipeTraceStart = CharacterLocation;
	FVector PipeTraceEnd = PipeTraceStart + CharacterForwardVector * 50;

	FHitResult PipeTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, PipeTraceStart, PipeTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Blue, FColor::Green);

	FVector PipeRightWallTraceStart = CharacterRightLocation;
	FVector PipeRightWallTraceEnd = CharacterRightLocation + CharacterForwardVector * 100;

	FHitResult PipeRightWallTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, PipeRightWallTraceStart, PipeRightWallTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Blue, FColor::Green);

	if(PipeTraceCheckHit.bBlockingHit && PipeRightWallTraceCheckHit.bBlockingHit)
	{
		FVector CharacterUpVector = OwnerCharacter->GetActorUpVector();
		FVector RightCheckVector = FVector::CrossProduct(PipeRightWallTraceCheckHit.Normal, CharacterUpVector);

		AActor* PipeActor = PipeTraceCheckHit.GetActor();
		FVector PipeActorLocation = PipeActor->GetActorLocation();

		FVector PipeRightTraceStart = FVector(PipeActorLocation.X,PipeActorLocation.Y,PipeTraceCheckHit.Location.Z);
		FVector PipeRightTraceEnd = PipeRightTraceStart + RightCheckVector * 200;

		FHitResult PipeRightTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, PipeRightTraceStart, PipeRightTraceEnd, { PipeActor }, bDrawDebug, FColor::Blue, FColor::Green);

		if(PipeRightTraceCheckHit.bBlockingHit)
		{
			AActor* JumpTargetPipe = PipeRightTraceCheckHit.GetActor();
			FVector JumpTargetPipeLocation = JumpTargetPipe->GetActorLocation();

			FVector PipeJumpTargetTopTraceEnd = FVector(JumpTargetPipeLocation.X,JumpTargetPipeLocation.Y, PipeRightTraceCheckHit.Location.Z)+ FVector::UpVector * CharacterCapsuleHalfHeight;
			FVector PipeJumpTargetTopTraceStart = PipeJumpTargetTopTraceEnd + PipeRightWallTraceCheckHit.Normal * 50;

			FHitResult PipeJumpTargetTopTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, PipeJumpTargetTopTraceStart, PipeJumpTargetTopTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Blue, FColor::Green);

			FVector PipeJumpTargetFootTraceEnd = FVector(JumpTargetPipeLocation.X, JumpTargetPipeLocation.Y, PipeRightTraceCheckHit.Location.Z) + FVector::DownVector * CharacterCapsuleHalfHeight;
			FVector PipeJumpTargetFootTraceStart = PipeJumpTargetFootTraceEnd + PipeRightWallTraceCheckHit.Normal * 50;

			FHitResult PipeJumpTargetFootTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, PipeJumpTargetFootTraceStart, PipeJumpTargetFootTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Blue, FColor::Green);

			if(PipeJumpTargetTopTraceCheckHit.bBlockingHit && PipeJumpTargetFootTraceCheckHit.bBlockingHit)
			{
				FMontagePlayInofo MontagePlayInofo;

				if (!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbPipeAction_RightJump, MontagePlayInofo))
					return false;

				FindClimbRightJump = true;

				FTransform MotionWarpingTransform;

				FVector MotionWarpingLocation = FVector(JumpTargetPipeLocation.X, JumpTargetPipeLocation.Y, PipeRightTraceCheckHit.Location.Z) + 
												PipeRightWallTraceCheckHit.Normal * (MontagePlayInofo.AnimMontageOffSet.X + CharacterCapsuleRadius) +
												CharacterUpVector * MontagePlayInofo.AnimMontageOffSet.Y;

				MotionWarpingTransform.SetLocation(MotionWarpingLocation);

				if (bDrawDebug)
					DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

				FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
				MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

				ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);
			}
		}
	}

	return FindClimbRightJump;
}

bool UClimbComponent::ClimbPipeLeftJumpCheck()
{
	bool FindClimbLeftJump = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return FindClimbLeftJump;

	FVector CharacterLocation = OwnerCharacter->GetActorLocation();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();

	float CharacterCapsuleHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float CharacterCapsuleRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();

	FVector CharacterLeftLocation = CharacterLocation + -CharacterRightVector * CharacterCapsuleRadius;

	FVector PipeTraceStart = CharacterLocation;
	FVector PipeTraceEnd = PipeTraceStart + CharacterForwardVector * 50;

	FHitResult PipeTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, PipeTraceStart, PipeTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Blue, FColor::Green);

	FVector PipeLeftWallTraceStart = CharacterLeftLocation;
	FVector PipeLeftWallTraceEnd = CharacterLeftLocation + CharacterForwardVector * 100;

	FHitResult PipeLeftWallTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, PipeLeftWallTraceStart, PipeLeftWallTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Blue, FColor::Green);

	if (PipeTraceCheckHit.bBlockingHit && PipeLeftWallTraceCheckHit.bBlockingHit)
	{
		FVector CharacterUpVector = OwnerCharacter->GetActorUpVector();
		FVector LeftCheckVector = -FVector::CrossProduct(PipeLeftWallTraceCheckHit.Normal, CharacterUpVector);

		AActor* PipeActor = PipeTraceCheckHit.GetActor();
		FVector PipeActorLocation = PipeActor->GetActorLocation();

		FVector PipeLeftTraceStart = FVector(PipeActorLocation.X, PipeActorLocation.Y, PipeTraceCheckHit.Location.Z);
		FVector PipeLeftTraceEnd = PipeLeftTraceStart + LeftCheckVector * 200;

		FHitResult PipeLeftTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, PipeLeftTraceStart, PipeLeftTraceEnd, { PipeActor }, bDrawDebug, FColor::Blue, FColor::Green);

		if (PipeLeftTraceCheckHit.bBlockingHit)
		{
			AActor* JumpTargetPipe = PipeLeftTraceCheckHit.GetActor();
			FVector JumpTargetPipeLocation = JumpTargetPipe->GetActorLocation();

			FVector PipeJumpTargetTopTraceEnd = FVector(JumpTargetPipeLocation.X, JumpTargetPipeLocation.Y, PipeLeftTraceCheckHit.Location.Z) + FVector::UpVector * CharacterCapsuleHalfHeight;
			FVector PipeJumpTargetTopTraceStart = PipeJumpTargetTopTraceEnd + PipeLeftTraceCheckHit.Normal * 50;

			FHitResult PipeJumpTargetTopTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, PipeJumpTargetTopTraceStart, PipeJumpTargetTopTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Blue, FColor::Green);

			FVector PipeJumpTargetFootTraceEnd = FVector(JumpTargetPipeLocation.X, JumpTargetPipeLocation.Y, PipeLeftTraceCheckHit.Location.Z) + FVector::DownVector * CharacterCapsuleHalfHeight;
			FVector PipeJumpTargetFootTraceStart = PipeJumpTargetFootTraceEnd + PipeLeftWallTraceCheckHit.Normal * 50;

			FHitResult PipeJumpTargetFootTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, PipeJumpTargetFootTraceStart, PipeJumpTargetFootTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Blue, FColor::Green);

			if (PipeJumpTargetTopTraceCheckHit.bBlockingHit && PipeJumpTargetFootTraceCheckHit.bBlockingHit)
			{
				FMontagePlayInofo MontagePlayInofo;

				if (!FindMontagePlayInofoByClimbAction(UClimbAction::ClimbPipeAction_LeftJump, MontagePlayInofo))
					return false;

				FindClimbLeftJump = true;

				FTransform MotionWarpingTransform;

				FVector MotionWarpingLocation = FVector(JumpTargetPipeLocation.X, JumpTargetPipeLocation.Y, PipeLeftTraceCheckHit.Location.Z) +
												PipeLeftWallTraceCheckHit.Normal * (MontagePlayInofo.AnimMontageOffSet.X + CharacterCapsuleRadius) +
												CharacterUpVector * MontagePlayInofo.AnimMontageOffSet.Y;

				MotionWarpingTransform.SetLocation(MotionWarpingLocation);

				if (bDrawDebug)
					DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

				FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
				MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

				ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);
			}
		}
	}

	return FindClimbLeftJump;
}

bool UClimbComponent::BalanceUpToWalkCheck()
{
	bool CanBalanceUpToWalk = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanBalanceUpToWalk;

	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterForwardFoot = GetFootLocation() + (CharacterRadius) * CharacterForwardVector;

	FVector CharacterLeftFloorTraceStart = CharacterForwardFoot + FVector::UpVector * 30 + -CharacterRightVector * CharacterRadius * GBalanceTraceScale;
	FVector CharacterLeftFloorTraceEnd = CharacterLeftFloorTraceStart + FVector::DownVector * 60;

	FHitResult  CharacterLeftFloorTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, CharacterLeftFloorTraceStart, CharacterLeftFloorTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

	FVector CharacterRightFloorTraceStart = CharacterForwardFoot + FVector::UpVector * 30 + CharacterRightVector * CharacterRadius * GBalanceTraceScale;
	FVector CharacterRightFloorTraceEnd = CharacterRightFloorTraceStart + FVector::DownVector * 60;

	FHitResult  CharacterRightFloorTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, CharacterRightFloorTraceStart, CharacterRightFloorTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

	if(CharacterLeftFloorTraceCheckHit.bBlockingHit && CharacterRightFloorTraceCheckHit.bBlockingHit)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::Balance_BalanceUpToWalk, MontagePlayInofo))
			return false;

		CanBalanceUpToWalk = true;

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				SetUpDefaultState();
			});

		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);

	}

	return CanBalanceUpToWalk;
}

bool UClimbComponent::BalanceDownToWalkCheck()
{
	bool CanBalanceDownToWalk = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanBalanceDownToWalk;

	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterBackwardFoot = GetFootLocation() + CharacterRadius * -CharacterForwardVector;

	FVector CharacterLeftFloorTraceStart = CharacterBackwardFoot + FVector::UpVector * 30 + -CharacterRightVector * CharacterRadius * GBalanceTraceScale;
	FVector CharacterLeftFloorTraceEnd = CharacterLeftFloorTraceStart + FVector::DownVector * 60;

	FHitResult  CharacterLeftFloorTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, CharacterLeftFloorTraceStart, CharacterLeftFloorTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

	FVector CharacterRightFloorTraceStart = CharacterBackwardFoot + FVector::UpVector * 30 + CharacterRightVector * CharacterRadius * GBalanceTraceScale;
	FVector CharacterRightFloorTraceEnd = CharacterRightFloorTraceStart + FVector::DownVector * 60;

	FHitResult  CharacterRightFloorTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, CharacterRightFloorTraceStart, CharacterRightFloorTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

	if (CharacterLeftFloorTraceCheckHit.bBlockingHit && CharacterRightFloorTraceCheckHit.bBlockingHit)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::Balance_BalanceDownToWalk, MontagePlayInofo))
			return false;

		CanBalanceDownToWalk = true;

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted BlendingOutDelegate;
		BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				SetUpDefaultState();
			});

		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);

	}

	return CanBalanceDownToWalk;
}

bool UClimbComponent::BalanceTurnBackCheck()
{
	bool CanBalanceTurnBack = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanBalanceTurnBack;

	FVector CharacterForwardVector = OwnerCharacter->GetActorRotation().Quaternion().GetForwardVector();
	FVector ControllerRotationOnyYawForwardVector = FRotator(0,OwnerCharacter->GetControlRotation().Yaw,0).Quaternion().GetForwardVector();

	float Degree = UKismetMathLibrary::DegAcos(FVector::DotProduct(CharacterForwardVector, ControllerRotationOnyYawForwardVector));

	if(Degree >= 150)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::Balance_BalanceTurnBack, MontagePlayInofo))
			return false;

		CanBalanceTurnBack = true;

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted MontageBlendingOutDelegate;
		MontageBlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				FloorDectectionBalance(FloorLocation, FloorNormalDir);
				FindBalanceRotationIdle();
			});
		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(MontageBlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return CanBalanceTurnBack;
}

bool UClimbComponent::NarrowSpaceUpToWalkCheck()
{
	bool CanNarrowSpaceUpToWalk = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanNarrowSpaceUpToWalk;

	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	float CharacterHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	FVector CharacterTargetTraceStart = CharacterLocation +
										CharacterForwardVector * CharacterRadius * 2 +
										FVector::UpVector * (CharacterHalfHeight - CharacterRadius);

	FVector CharacterTargetTraceEnd = CharacterLocation +
									  CharacterForwardVector * CharacterRadius * 2 +
									  FVector::DownVector * (CharacterHalfHeight - CharacterRadius);

	FHitResult  CharacterTargetTraceResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, CharacterTargetTraceStart, CharacterTargetTraceEnd, CharacterRadius,TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 3);
	if(!CharacterTargetTraceResult.bBlockingHit)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::NarrowSpace_NarrowSpaceUpToWalk, MontagePlayInofo))
			return false;

		CanNarrowSpaceUpToWalk = true;

		FTransform MotionWarpingTransform;

		FVector MotionWarpingLocation = CharacterLocation + CharacterForwardVector * CharacterRadius * 2;

		MotionWarpingTransform.SetLocation(MotionWarpingLocation);

		if (bDrawDebug)
			DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

		FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
		MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted MontageBlendingOutDelegate;
		MontageBlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				SetUpDefaultState();
			});
		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(MontageBlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}
	return CanNarrowSpaceUpToWalk;
}

bool UClimbComponent::NarrowSpaceDownToWalkCheck()
{
	bool CanNarrowSpaceDownToWalk = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanNarrowSpaceDownToWalk;

	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	float CharacterHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	FVector CharacterTargetTraceStart = CharacterLocation +
										-CharacterForwardVector * CharacterRadius * 2 +
										FVector::UpVector * (CharacterHalfHeight - CharacterRadius);

	FVector CharacterTargetTraceEnd = CharacterLocation +
									  -CharacterForwardVector * CharacterRadius * 2 +
		                              FVector::DownVector * (CharacterHalfHeight - CharacterRadius);

	FHitResult  CharacterTargetTraceResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, CharacterTargetTraceStart, CharacterTargetTraceEnd, CharacterRadius, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 3);
	if (!CharacterTargetTraceResult.bBlockingHit)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(UClimbAction::NarrowSpace_NarrowSpaceDownToWalk, MontagePlayInofo))
			return false;

		CanNarrowSpaceDownToWalk = true;

		FTransform MotionWarpingTransform;

		FVector MotionWarpingLocation = CharacterLocation + -CharacterForwardVector * CharacterRadius * 2;

		MotionWarpingTransform.SetLocation(MotionWarpingLocation);

		if (bDrawDebug)
			DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

		FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
		MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted MontageBlendingOutDelegate;
		MontageBlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				SetUpDefaultState();
			});
		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(MontageBlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return CanNarrowSpaceDownToWalk;
}

bool UClimbComponent::LedgeWalkUpInsideCornerCheck(bool IsRightWalk)
{
	bool CanLedgeWalkUpInsideCorner = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanLedgeWalkUpInsideCorner;

	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	float LedgeWalkRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.5;

	FVector UpInsideCornerTraceStart = CharacterLocation;
	FVector UpInsideCornerTraceEnd = UpInsideCornerTraceStart + 
									 CharacterForwardVector * (CharacterRadius + 40);

	FHitResult  UpInsideCornerTraceResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, UpInsideCornerTraceStart, UpInsideCornerTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

	if(UpInsideCornerTraceResult.bBlockingHit)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(IsRightWalk ? UClimbAction::LedgeWalkRight_UpInsideCorner : UClimbAction::LedgeWalkLeft_UpInsideCorner, MontagePlayInofo))
			return false;

		CanLedgeWalkUpInsideCorner = true;

		FTransform MotionWarpingTransform;

		FVector MotionWarpingLocation = UpInsideCornerTraceResult.Location +
										UpInsideCornerTraceResult.Normal * (LedgeWalkRadius + 10) +
										ObstacleNormalDir * (CharacterRadius + 10);

		MotionWarpingTransform.SetLocation(MotionWarpingLocation);

		if (bDrawDebug)
			DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

		FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
		MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted MontageBlendingOutDelegate;
		MontageBlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				bool IsRightWalk = (ClimbState == UClimbState::LedgeWalkRight);
				ObstacleDetectionLedgeWalk(IsRightWalk, ObstacleLocation, ObstacleNormalDir);
			});
		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(MontageBlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return CanLedgeWalkUpInsideCorner;
}

bool UClimbComponent::LedgeWalkDownInsideCornerCheck(bool IsRightWalk)
{
	bool CanLedgeWalkDownInsideCorner = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanLedgeWalkDownInsideCorner;

	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	float LedgeWalkRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.5;

	FVector DownInsideCornerTraceStart = CharacterLocation;
	FVector DownInsideCornerTraceEnd = DownInsideCornerTraceStart +
									   -CharacterForwardVector * (CharacterRadius + 40);

	FHitResult DownInsideCornerTraceResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, DownInsideCornerTraceStart, DownInsideCornerTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

	if(DownInsideCornerTraceResult.bBlockingHit)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(IsRightWalk ? UClimbAction::LedgeWalkRight_DownInsideCorner : UClimbAction::LedgeWalkLeft_DownInsideCorner, MontagePlayInofo))
			return false;

		CanLedgeWalkDownInsideCorner = true;

		FTransform MotionWarpingTransform;

		FVector MotionWarpingLocation = DownInsideCornerTraceResult.Location +
										DownInsideCornerTraceResult.Normal * (LedgeWalkRadius + 10) +
										ObstacleNormalDir * (CharacterRadius + 10);

		MotionWarpingTransform.SetLocation(MotionWarpingLocation);

		if (bDrawDebug)
			DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

		FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
		MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted MontageBlendingOutDelegate;
		MontageBlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				bool IsRightWalk = (ClimbState == UClimbState::LedgeWalkRight);
				ObstacleDetectionLedgeWalk(IsRightWalk, ObstacleLocation, ObstacleNormalDir);
			});
		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(MontageBlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return CanLedgeWalkDownInsideCorner;
}

bool UClimbComponent::LedgeWalkUpOutwardCornerCheck(bool IsRightWalk)
{
	bool CanLedgeWalkRightUpOutwardCorner = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanLedgeWalkRightUpOutwardCorner;

	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector TraceVector = IsRightWalk ? CharacterRightVector : -CharacterRightVector;
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	float LedgeWalkRightRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.5;

	FVector UpOutwardCornerTraceStart = CharacterLocation + CharacterForwardVector * CharacterRadius * 0.85;
	FVector UpOutwardCornerTraceEnd = UpOutwardCornerTraceStart + TraceVector * (CharacterRadius + 10);

	FHitResult UpOutsideCornerTraceResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, UpOutwardCornerTraceStart, UpOutwardCornerTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

	if(!UpOutsideCornerTraceResult.bBlockingHit)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(IsRightWalk ? UClimbAction::LedgeWalkRight_UpOutwardCorner : UClimbAction::LedgeWalkLeft_UpOutwardCorner, MontagePlayInofo))
			return false;

		CanLedgeWalkRightUpOutwardCorner = true;

		FTransform MotionWarpingTransform;

		FVector MotionWarpingLocation = ObstacleLocation +
										-ObstacleNormalDir * CharacterRadius +
										CharacterForwardVector * (CharacterRadius + 25);

		MotionWarpingTransform.SetLocation(MotionWarpingLocation);

		if (bDrawDebug)
			DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

		FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
		MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted MontageBlendingOutDelegate;
		MontageBlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				bool IsRightWalk = (ClimbState == UClimbState::LedgeWalkRight);
				ObstacleDetectionLedgeWalk(IsRightWalk, ObstacleLocation, ObstacleNormalDir);
			});
		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(MontageBlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return CanLedgeWalkRightUpOutwardCorner;
}

bool UClimbComponent::LedgeWalkDownOutwardCornerCheck(bool IsRightWalk)
{
	bool CanLedgeWalkDownOutwardCorner = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanLedgeWalkDownOutwardCorner;

	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector TraceVector = IsRightWalk ? CharacterRightVector : -CharacterRightVector;
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	float LedgeWalkRightRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.65;

	FVector DownOutwardCornerTraceStart = CharacterLocation +
										  -CharacterForwardVector * CharacterRadius * 0.5;
	FVector DownOutwardCornerTraceEnd = DownOutwardCornerTraceStart +
										TraceVector * (CharacterRadius + 10);

	FHitResult UpOutwardCornerTraceResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, DownOutwardCornerTraceStart, DownOutwardCornerTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

	if(!UpOutwardCornerTraceResult.bBlockingHit)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(IsRightWalk ? UClimbAction::LedgeWalkRight_DownOutwardCorner : UClimbAction::LedgeWalkLeft_DownOutwardCorner, MontagePlayInofo))
			return false;

		CanLedgeWalkDownOutwardCorner = true;

		FTransform MotionWarpingTransform;

		FVector MotionWarpingLocation = ObstacleLocation +
										-ObstacleNormalDir * CharacterRadius +
										-CharacterForwardVector * (CharacterRadius + 25);

		MotionWarpingTransform.SetLocation(MotionWarpingLocation);

		if (bDrawDebug)
			DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

		FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
		MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted MontageBlendingOutDelegate;
		MontageBlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				bool IsRightWalk = (ClimbState == UClimbState::LedgeWalkRight);
				ObstacleDetectionLedgeWalk(IsRightWalk, ObstacleLocation, ObstacleNormalDir);
			});
		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(MontageBlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}
	return CanLedgeWalkDownOutwardCorner;
}

bool UClimbComponent::LedgeWalkUpLedgeWalkToWalkCheck(bool IsRightWalk)
{
	bool CanUpLedgeWalkToWalk = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanUpLedgeWalkToWalk;

	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector TraceVector = IsRightWalk ? CharacterRightVector : -CharacterRightVector;
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	float CharacterHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	FVector UpLedgeWalkToWalkTraceStart = CharacterLocation +
											CharacterForwardVector * CharacterRadius * 2 +
											-TraceVector * 20 +
											FVector::UpVector * (CharacterHalfHeight - CharacterRadius);

	FVector UpLedgeWalkToWalkTraceEnd = CharacterLocation +
										  CharacterForwardVector * CharacterRadius * 2 +
										  -TraceVector * 20 +
										  FVector::DownVector * (CharacterHalfHeight - CharacterRadius);

	FHitResult UpLedgeWalkToWalkTraceResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, UpLedgeWalkToWalkTraceStart, UpLedgeWalkToWalkTraceEnd, CharacterRadius, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green,3);

	if(!UpLedgeWalkToWalkTraceResult.bBlockingHit)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(IsRightWalk ? UClimbAction::LedgeWalkRight_UpLedgeWalkToWalk : UClimbAction::LedgeWalkLeft_UpLedgeWalkToWalk, MontagePlayInofo))
			return false;

		CanUpLedgeWalkToWalk = true;

		FTransform MotionWarpingTransform;

		FVector MotionWarpingLocation = CharacterLocation +
										CharacterForwardVector * CharacterRadius * 2 +
										-TraceVector * 20;

		MotionWarpingTransform.SetLocation(MotionWarpingLocation);

		if (bDrawDebug)
			DrawDebugSphere(OwnerCharacter->GetWorld(), MotionWarpingLocation, 10, 32, FColor::Yellow, false, 3);

		FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
		MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted MontageBlendingOutDelegate;
		MontageBlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				SetUpDefaultState();
			});
		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(MontageBlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}
	return CanUpLedgeWalkToWalk;
}

bool UClimbComponent::LedgeWalkDownLedgeWalkToWalkCheck(bool IsRightWalk)
{
	bool CanDownLedgeWalkToWalk = false;

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return CanDownLedgeWalkToWalk;

	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector TraceVector = IsRightWalk ? CharacterRightVector : -CharacterRightVector;
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	float CharacterHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	FVector DownLedgeWalkToWalkTraceStart = CharacterLocation +
											-CharacterForwardVector * CharacterRadius * 2 + 
											-TraceVector * 20 +
											FVector::UpVector * (CharacterHalfHeight - CharacterRadius);

	FVector DownLedgeWalkToWalkTraceEnd = CharacterLocation +
										  -CharacterForwardVector * CharacterRadius * 2 +
										  -TraceVector * 20 +
										  FVector::DownVector * (CharacterHalfHeight - CharacterRadius);

	FHitResult DownLedgeWalkToWalkTraceResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, DownLedgeWalkToWalkTraceStart, DownLedgeWalkToWalkTraceEnd, CharacterRadius,TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green,3);

	if(!DownLedgeWalkToWalkTraceResult.bBlockingHit)
	{
		FMontagePlayInofo MontagePlayInofo;

		if (!FindMontagePlayInofoByClimbAction(IsRightWalk ? UClimbAction::LedgeWalkRight_DownLedgeWalkToWalk : UClimbAction::LedgeWalkLeft_DownLedgeWalkToWalk, MontagePlayInofo))
			return false;

		CanDownLedgeWalkToWalk = true;

		ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

		FOnMontageBlendingOutStarted MontageBlendingOutDelegate;
		MontageBlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
			{
				SetUpDefaultState();
			});
		ClimbingAnimInstance->Montage_SetBlendingOutDelegate(MontageBlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
	}

	return CanDownLedgeWalkToWalk;
}

FVector UClimbComponent::GetFootLocation() const
{
	return OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorUpVector() * -1 * OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() ;
}

FVector UClimbComponent::GetTopLocation() const
{
	return OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorUpVector() * OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

void UClimbComponent::FindClimbingRotationUp()
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	FVector UnitOLToOEL = (ObstacleEndLocation - ObstacleLocation).GetSafeNormal();

	FVector FindClimbingRotationRightVector = FVector::CrossProduct(UnitOLToOEL, ObstacleNormalDir) * -1;
	FVector FindClimbingRotationUpVector = FVector::CrossProduct(FindClimbingRotationRightVector, ObstacleNormalDir);
	FVector FindClimbingRotationForWardVector = ObstacleNormalDir * -1;

	ClimbingRotation = UKismetMathLibrary::MakeRotationFromAxes(FindClimbingRotationForWardVector, FindClimbingRotationRightVector, FindClimbingRotationUpVector);

	ClimbingUpVector = FindClimbingRotationUpVector.GetSafeNormal();
	ClimbingRightVector = FindClimbingRotationRightVector.GetSafeNormal();
	ClimbingForwardVector = FindClimbingRotationForWardVector.GetSafeNormal();
}

void UClimbComponent::FindClimbingRotationRight()
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	FVector UnitOLToOEL = (ObstacleEndLocation - ObstacleLocation).GetSafeNormal();

	FVector FindClimbingRotationUpVector = FVector::CrossProduct(UnitOLToOEL, ObstacleNormalDir);
	FVector FindClimbingRotationRightVector = FVector::CrossProduct(FindClimbingRotationUpVector, ObstacleNormalDir) * -1;
	FVector FindClimbingRotationForWardVector = ObstacleNormalDir * -1;

	ClimbingRotation = UKismetMathLibrary::MakeRotationFromAxes(FindClimbingRotationForWardVector, FindClimbingRotationRightVector, FindClimbingRotationUpVector);

	ClimbingUpVector = FindClimbingRotationUpVector.GetSafeNormal();
	ClimbingRightVector = FindClimbingRotationRightVector.GetSafeNormal();
	ClimbingForwardVector = FindClimbingRotationForWardVector.GetSafeNormal();
}

void UClimbComponent::FindClimbingRotationLeft()
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	FVector UnitOLToOEL = (ObstacleEndLocation - ObstacleLocation).GetSafeNormal();

	FVector FindClimbingRotationUpVector = FVector::CrossProduct(UnitOLToOEL, ObstacleNormalDir) * -1;
	FVector FindClimbingRotationRightVector = FVector::CrossProduct(FindClimbingRotationUpVector, ObstacleNormalDir) * -1;
	FVector FindClimbingRotationForWardVector = ObstacleNormalDir * -1;

	ClimbingRotation = UKismetMathLibrary::MakeRotationFromAxes(FindClimbingRotationForWardVector, FindClimbingRotationRightVector, FindClimbingRotationUpVector);

	ClimbingUpVector = FindClimbingRotationUpVector.GetSafeNormal();
	ClimbingRightVector = FindClimbingRotationRightVector.GetSafeNormal();
	ClimbingForwardVector = FindClimbingRotationForWardVector.GetSafeNormal();
}

void UClimbComponent::FindClimbingRotationDown()
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	FVector UnitOLToOEL = (ObstacleEndLocation - ObstacleLocation).GetSafeNormal();

	FVector FindClimbingRotationRightVector = FVector::CrossProduct(UnitOLToOEL, ObstacleNormalDir);
	FVector FindClimbingRotationUpVector = FVector::CrossProduct(FindClimbingRotationRightVector, ObstacleNormalDir);
	FVector FindClimbingRotationForWardVector = ObstacleNormalDir * -1;

	ClimbingRotation = UKismetMathLibrary::MakeRotationFromAxes(FindClimbingRotationForWardVector, FindClimbingRotationRightVector, FindClimbingRotationUpVector);

	ClimbingUpVector = FindClimbingRotationUpVector.GetSafeNormal();
	ClimbingRightVector = FindClimbingRotationRightVector.GetSafeNormal();
	ClimbingForwardVector = FindClimbingRotationForWardVector.GetSafeNormal();
}

void UClimbComponent::FindClimbingRotationIdle()
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	FVector FindClimbingRotationForWardVector = ObstacleNormalDir * -1;
	FVector FindClimbingRotationUpVector = FVector::CrossProduct(OwnerCharacter->GetActorRightVector(), ObstacleNormalDir);
	FVector FindClimbingRotationRightVector = FVector::CrossProduct(FindClimbingRotationUpVector, ObstacleNormalDir) * -1;

	ClimbingRotation = UKismetMathLibrary::MakeRotationFromAxes(FindClimbingRotationForWardVector, FindClimbingRotationRightVector, FindClimbingRotationUpVector);

	ClimbingUpVector = FindClimbingRotationUpVector.GetSafeNormal();
	ClimbingRightVector = FindClimbingRotationRightVector.GetSafeNormal();
	ClimbingForwardVector = FindClimbingRotationForWardVector.GetSafeNormal();
}

void UClimbComponent::FindBalanceRotationIdle(bool UseNormal)
{
	FVector BalanceRotationUpVector;
	FVector BalanceRotationFowWardVector;
	FVector BalanceRotationRightVector;
	
	if(UseNormal)
	{
		BalanceRotationRightVector = FloorEndNormalDir;
		BalanceRotationFowWardVector = FVector::CrossProduct(BalanceRotationRightVector,OwnerCharacter->GetActorUpVector());
		BalanceRotationUpVector = FVector::CrossProduct(BalanceRotationFowWardVector, BalanceRotationRightVector);
	}
	else
	{
		BalanceRotationUpVector = (OwnerCharacter->GetActorLocation() - FloorLocation).GetSafeNormal();
		BalanceRotationFowWardVector = FVector::CrossProduct(BalanceRotationUpVector, -OwnerCharacter->GetActorRightVector());
		BalanceRotationRightVector = FVector::CrossProduct(BalanceRotationUpVector, BalanceRotationFowWardVector);
	}

	FRotator CurrentRotation = OwnerCharacter->GetActorRotation();
	FRotator TargetRotation = UKismetMathLibrary::MakeRotationFromAxes(BalanceRotationFowWardVector, BalanceRotationRightVector, BalanceRotationUpVector);

	BalanceRotation = TargetRotation;
}

//void UClimbComponent::FindNarrowSpaceRotationUp()
//{
//	FVector CharacterTargetUpVector = OwnerCharacter->GetActorUpVector();
//	FVector CharacterTargetForwardVector = (ObstacleEndLocation - ObstacleLocation).GetSafeNormal();
//	FVector CharacterTargetRightVector = -FVector::CrossProduct(CharacterTargetForwardVector, CharacterTargetUpVector).GetSafeNormal();
//
//	NarrowSpaceRotation = UKismetMathLibrary::MakeRotationFromAxes(CharacterTargetForwardVector, CharacterTargetRightVector, CharacterTargetUpVector);
//}

void UClimbComponent::FindNarrowSpaceRotationIdle()
{
	FVector CharacterTargetRightVector = -ObstacleNormalDir;
	FVector CharacterTargetUpVector = OwnerCharacter->GetActorUpVector();
	FVector CharacterTargetForwardVector = FVector::CrossProduct(CharacterTargetRightVector, CharacterTargetUpVector).GetSafeNormal();

	NarrowSpaceRotation = UKismetMathLibrary::MakeRotationFromAxes(CharacterTargetForwardVector, CharacterTargetRightVector, CharacterTargetUpVector);
}

void UClimbComponent::FindLedgeWalkRotationIdle(bool IsRightWalk)
{
	FVector CharacterTargetRightVector = IsRightWalk ? -ObstacleNormalDir : ObstacleNormalDir;
	FVector CharacterTargetUpVector = OwnerCharacter->GetActorUpVector();
	FVector CharacterTargetForwardVector = FVector::CrossProduct(CharacterTargetRightVector, CharacterTargetUpVector).GetSafeNormal();

	LedgeWalkRotation = UKismetMathLibrary::MakeRotationFromAxes(CharacterTargetForwardVector, CharacterTargetRightVector, CharacterTargetUpVector);
}

void UClimbComponent::HandleClimbLerpTransfor(float DeltaTime)
{
	if (ClimbingAnimInstance)
	{
		UClass* ActorClass = ClimbingAnimInstance->GetClass();
		if (ActorClass->ImplementsInterface(UIAnimInt::StaticClass()))
		{
			float ClimbingDirection = UKismetMathLibrary::Conv_VectorToRotator(FVector(MovementInput.Y, MovementInput.X, 0)).Yaw;

			IIAnimInt::Execute_INT_Direction(ClimbingAnimInstance, ClimbingDirection);

			IIAnimInt::Execute_INT_Input(ClimbingAnimInstance, MovementInput);
		}

	}

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = ObstacleLocation + (OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() + 10) * ObstacleNormalDir;


	FVector ClimbLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, 10);
	OwnerCharacter->SetActorLocationAndRotation(ClimbLocation, ClimbingRotation, true);
}

void UClimbComponent::HandleClimbPipeLerpTransfor(float DeltaTime)
{
	if (ClimbingAnimInstance)
	{
		UClass* ActorClass = ClimbingAnimInstance->GetClass();
		if (ActorClass->ImplementsInterface(UIAnimInt::StaticClass()))
		{
			float ClimbingDirection = UKismetMathLibrary::Conv_VectorToRotator(FVector(MovementInput.Y, MovementInput.X, 0)).Yaw;

			IIAnimInt::Execute_INT_Direction(ClimbingAnimInstance, ClimbingDirection);
		}
	}

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = ObstacleLocation + (OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius()) * ObstacleNormalDir;


	FVector ClimbLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, 10);
	OwnerCharacter->SetActorLocationAndRotation(ClimbLocation, ClimbingRotation, true);
}

void UClimbComponent::HandleHangingLerpTransfor(float DeltaTime)
{
	if (ClimbingAnimInstance)
	{
		UClass* ActorClass = ClimbingAnimInstance->GetClass();
		if (ActorClass->ImplementsInterface(UIAnimInt::StaticClass()))
		{
			float ClimbingDirection = UKismetMathLibrary::Conv_VectorToRotator(FVector(MovementInput.Y, MovementInput.X, 0)).Yaw;

			IIAnimInt::Execute_INT_Direction(ClimbingAnimInstance, ClimbingDirection);
		}
	}

	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	float CharaterHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = ObstacleLocation +
							 ObstacleNormalDir * 5 + 
							 FVector::UpVector * -(GHangingTraceOffsetZ + CharaterHalfHeight);

	FVector ClimbLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, 10);
	OwnerCharacter->SetActorLocationAndRotation(ClimbLocation, ClimbingRotation, true);
}

void UClimbComponent::HandleBalanceLerpTransfor(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = FloorLocation + (OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) * FloorNormalDir;

	FVector BalanceLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, 10);
	OwnerCharacter->SetActorLocationAndRotation(BalanceLocation,BalanceRotation,true);
}

void UClimbComponent::HandleNarrowSpaceLerpTransfor(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	float NarrowSpaceRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.5;

	FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = ObstacleLocation + ObstacleNormalDir * NarrowSpaceRadius;

	FVector NarrowSpaceTargetLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, 10);

	OwnerCharacter->SetActorLocationAndRotation(NarrowSpaceTargetLocation,NarrowSpaceRotation);
}

void UClimbComponent::HandleLedgeWalkLerpTransfor(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	float LedgeWalkRightRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.5;

	FVector CurrentLocation = OwnerCharacter->GetActorLocation();
	FVector TargetLocation = ObstacleLocation + ObstacleNormalDir * (LedgeWalkRightRadius + 10);

	FVector  LedgeWalkRightTargetLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, 10);
	OwnerCharacter->SetActorLocationAndRotation(LedgeWalkRightTargetLocation, LedgeWalkRotation);
}

void UClimbComponent::HandleClimbMoveInput()
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	FVector ForwardDirection = ClimbingUpVector;
	FVector RightDirection = ClimbingRightVector;

	OwnerCharacter->AddMovementInput(ForwardDirection, MovementInput.Y);
	OwnerCharacter->AddMovementInput(RightDirection, MovementInput.X);
}

void UClimbComponent::HandleClimbPipeMoveInput()
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	FVector ObstacleToEndDirection =(ObstacleEndLocation -  ObstacleLocation).GetSafeNormal();
	FVector ForwardDirection = ClimbingUpVector;

 	float Degree = UKismetMathLibrary::DegAcos(FMath::Abs(FVector::DotProduct(ForwardDirection, -ObstacleToEndDirection)));

	if(Degree <= 10)
	{
		OwnerCharacter->AddMovementInput(ForwardDirection, MovementInput.Y);
	}
}

void UClimbComponent::HandleHangingMoveInput()
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	FVector RightDirection = OwnerCharacter->GetActorRightVector();

	OwnerCharacter->AddMovementInput(RightDirection, MovementInput.X);
}

void UClimbComponent::HandleBalanceMoveInput()
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if (ClimbingAnimInstance)
	{
		UClass* ActorClass = ClimbingAnimInstance->GetClass();
		if (ActorClass->ImplementsInterface(UIAnimInt::StaticClass()))
		{
			IIAnimInt::Execute_INT_Input(ClimbingAnimInstance, MovementInput);
		}
	}
}

void UClimbComponent::HandleNarrowSpaceMoveInput()
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if (ClimbingAnimInstance)
	{
		UClass* ActorClass = ClimbingAnimInstance->GetClass();
		if (ActorClass->ImplementsInterface(UIAnimInt::StaticClass()))
		{
			IIAnimInt::Execute_INT_Input(ClimbingAnimInstance, MovementInput);
		}
	}
}

void UClimbComponent::HandleLedgeWalkMoveInput()
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if (ClimbingAnimInstance)
	{
		UClass* ActorClass = ClimbingAnimInstance->GetClass();
		if (ActorClass->ImplementsInterface(UIAnimInt::StaticClass()))
		{
			IIAnimInt::Execute_INT_Input(ClimbingAnimInstance, MovementInput);
		}
	}
}

void UClimbComponent::DefaultObstacleCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if(JumpState != UJumpState::Idle)
		return;

	if (ClimbingMovementComponent->MovementMode == EMovementMode::MOVE_Falling)
	{
		ObstacleCheckDefault(DeltaTime);
	}
	else if(ClimbingMovementComponent->MovementMode == EMovementMode::MOVE_Walking)
	{
		DefaultFloorCheck(DeltaTime);
		DefaultNarrowSpaceCheck(DeltaTime);
	}
}

void UClimbComponent::DefaultFloorCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if (ClimbState != UClimbState::Default)
		return;

	float MiddleTraceRadius = 10;

	FVector CharacterVelocity = ClimbingMovementComponent->Velocity;
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector CharacterUpVector = OwnerCharacter->GetActorUpVector();

	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	float LedgeWalkRadius = CharacterRadius / 2;

	FVector CharacterForwardFoot = GetFootLocation() + CharacterForwardVector * CharacterRadius;
	FVector CharacterForwardLocation = OwnerCharacter->GetActorLocation() + CharacterForwardVector * CharacterRadius;
	FRotator ControllerRotaor = OwnerCharacter->GetControlRotation();

	FVector CharacterForwardVectorIgnoreZ = FVector(CharacterForwardVector.X, CharacterForwardVector.Y,0).GetSafeNormal();

	FVector ControllerRotaorOnlyYawForwardDirection = FRotator(0, ControllerRotaor.Yaw, 0).Quaternion().GetForwardVector();
	FVector ControllerRotaorOnlyYawRightDirection = FRotator(0, ControllerRotaor.Yaw, 0).Quaternion().GetRightVector();

	FVector TargetVector = (ControllerRotaorOnlyYawForwardDirection * MovementInput.Y + ControllerRotaorOnlyYawRightDirection * MovementInput.X).GetSafeNormal();

	if (CharacterVelocity.Size2D() >= 10 &&
		UKismetMathLibrary::DegAcos(FVector::DotProduct(CharacterForwardVectorIgnoreZ, TargetVector)) <= 5 &&
	    ClimbState == UClimbState::Default)
	{
		FVector CharacterLeftFloorTraceStart = CharacterForwardFoot + FVector::UpVector * 30 + -CharacterRightVector * CharacterRadius * GBalanceTraceScale;
		FVector CharacterLeftFloorTraceEnd = CharacterLeftFloorTraceStart + FVector::DownVector * 60;

		FHitResult  CharacterLeftFloorTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, CharacterLeftFloorTraceStart, CharacterLeftFloorTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

		FVector CharacterRightFloorTraceStart = CharacterForwardFoot + FVector::UpVector * 30 + CharacterRightVector * CharacterRadius * GBalanceTraceScale;
		FVector CharacterRightFloorTraceEnd = CharacterRightFloorTraceStart + FVector::DownVector * 60;

		FHitResult  CharacterRightFloorTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, CharacterRightFloorTraceStart, CharacterRightFloorTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

		if(!CharacterLeftFloorTraceCheckHit.bBlockingHit &&
		   !CharacterRightFloorTraceCheckHit.bBlockingHit
		   )
		{
			FVector CharacterMiddleFloorTraceStart = CharacterForwardFoot + FVector::UpVector * 30;
			FVector CharacterMiddleFloorTraceEnd = CharacterMiddleFloorTraceStart + FVector::DownVector * 60;

			FHitResult  CharacterMiddleFloorTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, CharacterMiddleFloorTraceStart, CharacterMiddleFloorTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

			if(CharacterMiddleFloorTraceCheckHit.bBlockingHit)
			{
				FVector FloorTraceEnd = CharacterForwardFoot + FVector::DownVector * 5;

				FVector RightFloorTraceStart = FloorTraceEnd + CharacterRightVector * CharacterRadius * GBalanceTraceScale;
				FHitResult  RightFloorTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, RightFloorTraceStart, FloorTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

				FVector LeftFloorTraceStart = FloorTraceEnd + -CharacterRightVector * CharacterRadius * GBalanceTraceScale;
				FHitResult  LeftFloorTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, LeftFloorTraceStart, FloorTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

				if (RightFloorTraceCheckHit.bBlockingHit &&
					LeftFloorTraceCheckHit.bBlockingHit &&
					FVector::DotProduct(RightFloorTraceCheckHit.Normal, LeftFloorTraceCheckHit.Normal) <= -0.96592583/*Cos(165)*/)
				{
					FMontagePlayInofo MontagePlayInofo;

					if (!FindMontagePlayInofoByClimbAction(UClimbAction::Walk_WalkToBalance, MontagePlayInofo))
						return;

					FVector BalanceRotationRightVector = RightFloorTraceCheckHit.Normal;
					FVector BalanceRotationFowWardVector = FVector::CrossProduct(BalanceRotationRightVector, OwnerCharacter->GetActorUpVector());
					FVector BalanceRotationUpVector = FVector::CrossProduct(BalanceRotationFowWardVector, BalanceRotationRightVector);

					FRotator TargetRotation = UKismetMathLibrary::MakeRotationFromAxes(BalanceRotationFowWardVector, BalanceRotationRightVector, BalanceRotationUpVector);

					FVector LastRightFloorEnd = CharacterMiddleFloorTraceCheckHit.Location;
					FVector LastLeftFloorEnd = CharacterMiddleFloorTraceCheckHit.Location;

					const int TraceUnit = 2;
					const int QuerLength = 10;

					for (int i = 1; i <= QuerLength; i++)
					{
						FVector RightFloorEndTraceStart = CharacterMiddleFloorTraceCheckHit.Location + FVector::UpVector * 30 + CharacterRightVector * TraceUnit * i;
						FVector RightFloorEndTraceEnd = RightFloorEndTraceStart + FVector::DownVector * 60;

						FHitResult RightFloorEndTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, RightFloorEndTraceStart, RightFloorEndTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 5);
						if (RightFloorEndTraceCheckHit.bBlockingHit)
						{
							LastRightFloorEnd = RightFloorEndTraceCheckHit.Location;
						}
						else
						{
							break;
						}
					}

					for (int i = 1; i <= QuerLength; i++)
					{
						FVector LeftFloorEndTraceStart = CharacterMiddleFloorTraceCheckHit.Location + FVector::UpVector * 30 + -CharacterRightVector * TraceUnit * i;
						FVector LeftFloorEndTraceEnd = LeftFloorEndTraceStart + FVector::DownVector * 60;

						FHitResult LeftFloorEndTraceCheckHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, LeftFloorEndTraceStart, LeftFloorEndTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green, 5);
						if (LeftFloorEndTraceCheckHit.bBlockingHit)
						{
							LastLeftFloorEnd = LeftFloorEndTraceCheckHit.Location;
						}
						else
						{
							break;
						}
					}

					FVector TargetLocation = (LastLeftFloorEnd + LastRightFloorEnd) * 0.5;

					FTransform MotionWarpingTransform;

					MotionWarpingTransform.SetRotation(TargetRotation.Quaternion());
					MotionWarpingTransform.SetLocation(TargetLocation);

					if (bDrawDebug)
						DrawDebugSphere(OwnerCharacter->GetWorld(), TargetLocation, 10, 32, FColor::Yellow, false, 3);

					FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
					MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

					ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

					FOnMontageBlendingOutStarted BlendingOutDelegate;
					BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
						{
							SetUpBalanceState();
							FloorDectectionBalance(FloorLocation, FloorNormalDir);
							FindBalanceRotationIdle();
						});

					ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
				}
			}
		}
		else if(!CharacterLeftFloorTraceCheckHit.bBlockingHit ||
				!CharacterRightFloorTraceCheckHit.bBlockingHit)
		{
			bool IsRightWalk = !CharacterLeftFloorTraceCheckHit.bBlockingHit;

			FVector TraceVector = IsRightWalk ? CharacterRightVector : -CharacterRightVector;

			FVector WallTraceStart = CharacterForwardLocation;
			FVector WallTraceEnd = WallTraceStart + TraceVector * (CharacterRadius + 5);

			FHitResult WallTraceHit = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, WallTraceStart, WallTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

			if(WallTraceHit.bBlockingHit)
			{
				FMontagePlayInofo MontagePlayInofo;

				if (!FindMontagePlayInofoByClimbAction(IsRightWalk?UClimbAction::Walk_WalkToLedgeWalkRight:UClimbAction::Walk_WalkToLedgeWalkLeft, MontagePlayInofo))
					return;

				FTransform MotionWarpingTransform;

				FVector CharacterTargetLocation = WallTraceHit.Location + WallTraceHit.Normal * (LedgeWalkRadius + 10);
				MotionWarpingTransform.SetLocation(CharacterTargetLocation);

				FVector CharacterTargetRightVector = IsRightWalk?-WallTraceHit.Normal:WallTraceHit.Normal;
				FVector CharacterTargetUpVector = CharacterUpVector;
				FVector CharacterTargetForwardVector = FVector::CrossProduct(CharacterTargetRightVector, CharacterTargetUpVector);

				FRotator CharacterTargetRotation = UKismetMathLibrary::MakeRotationFromAxes(CharacterTargetForwardVector, CharacterTargetRightVector, CharacterTargetUpVector);
				MotionWarpingTransform.SetRotation(CharacterTargetRotation.Quaternion());

				if (bDrawDebug)
					DrawDebugSphere(OwnerCharacter->GetWorld(), CharacterTargetLocation, 10, 32, FColor::Yellow, false, 3);

				FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
				MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

				ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

				SetUpLedgeWalkState(IsRightWalk);

				/*
				FOnMontageBlendingOutStarted BlendingOutDelegate;
				BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
					{

					});

				ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
				*/
			}
		}
		/*
		else if(!CharacterRightFloorTraceCheckHit.bBlockingHit &&
				 CharacterLeftFloorTraceCheckHit.bBlockingHit)
		{

		}
		*/
	}
}

void UClimbComponent::DefaultNarrowSpaceCheck(float DeltaTime)
{
	if (ClimbingAnimInstance->IsAnyMontagePlaying())
		return;

	if (ClimbState != UClimbState::Default)
		return;

	FVector CharacterVelocity = ClimbingMovementComponent->Velocity;
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector CharacterUpVector = OwnerCharacter->GetActorUpVector();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	float CharacterHalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector CharacterForwardLocation = OwnerCharacter->GetActorLocation() + CharacterForwardVector * CharacterRadius;
	FRotator ControllerRotaor = OwnerCharacter->GetControlRotation();

	FVector CharacterForwardVectorIgnoreZ = FVector(CharacterForwardVector.X, CharacterForwardVector.Y, 0).GetSafeNormal();

	FVector ControllerRotaorOnlyYawForwardDirection = FRotator(0, ControllerRotaor.Yaw, 0).Quaternion().GetForwardVector();
	FVector ControllerRotaorOnlyYawRightDirection = FRotator(0, ControllerRotaor.Yaw, 0).Quaternion().GetRightVector();

	FVector TargetVector = (ControllerRotaorOnlyYawForwardDirection * MovementInput.Y + ControllerRotaorOnlyYawRightDirection * MovementInput.X).GetSafeNormal();

	float NarrowSpaceCheckCheckRadius = CharacterRadius / 2;

	if (CharacterVelocity.Size2D() <= 10 &&
		UKismetMathLibrary::DegAcos(FVector::DotProduct(CharacterForwardVectorIgnoreZ, TargetVector)) <= 5 &&
		ClimbState == UClimbState::Default)
		
	{
		FVector NarrowSpaceLeftTraceStart = CharacterForwardLocation + -CharacterRightVector * (CharacterRadius - GNarrowSpaceTraceLength);
		FVector NarrowSpaceLeftTraceEnd = NarrowSpaceLeftTraceStart + -CharacterRightVector * GNarrowSpaceTraceLength * 2;

		FHitResult NarrowSpaceLeftTraceResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, NarrowSpaceLeftTraceStart, NarrowSpaceLeftTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

		FVector NarrowSpaceRightTraceStart = CharacterForwardLocation + CharacterRightVector * (CharacterRadius - GNarrowSpaceTraceLength);
		FVector NarrowSpaceRightTraceEnd = NarrowSpaceRightTraceStart + CharacterRightVector * GNarrowSpaceTraceLength * 2;

		FHitResult NarrowSpaceRightTraceResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, NarrowSpaceRightTraceStart, NarrowSpaceRightTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

		if(NarrowSpaceLeftTraceResult.bBlockingHit && NarrowSpaceRightTraceResult.bBlockingHit)
		{
			FVector NarrowSpaceMiddleTraceStart = CharacterForwardLocation;
			FVector NarrowSpaceMiddleTraceEnd = NarrowSpaceMiddleTraceStart + CharacterForwardVector  * ( 2 * NarrowSpaceCheckCheckRadius);

			FHitResult NarrowSpaceMiddleTraceResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, NarrowSpaceMiddleTraceStart, NarrowSpaceMiddleTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

			if(!NarrowSpaceMiddleTraceResult.bBlockingHit)
			{
				FVector NarrowSpaceTargetRotationTraceStart = NarrowSpaceMiddleTraceStart + CharacterForwardVector  * NarrowSpaceCheckCheckRadius;
				FVector NarrowSpaceTargetRotationTraceEnd = NarrowSpaceTargetRotationTraceStart + CharacterRightVector * (CharacterRadius * 1.5);

				FHitResult NarrowSpaceTargetRotationTraceResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, NarrowSpaceTargetRotationTraceStart, NarrowSpaceTargetRotationTraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

				if(NarrowSpaceTargetRotationTraceResult.bBlockingHit)
				{
					FVector  NarrowSpaceTargetActorLocation = NarrowSpaceTargetRotationTraceResult.Location + NarrowSpaceTargetRotationTraceResult.Normal * (NarrowSpaceCheckCheckRadius + 5);

					FVector NarrowSpaceTargetTraceStart = NarrowSpaceTargetActorLocation + FVector::UpVector * (CharacterHalfHeight - NarrowSpaceCheckCheckRadius);
					FVector  NarrowSpaceTargetTraceEnd = NarrowSpaceTargetActorLocation + FVector::DownVector * (CharacterHalfHeight - NarrowSpaceCheckCheckRadius);

					FHitResult NarrowSpaceTargetTraceResult = UTraceBlueprintFunctionLibrary::SphereTrace(OwnerCharacter, NarrowSpaceTargetTraceStart, NarrowSpaceTargetTraceEnd, NarrowSpaceCheckCheckRadius, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

					if (!NarrowSpaceTargetTraceResult.bBlockingHit)
					{
						FMontagePlayInofo MontagePlayInofo;

						if (!FindMontagePlayInofoByClimbAction(UClimbAction::Walk_WalkToNarrowSpace, MontagePlayInofo))
							return;
								
						FVector CharacterTargetRightVector = -NarrowSpaceTargetRotationTraceResult.Normal;
						FVector CharacterTargetUpVector = CharacterUpVector;
						FVector CharacterTargetForwardVector = FVector::CrossProduct(CharacterTargetRightVector, CharacterTargetUpVector).GetSafeNormal();

						FRotator CharacterTargetRotation = UKismetMathLibrary::MakeRotationFromAxes(CharacterTargetForwardVector, CharacterTargetRightVector, CharacterTargetUpVector);

						FTransform MotionWarpingTransform;

						MotionWarpingTransform.SetRotation(CharacterTargetRotation.Quaternion());
						MotionWarpingTransform.SetLocation(NarrowSpaceTargetActorLocation);

						if (bDrawDebug)
							DrawDebugSphere(OwnerCharacter->GetWorld(), NarrowSpaceTargetActorLocation, 10, 32, FColor::Yellow, false, 3);

						FMotionWarpingTarget MotionWarpingTarget = FMotionWarpingTarget("ClimbTarget", MotionWarpingTransform);
						MotionWarpingComponent->AddOrUpdateWarpTarget(MotionWarpingTarget);

						ClimbingMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying);
						OwnerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
						
						ClimbingAnimInstance->Montage_Play(MontagePlayInofo.AnimMontageToPlay);

						FOnMontageBlendingOutStarted BlendingOutDelegate;
						BlendingOutDelegate.BindLambda([this](UAnimMontage* Montage, bool bInterrupted)
							{
								SetUpNarrowSpaceState();
								ObstacleDetectionNarrowSpace(ObstacleLocation, ObstacleNormalDir);
							});

						ClimbingAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontagePlayInofo.AnimMontageToPlay);
					}
				}
			}
		}
	}
}

void UClimbComponent::SetUpDefaultState()
{
	ClimbState = UClimbState::Default;

	ClimbingMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
	ClimbingMovementComponent->bOrientRotationToMovement = true;
	ClimbingMovementComponent->MaxWalkSpeed = 500;

	if (ClimbingAnimInstance)
	{
		UClass* ActorClass = ClimbingAnimInstance->GetClass();
		if (ActorClass->ImplementsInterface(UIAnimInt::StaticClass()))
		{
			IIAnimInt::Execute_INT_ChangeClimbPosture(ClimbingAnimInstance, ClimbState);
		}
		ClimbingAnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	}

	FRotator CharacterRotation = OwnerCharacter->GetActorRotation();
    if(CharacterRotation.Pitch != 0)
	{
		OwnerCharacter->SetActorRotation(FRotator(0, CharacterRotation.Yaw,0));
	}

	OwnerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Block);
}

void UClimbComponent::SetUpClimbingState()
{
	ClimbState = UClimbState::Climbing;

	ClimbingMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying);
	ClimbingMovementComponent->bOrientRotationToMovement = false;
	ClimbingMovementComponent->MaxFlySpeed = 200;

	if (ClimbingAnimInstance)
	{
		UClass* ActorClass = ClimbingAnimInstance->GetClass();
		if (ActorClass->ImplementsInterface(UIAnimInt::StaticClass()))
		{
			IIAnimInt::Execute_INT_ChangeClimbPosture(ClimbingAnimInstance, ClimbState);
		}
		ClimbingAnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	}

	OwnerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Block);
}

void UClimbComponent::SetUpClimbingPipeState()
{
	ClimbState = UClimbState::ClimbingPipe;

	ClimbingMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying);
	ClimbingMovementComponent->bOrientRotationToMovement = false;
	ClimbingMovementComponent->MaxFlySpeed = 300;

	if (ClimbingAnimInstance)
	{
		UClass* ActorClass = ClimbingAnimInstance->GetClass();
		if (ActorClass->ImplementsInterface(UIAnimInt::StaticClass()))
		{
			IIAnimInt::Execute_INT_ChangeClimbPosture(ClimbingAnimInstance, ClimbState);
		}
		ClimbingAnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	}

	OwnerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Block);
}

void UClimbComponent::SetUpHangingState()
{
	ClimbState = UClimbState::Hanging;

	ClimbingMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying);
	ClimbingMovementComponent->bOrientRotationToMovement = false;
	ClimbingMovementComponent->MaxFlySpeed = 150;

	if (ClimbingAnimInstance)
	{
		UClass* ActorClass = ClimbingAnimInstance->GetClass();
		if (ActorClass->ImplementsInterface(UIAnimInt::StaticClass()))
		{
			IIAnimInt::Execute_INT_ChangeClimbPosture(ClimbingAnimInstance, ClimbState);
		}
		ClimbingAnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	}

	OwnerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Block);

}

void UClimbComponent::SetUpBalanceState()
{
	ClimbState = UClimbState::Balance;

	ClimbingMovementComponent->SetMovementMode(EMovementMode::MOVE_Walking);
	ClimbingMovementComponent->bOrientRotationToMovement = false;
	ClimbingMovementComponent->MaxWalkSpeed = 50;

	if (ClimbingAnimInstance)
	{
		UClass* ActorClass = ClimbingAnimInstance->GetClass();
		if (ActorClass->ImplementsInterface(UIAnimInt::StaticClass()))
		{
			IIAnimInt::Execute_INT_ChangeClimbPosture(ClimbingAnimInstance, ClimbState);
		}
		ClimbingAnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromEverything);
	}

	OwnerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Block);
}

void UClimbComponent::SetUpNarrowSpaceState()
{
	ClimbState = UClimbState::NarrowSpace;

	ClimbingMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying);
	ClimbingMovementComponent->bOrientRotationToMovement = false;
	ClimbingMovementComponent->MaxWalkSpeed = 100;

	if (ClimbingAnimInstance)
	{
		UClass* ActorClass = ClimbingAnimInstance->GetClass();
		if (ActorClass->ImplementsInterface(UIAnimInt::StaticClass()))
		{
			IIAnimInt::Execute_INT_ChangeClimbPosture(ClimbingAnimInstance, ClimbState);
		}
		ClimbingAnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromEverything);
	}

	OwnerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
}

void UClimbComponent::SetUpLedgeWalkState(bool IsRightWalk)
{
	ClimbState = IsRightWalk ? UClimbState::LedgeWalkRight : UClimbState::LedgeWalkLeft;

	ClimbingMovementComponent->SetMovementMode(EMovementMode::MOVE_Flying);
	ClimbingMovementComponent->bOrientRotationToMovement = false;

	if (ClimbingAnimInstance)
	{
		UClass* ActorClass = ClimbingAnimInstance->GetClass();
		if (ActorClass->ImplementsInterface(UIAnimInt::StaticClass()))
		{
			IIAnimInt::Execute_INT_ChangeClimbPosture(ClimbingAnimInstance, ClimbState);
		}
		ClimbingAnimInstance->SetRootMotionMode(ERootMotionMode::RootMotionFromEverything);
	}

	OwnerCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldStatic, ECollisionResponse::ECR_Ignore);
}

bool UClimbComponent::ObstacleEndDetectionUp(float Distance, FVector& Location)
{
	FVector CharactorLocation = OwnerCharacter->GetActorLocation();
	FVector OToCVector = CharactorLocation - ObstacleLocation;

	float CosCToOVectorToObstacleNormalDir = FVector::DotProduct(OToCVector.GetSafeNormal(), ObstacleNormalDir);


	FVector TraceStart = ObstacleLocation + CosCToOVectorToObstacleNormalDir * OToCVector.Length()* ObstacleNormalDir + OwnerCharacter->GetActorUpVector() * OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector TraceEnd = TraceStart + ObstacleNormalDir * Distance * -1;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.Location;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::ObstacleEndDetectionRight(float Distance, FVector& Location)
{
	FVector TraceStart = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorRightVector() * OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector TraceEnd = TraceStart + OwnerCharacter->GetActorForwardVector() * Distance;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.Location;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::ObstacleEndDetectionLeft(float Distance, FVector& Location)
{	
	FVector TraceStart = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorRightVector() * OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius() * -1;
	FVector TraceEnd = TraceStart + OwnerCharacter->GetActorForwardVector() * Distance;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.Location;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::ObstacleEndDetectionDown(float Distance, FVector& Location)
{
	FVector TraceStart = GetFootLocation();
	FVector TraceEnd = TraceStart + OwnerCharacter->GetActorForwardVector() * Distance;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.Location;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::HangingEndDetectionRight(float Distance, FVector& Locatoon)
{
	FVector TreceStart = GetTopLocation() +
						 FVector::UpVector * GHangingTraceOffsetZ +
						 OwnerCharacter->GetActorForwardVector() * -10 +
						 OwnerCharacter->GetActorRightVector() * OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector TraceEnd = OwnerCharacter->GetActorForwardVector() * Distance + TreceStart;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TreceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Locatoon = HitResult.ImpactPoint;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::HangingEndDetectionLeft(float Distance, FVector& Locatoon)
{
	FVector TraceStart = GetTopLocation() +
						 FVector::UpVector * GHangingTraceOffsetZ +
						 OwnerCharacter->GetActorForwardVector() * -10 +
						 OwnerCharacter->GetActorRightVector() * -OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector TraceEnd = OwnerCharacter->GetActorForwardVector() * Distance + TraceStart;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Locatoon = HitResult.ImpactPoint;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::BalanceEndDectionUp(FVector& Location, FVector& Normal)
{
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterForwardFoot = GetFootLocation() + (CharacterRadius) * CharacterForwardVector;

	FVector TraceStart = CharacterForwardFoot + FVector::UpVector * 30;
	FVector TraceEnd = TraceStart + FVector::DownVector * 60;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if(HitResult.bBlockingHit)
	{
		Location = HitResult.Location;
		Normal = FVector::ZeroVector;

		return true;
	}
	else
	{
		Location = FVector::ZeroVector;

		FVector FloorTraceStart = CharacterForwardFoot + FVector::DownVector * 5;

		//TraceLeftFoor
		FVector TraceLeftFoorEnd = FloorTraceStart + CharacterRightVector * -CharacterRadius * 0.5;
		FHitResult TraceLeftFoorHitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, FloorTraceStart, TraceLeftFoorEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

		if(TraceLeftFoorHitResult.bBlockingHit)
		{
			Normal = TraceLeftFoorHitResult.Normal;
		}
		else
		{
			//TraceRightFoor
			FVector TraceRightFoorEnd = FloorTraceStart + CharacterRightVector * CharacterRadius * 0.5;
			FHitResult TraceRightFoorHitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, FloorTraceStart, TraceRightFoorEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

			if(TraceRightFoorHitResult.bBlockingHit)
			{
				Normal = -TraceRightFoorHitResult.Normal;
			}
			else
			{
				Normal = FVector::ZeroVector;
			}
		}
	}

	return false;
}

bool UClimbComponent::BalanceEndDectionDown(FVector& Location, FVector& Normal)
{
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterBackwardFoot = GetFootLocation() + CharacterRadius* -CharacterForwardVector;

	FVector TraceStart = CharacterBackwardFoot + FVector::UpVector * 30;
	FVector TraceEnd = TraceStart + FVector::DownVector * 60;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.Location;
		Normal = FVector::ZeroVector;

		return true;
	}
	else
	{
		Location = FVector::ZeroVector;

		FVector FloorTraceStart = CharacterBackwardFoot + FVector::DownVector * 5;

		//TraceLeftFoor
		FVector TraceLeftFoorEnd = FloorTraceStart + CharacterRightVector * -CharacterRadius * 0.5;
		FHitResult TraceLeftFoorHitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, FloorTraceStart, TraceLeftFoorEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

		if (TraceLeftFoorHitResult.bBlockingHit)
		{
			Normal = TraceLeftFoorHitResult.Normal;
		}
		else
		{
			//TraceRightFoor
			FVector TraceRightFoorEnd = FloorTraceStart + CharacterRightVector * CharacterRadius * 0.5;
			FHitResult TraceRightFoorHitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, FloorTraceStart, TraceRightFoorEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);

			if (TraceRightFoorHitResult.bBlockingHit)
			{
				Normal = -TraceRightFoorHitResult.Normal;
			}
			else
			{
				Normal = FVector::ZeroVector;
			}
		}
	}

	return false;
}

bool UClimbComponent::NarrowSpaceEndDectionUp(FVector& Location, FVector& Normal)
{
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	FVector TraceStart = CharacterLocation +
						 CharacterForwardVector * CharacterRadius;

	FVector TraceEnd = TraceStart +
					   CharacterRightVector * ( CharacterRadius + 5 );

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.ImpactPoint;
		Normal = HitResult.Normal;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::NarrowSpaceEndDectionDown(FVector& Location, FVector& Normal)
{
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterLocation = OwnerCharacter->GetActorLocation();

	FVector TraceStart = CharacterLocation +
						 -CharacterForwardVector * CharacterRadius;

	FVector TraceEnd = TraceStart +
					   CharacterRightVector * (CharacterRadius + 5);

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.ImpactPoint;
		Normal = HitResult.Normal;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::LedgeWalkEndDectionUp(bool IsRightWalk, FVector& Location, FVector& Normal)
{
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterUpVector = OwnerCharacter->GetActorUpVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector TraceVector = IsRightWalk ? CharacterRightVector : -CharacterRightVector;
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterFootLocation = GetFootLocation();

	FVector TraceStart = CharacterFootLocation +
						 CharacterForwardVector * CharacterRadius + 
						 -TraceVector * CharacterRadius +
						 CharacterUpVector * 30;

	FVector TraceEnd = TraceStart +
					   CharacterUpVector * -60;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.ImpactPoint;
		Normal = HitResult.Normal;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::LedgeWalkEndDectionDown(bool IsRightWalk, FVector& Location, FVector& Normal)
{
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();
	FVector CharacterUpVector = OwnerCharacter->GetActorUpVector();
	FVector CharacterRightVector = OwnerCharacter->GetActorRightVector();
	FVector TraceVector = IsRightWalk ? CharacterRightVector : -CharacterRightVector;
	float CharacterRadius = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector CharacterFootLocation = GetFootLocation();

	FVector TraceStart = CharacterFootLocation +
						 -CharacterForwardVector * CharacterRadius +
						 -TraceVector * CharacterRadius +
						 CharacterUpVector * 30;

	FVector TraceEnd = TraceStart +
					   CharacterUpVector * -60;

	FHitResult HitResult = UTraceBlueprintFunctionLibrary::LineTrace(OwnerCharacter, TraceStart, TraceEnd, TArray<AActor*>(), bDrawDebug, FColor::Red, FColor::Green);
	if (HitResult.bBlockingHit)
	{
		Location = HitResult.ImpactPoint;
		Normal = HitResult.Normal;
	}

	return HitResult.bBlockingHit;
}

bool UClimbComponent::FindMontagePlayInofoByClimbAction(UClimbAction ClimbAction, FMontagePlayInofo& outMontagePlayInofo)
{
	if (ClimbMontageAnimConfig == nullptr)
	{
		return false;
	}

	return ClimbMontageAnimConfig->GetMontagePlayInofoByClimbAction(ClimbAction , outMontagePlayInofo);
}

void UClimbComponent::HangingRemapInputVector()
{
	if(ClimbState != UClimbState::Hanging)
		return;

	if(MovementInput.X == 0)
		return;

	FRotator ControllerRotation = OwnerCharacter->GetControlRotation();
	FVector ControllerForwardVector = ControllerRotation.Quaternion().GetForwardVector();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();

	MovementInput.X = (FVector::DotProduct(ControllerForwardVector, CharacterForwardVector) >= 0) ? MovementInput.X : -MovementInput.X;
}

void UClimbComponent::BalanceRemapInputVector()
{
	if (ClimbState != UClimbState::Balance)
		return;

	if (MovementInput.Y == 0)
		return;

	FRotator ControllerRotation = OwnerCharacter->GetControlRotation();
	FVector ControllerForwardVector = ControllerRotation.Quaternion().GetForwardVector();
	FVector CharacterForwardVector = OwnerCharacter->GetActorForwardVector();

	MovementInput.Y = (FVector::DotProduct(ControllerForwardVector, CharacterForwardVector) >= 0) ? MovementInput.Y : -MovementInput.Y;
}

