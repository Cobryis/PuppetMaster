// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTargetActor_Trace.h"

#include "UObject/StrongObjectPtr.h"

#include "GameplayAbility_PlayerCursor.generated.h"

class AGameplayAbilityTargetActor_Cursor;
class UAbilityTask_NavTo;

/**
 * 
 */
UCLASS()
class UGameplayAbility_PlayerCursor : public UGameplayAbility
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AGameplayAbilityWorldReticle> ReticleClass;

private:

	UGameplayAbility_PlayerCursor();

	void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;

	void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	void OnTargetDataReplicatedCallback(const FGameplayAbilityTargetDataHandle& TargetDataHandle, FGameplayTag Tag);

private:

	using GameplayAbilityTargetActorType = AGameplayAbilityTargetActor_Cursor;
	TWeakObjectPtr<GameplayAbilityTargetActorType> CurrentTargetActor;

	UPROPERTY()
	UAbilityTask_NavTo* NavToTask;

};


/**
 * 
 */
UCLASS()
class AGameplayAbilityTargetActor_Cursor : public AGameplayAbilityTargetActor_Trace
{
	GENERATED_BODY()

public:

	void InputPressed();
	void InputReleased();

protected:

	// the hit target is valid if it has any of these gameplay tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	FGameplayTagContainer RequiredGameplayTags;

	AGameplayAbilityTargetActor_Cursor();

	void Tick(float DeltaSeconds) override;
	FHitResult PerformTrace(AActor* InSourceActor) override;

	bool IsConfirmTargetingAllowed() override { return TargetDataHandle.IsValid(0); }
	void ConfirmTargetingAndContinue() override;

private:

	bool bInputPressed = false;

	FGameplayAbilityTargetDataHandle TargetDataHandle;

};
