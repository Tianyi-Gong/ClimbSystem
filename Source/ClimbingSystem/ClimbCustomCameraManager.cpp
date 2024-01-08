// Fill out your copyright notice in the Description page of Project Settings.


#include "ClimbCustomCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "TraceBlueprintFunctionLibrary.h"
#include "Engine/Private/KismetTraceUtils.h"

AClimbCustomCameraManager::AClimbCustomCameraManager()
{
	CameraMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CustomCamera"));
	CameraMesh->SetupAttachment(RootComponent);
	CameraMesh->bHiddenInGame = true;
}

void AClimbCustomCameraManager::BeginPlay()
{
	Super::BeginPlay();
	
	if (CameraAnimInstace == nullptr)
	{
		UAnimInstance* AnimInstace = CameraMesh->GetAnimInstance();

		if (AnimInstace != nullptr)
		{
			UClass* CameraAnimClass = AnimInstace->GetClass();
			if (CameraAnimClass->ImplementsInterface(UClimbCustomCameraInterface::StaticClass()))
			{
				CameraAnimInstace = AnimInstace;
				IClimbCustomCameraInterface::Execute_INT_SetControllerData(CameraAnimInstace, ControlPawn, GetOwningPlayerController());
			}
		}
	}
}

void AClimbCustomCameraManager::INT_OnPossess_Implementation(APawn* ControlledPawn)
{
	UClass* ControlledPawnClass = ControlledPawn->GetClass();
	if(ControlledPawnClass->ImplementsInterface(UClimbCustomCameraInterface::StaticClass()))
	{
		ControlPawn = ControlledPawn;

		if (CameraAnimInstace == nullptr)
		{
			UAnimInstance* AnimInstace = CameraMesh->GetAnimInstance();

			if (AnimInstace != nullptr)
			{
				UClass* CameraAnimClass = AnimInstace->GetClass();
				if (CameraAnimClass->ImplementsInterface(UClimbCustomCameraInterface::StaticClass()))
				{
					CameraAnimInstace = AnimInstace;
					IClimbCustomCameraInterface::Execute_INT_SetControllerData(CameraAnimInstace, ControlPawn, GetOwningPlayerController());
				}
			}
		}
	}
}

void AClimbCustomCameraManager::UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime)
{
	if (OutVT.Target)
	{
		FVector OutLocation;
		FRotator OutRotation;
		float OutFOV;

		if (BlueprintUpdateCamera(OutVT.Target, OutLocation, OutRotation, OutFOV))
		{
			OutVT.POV.Location = OutLocation;
			OutVT.POV.Rotation = OutRotation;
			OutVT.POV.FOV = OutFOV;
		}
		else if(NativeUpdateCamera(OutVT.Target, OutLocation, OutRotation, OutFOV))
		{
			OutVT.POV.Location = OutLocation;
			OutVT.POV.Rotation = OutRotation;
			OutVT.POV.FOV = OutFOV;
		}
		else
		{
			OutVT.Target->CalcCamera(DeltaTime, OutVT.POV);
		}
	}
}

