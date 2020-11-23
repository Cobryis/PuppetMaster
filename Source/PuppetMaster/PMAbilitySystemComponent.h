// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTargetActor_Trace.h"

#include "PMAbilitySystemComponent.generated.h"

UCLASS()
class UPMAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:

	UPMAbilitySystemComponent(const FObjectInitializer& OI);

	void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

};

UCLASS()
class AGameplayAbilityTargetActor_Cursor : public AGameplayAbilityTargetActor_Trace
{
	GENERATED_BODY()

protected:

	// the hit target is valid if it has any of these gameplay tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	FGameplayTagContainer RequiredGameplayTags;

	FHitResult PerformTrace(AActor* InSourceActor) override;

	bool IsConfirmTargetingAllowed() override { return bHasValidTarget; }

private:

	bool bHasValidTarget = false;

};
