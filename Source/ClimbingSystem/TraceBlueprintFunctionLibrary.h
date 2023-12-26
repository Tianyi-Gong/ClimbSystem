// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TraceBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class CLIMBINGSYSTEM_API UTraceBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	static FHitResult LineTrace(const AActor* TraceContext,const FVector& start, const FVector& end, const TArray<AActor*>& InIgnoreActors, bool DebugDraw = false, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawDuration = 0);

	UFUNCTION(BlueprintCallable)
	static FHitResult SphereTrace(const AActor* TraceContext, const FVector& start, const FVector& end, float radius, const TArray<AActor*>& InIgnoreActors, bool DebugDraw = false, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawDuration = 0);

	UFUNCTION(BlueprintCallable)
	static FHitResult BoxTrace(const AActor* TraceContext, const FVector& start, const FVector& end, FRotator rotation, float HalfExtent, const TArray<AActor*>& InIgnoreActors, bool DebugDraw = false, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawDuration = 0);

	UFUNCTION(BlueprintCallable)
	static FHitResult CapsuleTrace(const AActor* TraceContext, const FVector& start, const FVector& end, FRotator rotation, float CapsuleHalfHeight, float CapsuleRadius,const TArray<AActor*>& InIgnoreActors, bool DebugDraw = false, FLinearColor TraceColor = FLinearColor::Red, FLinearColor TraceHitColor = FLinearColor::Green, float DrawDuration = 0);
};