bool AClimbCustomCameraManager::NativeUpdateCamera(AActor* CameraTarget, FVector& NewCameraLocation, FRotator& NewCameraRotation, float& NewCameraFOV)
{
	if(ControlPawn)
	{
		
		bool IsSupportClimbCustomCamera = false;
		IClimbCustomCameraInterface::Execute_INT_PawnIsSupportClimbCustomCamera(ControlPawn, IsSupportClimbCustomCamera);

		float WorldDeltaSeconds = UGameplayStatics::GetWorldDeltaSeconds(this);
		float DebugAlpha = GetCameraBehaviorParam("Override_Debug");

		if(IsSupportClimbCustomCamera)
		{
			//Step 1: Get Camera Parameters from ControlPawn the Camera Interface
			IClimbCustomCameraInterface::Execute_INT_GetCameraParameters(ControlPawn, PivotTarget, FOV, RightShoulder);
			
			//Step 2: Calculate Target Camera Rotation. Use the Control Rotation and interpolate for smooth camera rotation.
			FRotator CurrentCameraRotation = GetCameraRotation();
			FRotator DesiredCameraRotation = GetOwningPlayerController()->GetControlRotation();

			TargetCameraRotation = FMath::RInterpTo(CurrentCameraRotation, DesiredCameraRotation,WorldDeltaSeconds, GetCameraBehaviorParam("RotationLagSpeed"));
			TargetCameraRotation = FMath::Lerp(TargetCameraRotation, DebugViewRotation, DebugAlpha);

			//Step 3: Calculate the Smoothed Pivot Target (Orange Sphere). Get the 3P Pivot Target (Green Sphere) and interpolate using axis independent lag for maximum control.
			FVector LagSpeeds = FVector( GetCameraBehaviorParam("PivotLagSpeed_X"), GetCameraBehaviorParam("PivotLagSpeed_Y"), GetCameraBehaviorParam("PivotLagSpeed_Z"));
			FVector SmoothedPivotTargetLocation = CalculateAxisIndependentLag( SmoothedPivotTarget.GetLocation(), PivotTarget.GetLocation(), TargetCameraRotation, LagSpeeds);
			SmoothedPivotTarget = FTransform(PivotTarget.Rotator(), SmoothedPivotTargetLocation);

			//Step 4: Calculate Pivot Location (BlueSphere). Get the Smoothed Pivot Target and apply local offsets for further camera control.
			FVector SmoothedPivotTargeForwardVector = SmoothedPivotTarget.GetRotation().GetForwardVector();
			FVector SmoothedPivotTargeRightVector = SmoothedPivotTarget.GetRotation().GetRightVector();
			FVector SmoothedPivotTargeUpVector = SmoothedPivotTarget.GetRotation().GetUpVector();

			FVector PivotOffsetForward = SmoothedPivotTargeForwardVector * GetCameraBehaviorParam("PivotOffset_X");
			FVector PivotOffsetRight = SmoothedPivotTargeRightVector * GetCameraBehaviorParam("PivotOffset_Y");
			FVector PivotOffsetUp = SmoothedPivotTargeUpVector * GetCameraBehaviorParam("PivotOffset_Z");

			PivotLocation = SmoothedPivotTarget.GetLocation() + PivotOffsetForward + PivotOffsetRight + PivotOffsetUp;

			//Step 5: Calculate Target Camera Location. Get the Pivot location and apply camera relative offsets.
			FVector TargetCameraRotationForwardVector = TargetCameraRotation.Quaternion().GetForwardVector();
			FVector TargetCameraRotationRightVector = TargetCameraRotation.Quaternion().GetRightVector();
			FVector TargetCameraRotationUpVector = TargetCameraRotation.Quaternion().GetUpVector();

			FVector CameraOffsetForwardVector = TargetCameraRotationForwardVector * GetCameraBehaviorParam("CameraOffset_X");
			FVector CameraOffsetRightVector = TargetCameraRotationRightVector * GetCameraBehaviorParam("CameraOffset_Y");
			FVector CameraOffsetUpVector = TargetCameraRotationUpVector * GetCameraBehaviorParam("CameraOffset_Z");

			TargetCameraLocation = PivotLocation + CameraOffsetForwardVector + CameraOffsetRightVector + CameraOffsetUpVector;

			FVector DebugCameraLocation = PivotTarget.GetLocation() + DebugViewOffset;
			TargetCameraLocation = FMath::Lerp(TargetCameraLocation, DebugCameraLocation, DebugAlpha);

			//Step 6: Trace for an object between the camera and character to apply a corrective offset.Trace origins are set within the Character BP via the Camera Interface.Functions like the normal spring arm, but can allow for different trace origins regardless of the pivot.
			FVector CameraTraceStart = PivotTarget.GetLocation();
			FVector CameraTraceEnd = TargetCameraLocation;

			FHitResult CameraTraceResult = UTraceBlueprintFunctionLibrary::SphereTrace(this, CameraTraceStart, CameraTraceEnd, CameraTrachRadius, {ControlPawn, this}, bDrawDebug, FLinearColor::Red, FLinearColor::Green, 0, ECollisionChannel::ECC_WorldStatic);

			if(CameraTraceResult.IsValidBlockingHit())
			{
				TargetCameraLocation = TargetCameraLocation + (CameraTraceResult.Location - CameraTraceResult.TraceEnd);
			}

			//Step 7: Draw Debug Shapes.
			if(bDrawDebug)
			{
				DrawDebugSphere(this->GetWorld(), PivotTarget.GetLocation(), 16, 8, FColor::Green);
				DrawDebugSphere(this->GetWorld(), SmoothedPivotTarget.GetLocation(), 16, 8, FColor::Orange);
				DrawDebugSphere(this->GetWorld(), PivotLocation, 16, 8, FColor::Purple);
				DrawDebugLine(this->GetWorld(), SmoothedPivotTarget.GetLocation(), PivotTarget.GetLocation(), FColor::Orange);
				DrawDebugLine(this->GetWorld(), PivotLocation, SmoothedPivotTarget.GetLocation(),FColor::Purple);
			}

			//Step 8: Lerp First Person Override and return target camera parameters.
			FTransform TragetCameraTransform = FTransform(TargetCameraRotation, TargetCameraLocation);
			FTransform DebugCameraTransform = FTransform(DebugViewRotation, TargetCameraLocation);
			
			FTransform CameraTransform = UKismetMathLibrary::TLerp(TragetCameraTransform, DebugCameraTransform, DebugAlpha);

			NewCameraLocation = CameraTransform.GetLocation();
			NewCameraRotation = CameraTransform.GetRotation().Rotator();
			NewCameraFOV = FOV;

			return true;
		}
	}

	return false;
}

float AClimbCustomCameraManager::GetCameraBehaviorParam(FName CurveName)
{
	if(CameraAnimInstace == nullptr)
	{
		return 0;
	}

	return CameraAnimInstace->GetCurveValue(CurveName);
}

FVector AClimbCustomCameraManager::CalculateAxisIndependentLag(FVector CurrentLocation, FVector TargetLocation, FRotator CameraRotation, FVector LagSpeeds)
{
	float WorldDeltaSeconds = UGameplayStatics::GetWorldDeltaSeconds(this);

	FRotator CameraRotationYaw = FRotator(0, 0, CameraRotation.Yaw);

	FVector UnrotateCurrentLocation = CameraRotationYaw.UnrotateVector(CurrentLocation);
	FVector UnrotateTargetLocation = CameraRotationYaw.UnrotateVector(TargetLocation);

	float LerpX = FMath::FInterpTo( UnrotateCurrentLocation.X, UnrotateTargetLocation.X, WorldDeltaSeconds, LagSpeeds.X);
	float LerpY = FMath::FInterpTo( UnrotateCurrentLocation.Y, UnrotateTargetLocation.Y, WorldDeltaSeconds, LagSpeeds.Y);
	float LerpZ = FMath::FInterpTo( UnrotateCurrentLocation.Z, UnrotateTargetLocation.Z, WorldDeltaSeconds, LagSpeeds.Z);

	return CameraRotationYaw.RotateVector(FVector(LerpX, LerpY, LerpZ));
}
