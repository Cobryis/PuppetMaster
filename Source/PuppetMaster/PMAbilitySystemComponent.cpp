// Copyright Epic Games, Inc. All Rights Reserved.

#include "PMAbilitySystemComponent.h"

#include "GameplayAbility_PlayerCursor.h"
#include "PMAttributeSets.h"

#define stringify(t) #t

void UPMAbilitySystemComponent::SetupPlayerInputComponent(UInputComponent& InputComponent)
{
	FGameplayAbilityInputBinds AbilityBinds(TEXT(""), TEXT(""), stringify(EAbilityBindings), static_cast<int32>(EAbilityBindings::Confirm), static_cast<int32>(EAbilityBindings::Cancel));
	BindAbilityActivationToInputComponent(&InputComponent, AbilityBinds);
}

UPMAbilitySystemComponent::UPMAbilitySystemComponent(const FObjectInitializer& OI)
	: Super(OI)
{

}

void UPMAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwnerRole() == ROLE_Authority)
	{
		FGameplayAbilitySpec PlayerCursorSpec{ UGameplayAbility_PlayerCursor::StaticClass(), 1, static_cast<int32>(EAbilityBindings::Select) };
		GiveAbility(PlayerCursorSpec);
	}
}

void UPMAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
}

void UPMAbilitySystemComponent::OnPlayerControllerSet()
{
	if (AbilityActorInfo->IsLocallyControlled())
	{
		TryActivateAbilityByClass(UGameplayAbility_PlayerCursor::StaticClass());
	}
}