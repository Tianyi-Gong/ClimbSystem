// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IZipSystem.generated.h"

USTRUCT(BlueprintType)
struct FZipLineData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector ZipLineStartLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ZipLineEndLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector ZipLineHookLocation;
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UIZipSystem : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class CLIMBINGSYSTEM_API IIZipSystem
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ZipSystem)
	void INT_GetZipLineData(FVector HookLocation, FZipLineData& outZipLineData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ZipSystem)
	void INT_SetUpZipLineGliding(const AActor* HookCharacter, const FZipLineData& ZipLineData,float ZOffset, UActorComponent* ClimbComponent);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ZipSystem)
	void INT_StartZiplineGliding();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = ZipSystem)
	void INT_FinishZiplineGliding();
};
