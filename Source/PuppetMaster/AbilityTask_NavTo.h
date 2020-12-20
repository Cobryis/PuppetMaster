// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Abilities/Tasks/AbilityTask.h"

#include "Navigation/PathFollowingComponent.h"

#include "AbilityTask_NavTo.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNavToDelegate);

UCLASS()
class UAbilityTask_NavTo : public UAbilityTask
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FNavToDelegate OnNavSuccess;

	UPROPERTY(BlueprintAssignable)
	FNavToDelegate OnNavAborted;

	/** Move to the specified location, using the vector curve (range 0 - 1) if specified, otherwise the float curve (range 0 - 1) or fallback to linear interpolation */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_NavTo* NavTo(UGameplayAbility* OwningAbility, FName TaskInstanceName, const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	void Activate() override;

private:
	void OnDestroy(bool AbilityIsEnding) override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void OnMoveRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& Result);

	UPROPERTY(Replicated)
	FGameplayAbilityTargetDataHandle TargetDataHandle; 
	
	FDelegateHandle FollowHandle;

	TWeakObjectPtr<class UPathFollowingComponent> PathFollowingComponent;

};
