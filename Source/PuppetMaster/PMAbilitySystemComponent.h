// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

#include "PMAbilitySystemComponent.generated.h"

UENUM()
enum class EAbilityBindings
{
	Ability0,
	Ability1,
	Ability2,
	Ability3,
	Ability4,
	Confirm,
	Cancel,
	Select,
};

UCLASS()
class UPMAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:

	void SetupPlayerInputComponent(UInputComponent& InputComponent);

protected:

	UPMAbilitySystemComponent(const FObjectInitializer& OI);

	void BeginPlay() override;

	void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	void OnPlayerControllerSet() override;

private:

	UPROPERTY()
	TSubclassOf<UGameplayEffect> PlayerDefaultsGE;

};
