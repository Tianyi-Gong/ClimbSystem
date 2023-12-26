// Fill out your copyright notice in the Description page of Project Settings.


#include "HangingTargetAnimNotify.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UHangingTargetAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	ACharacter* NotifyCharacter = Cast<ACharacter>(MeshComp->GetOwner());
	if(NotifyCharacter != nullptr)
	{
		Cast<UCharacterMovementComponent>(NotifyCharacter->GetMovementComponent())->SetMovementMode(EMovementMode::MOVE_Flying);
	}
}
