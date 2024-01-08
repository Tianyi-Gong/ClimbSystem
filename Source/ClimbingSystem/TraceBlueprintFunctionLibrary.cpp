// Fill out your copyright notice in the Description page of Project Settings.


#include "TraceBlueprintFunctionLibrary.h"
#include "Engine/Private/KismetTraceUtils.h"

void UTraceBlueprintFunctionLibrary::FindDeltaAngleDegrees(float StartAngle, float TargetAngle, float& DeltaAngle)
{
	DeltaAngle = FMath::FindDeltaAngleDegrees(StartAngle,TargetAngle);
}

void UTraceBlueprintFunctionLibrary::LimitAngle(float InputAngle, float MinAngle, float MaxAngle, float& OutputAngle)
{
	OutputAngle = FMath::ClampAngle(InputAngle, MinAngle, MaxAngle);
	OutputAngle = FRotator::ClampAxis(OutputAngle);
}

FHitResult UTraceBlueprintFunctionLibrary::LineTrace(const AActor* TraceContext, const FVector& start, const FVector& end, const TArray<AActor*>& InIgnoreActors, bool DebugDraw, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawDuration)
{
	FHitResult hitResult;

	//FCollisionObjectQueryParams CollisionObjectQueryParams(ECC_TO_BITFIELD(ECollisionChannel::ECC_WorldStatic) | ECC_TO_BITFIELD(ECollisionChannel::ECC_WorldDynamic));
	FCollisionObjectQueryParams CollisionObjectQueryParams(ECC_TO_BITFIELD(ECollisionChannel::ECC_WorldStatic));

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActors(InIgnoreActors);

	bool bHit = TraceContext->GetWorld()->LineTraceSingleByObjectType(hitResult, start, end, CollisionObjectQueryParams, CollisionQueryParams);

	if (DebugDraw)
	{
#if ENABLE_DRAW_DEBUG
		DrawDebugLineTraceSingle(TraceContext->GetWorld(), hitResult.TraceStart, hitResult.TraceEnd, EDrawDebugTrace::ForDuration, bHit, hitResult, TraceColor, TraceHitColor, DrawDuration);
#endif
	}

	return hitResult;
}

FHitResult UTraceBlueprintFunctionLibrary::SphereTrace(const AActor* TraceContext, const FVector& start, const FVector& end, float radius,const TArray<AActor*>& InIgnoreActors, bool DebugDraw /*= false*/, FLinearColor TraceColor /*= FLinearColor::Red*/, FLinearColor TraceHitColor /*= FLinearColor::Green*/, float DrawDuration /*= 0*/, ECollisionChannel CollisionChannel /*ECollisionChannel::ECC_WorldStatic*/)
{
	FHitResult hitResult;

	FCollisionObjectQueryParams CollisionObjectQueryParams(ECC_TO_BITFIELD(CollisionChannel));

	FCollisionShape CollisionShape;
	CollisionShape.SetSphere(radius);

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActors(InIgnoreActors);

	bool bHit = TraceContext->GetWorld()->SweepSingleByObjectType(hitResult, start, end, FQuat::Identity, CollisionObjectQueryParams, CollisionShape, CollisionQueryParams);

	if (DebugDraw)
	{
#if ENABLE_DRAW_DEBUG
		DrawDebugSphereTraceSingle(TraceContext->GetWorld(), start, end,radius, EDrawDebugTrace::ForDuration, bHit,hitResult, TraceColor, TraceHitColor, DrawDuration);
#endif
	}

	return hitResult;
}

FHitResult UTraceBlueprintFunctionLibrary::BoxTrace(const AActor* TraceContext, const FVector& start, const FVector& end, FRotator rotation, float HalfExtent, const TArray<AActor*>& InIgnoreActors, bool DebugDraw /*= false*/, FLinearColor TraceColor /*= FLinearColor::Red*/, FLinearColor TraceHitColor /*= FLinearColor::Green*/, float DrawDuration /*= 0*/)
{
	FHitResult hitResult;

	FCollisionObjectQueryParams CollisionObjectQueryParams(ECC_TO_BITFIELD(ECollisionChannel::ECC_WorldStatic));

	FCollisionShape CollisionShape;
	//CollisionShape.SetBox(FVector3f(HalfExtent));
	CollisionShape.SetBox(FVector3f(HalfExtent,HalfExtent, HalfExtent));

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActors(InIgnoreActors);

	bool bHit = TraceContext->GetWorld()->SweepSingleByObjectType(hitResult, start, end, rotation.Quaternion(), CollisionObjectQueryParams, CollisionShape, CollisionQueryParams);

	if (DebugDraw)
	{
#if ENABLE_DRAW_DEBUG
		DrawDebugBoxTraceSingle(TraceContext->GetWorld(),start,end,FVector(HalfExtent), rotation, EDrawDebugTrace::ForDuration, bHit, hitResult, TraceColor, TraceHitColor, DrawDuration);
#endif
	}

	return hitResult;
}

FHitResult UTraceBlueprintFunctionLibrary::CapsuleTrace(const AActor* TraceContext, const FVector& start, const FVector& end, FRotator rotation, float CapsuleHalfHeight, float CapsuleRadius, const TArray<AActor*>& InIgnoreActors, bool DebugDraw, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawDuration )
{
	FHitResult hitResult;

	FCollisionObjectQueryParams CollisionObjectQueryParams(ECC_TO_BITFIELD(ECollisionChannel::ECC_WorldStatic));

	FCollisionShape CollisionShape;
	CollisionShape.SetCapsule(CapsuleRadius, CapsuleHalfHeight);

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActors(InIgnoreActors);

	bool bHit = TraceContext->GetWorld()->SweepSingleByObjectType(hitResult, start, end, rotation.Quaternion(), CollisionObjectQueryParams, CollisionShape, CollisionQueryParams);

	if (DebugDraw)
	{
#if ENABLE_DRAW_DEBUG
	DrawDebugCapsuleTraceSingle(TraceContext->GetWorld(), start,end, CapsuleRadius,CapsuleHalfHeight, EDrawDebugTrace::ForDuration, bHit, hitResult, TraceColor, TraceHitColor, DrawDuration);
#endif
	}

	return hitResult;
}

