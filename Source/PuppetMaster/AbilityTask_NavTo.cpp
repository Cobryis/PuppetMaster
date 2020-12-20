// Copyright Epic Games, Inc. All Rights Reserved.

#include "AbilityTask_NavTo.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"

#include "NavigationSystem.h"

#include "Net/UnrealNetwork.h"

UAbilityTask_NavTo* UAbilityTask_NavTo::NavTo(UGameplayAbility* OwningAbility, FName TaskInstanceName, const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	UAbilityTask_NavTo* MyObj = NewAbilityTask<UAbilityTask_NavTo>(OwningAbility, TaskInstanceName);
	MyObj->TargetDataHandle = TargetDataHandle;

	return MyObj;
}

namespace
{
	UPathFollowingComponent* InitNavigationControl(AController& Controller)
	{
		AAIController* AsAIController = Cast<AAIController>(&Controller);
		UPathFollowingComponent* PathFollowingComp = nullptr;

		if (AsAIController)
		{
			PathFollowingComp = AsAIController->GetPathFollowingComponent();
		}
		else
		{
			PathFollowingComp = Controller.FindComponentByClass<UPathFollowingComponent>();
			if (PathFollowingComp == nullptr)
			{
				PathFollowingComp = NewObject<UPathFollowingComponent>(&Controller);
				PathFollowingComp->RegisterComponentWithWorld(Controller.GetWorld());
				PathFollowingComp->Initialize();
			}
		}

		return PathFollowingComp;
	}
}

void UAbilityTask_NavTo::Activate()
{
	TSharedPtr<FGameplayAbilityActorInfo> ActorInfo = AbilitySystemComponent->AbilityActorInfo;
	AController* Controller = ActorInfo->PlayerController.Get();
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (!TargetDataHandle.IsValid(0) || (Controller == nullptr) || (NavSys == nullptr))
	{
		ensure(false); // #todo: print an error
		EndTask();
		return;
	}

	SetWaitingOnAvatar();

	const FVector TargetLocation = TargetDataHandle.Get(0)->GetEndPoint();

	PathFollowingComponent = InitNavigationControl(*Controller);
	check(PathFollowingComponent.IsValid());
	check(PathFollowingComponent->IsPathFollowingAllowed());

	const bool bAlreadyAtGoal = PathFollowingComponent->HasReached(TargetLocation, EPathFollowingReachMode::OverlapAgentAndGoal);

	// script source, keep only one move request at time
	if (PathFollowingComponent->GetStatus() != EPathFollowingStatus::Idle)
	{
		PathFollowingComponent->AbortMove(*NavSys, FPathFollowingResultFlags::ForcedScript | FPathFollowingResultFlags::NewRequest, FAIRequestID::AnyRequest, bAlreadyAtGoal ? EPathFollowingVelocityMode::Reset : EPathFollowingVelocityMode::Keep);
	}

	check(!FollowHandle.IsValid()); // this handle should have been cleared before this point
	// bind this after the previous move was aborted and before we potentially request with immediate finish
	FollowHandle = PathFollowingComponent->OnRequestFinished.AddUObject(this, &UAbilityTask_NavTo::OnMoveRequestFinished);

	if (bAlreadyAtGoal)
	{
		PathFollowingComponent->RequestMoveWithImmediateFinish(EPathFollowingResult::Success);
	}
	else
	{
		const FVector AgentNavLocation = Controller->GetNavAgentLocation();
		const ANavigationData* NavData = NavSys->GetNavDataForProps(Controller->GetNavAgentPropertiesRef(), AgentNavLocation);
		if (NavData)
		{
			FPathFindingQuery Query(Controller, *NavData, AgentNavLocation, TargetLocation);
			FPathFindingResult Result = NavSys->FindPathSync(Query);
			if (Result.IsSuccessful())
			{
				PathFollowingComponent->RequestMove(FAIMoveRequest(TargetLocation), Result.Path);
			}
			else if (PathFollowingComponent->GetStatus() != EPathFollowingStatus::Idle)
			{
				PathFollowingComponent->RequestMoveWithImmediateFinish(EPathFollowingResult::Invalid);
			}
		}
	}
}

void UAbilityTask_NavTo::OnMoveRequestFinished(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (Result.Code == EPathFollowingResult::Success)
	{
		OnNavSuccess.Broadcast();
	}
	else if (Result.Code == EPathFollowingResult::Aborted)
	{
		OnNavAborted.Broadcast();
	}

	if (PathFollowingComponent.IsValid() && FollowHandle.IsValid())
	{
		PathFollowingComponent->OnRequestFinished.Remove(FollowHandle);
	}

	FollowHandle.Reset();

	EndTask();
}

void UAbilityTask_NavTo::OnDestroy(bool AbilityIsEnding)
{
	if (PathFollowingComponent.IsValid() && FollowHandle.IsValid())
	{
		PathFollowingComponent->OnRequestFinished.Remove(FollowHandle);
	}

	Super::OnDestroy(AbilityIsEnding);
}

void UAbilityTask_NavTo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAbilityTask_NavTo, TargetDataHandle);
}
