// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameplayAbility_PlayerCursor.h"

#include "AbilityTask_NavTo.h"

#include "GameplayTagAssetInterface.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityWorldReticle.h"

#include "DrawDebugHelpers.h"


//////////////////////////////////////////////////////////////////////////
//
//	GA_PlayerCursor	
//

UGameplayAbility_PlayerCursor::UGameplayAbility_PlayerCursor()
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateYes;

	// looks lik
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	static ConstructorHelpers::FClassFinder<AGameplayAbilityWorldReticle> ReticleFinder(TEXT("/Game/GAS/Shared/GAR_Cursor"));
	ReticleClass = ReticleFinder.Class;
}

void UGameplayAbility_PlayerCursor::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	check(!CurrentTargetActor.IsValid()); // we should only be activating this one at a time
	if (ActorInfo->IsLocallyControlled())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = ActorInfo->PlayerController.Get();
		SpawnParams.bDeferConstruction = true;

		GameplayAbilityTargetActorType* TargetActor = GetWorld()->SpawnActor<GameplayAbilityTargetActorType>(SpawnParams);
		{
			TargetActor->ReticleClass = ReticleClass;
			TargetActor->MasterPC = ActorInfo->PlayerController.Get();
		}
		TargetActor->FinishSpawning(FTransform::Identity);

		TargetActor->TargetDataReadyDelegate.AddUObject(this, &UGameplayAbility_PlayerCursor::OnTargetDataReady);

		TargetActor->StartTargeting(this);

		CurrentTargetActor = TargetActor;
	}
	else
	{
		const GameplayAbilityTargetActorType* CDO = GetDefault<GameplayAbilityTargetActorType>();
		if (!CDO->ShouldProduceTargetDataOnServer)
		{
			FPredictionKey ActivationPredictionKey = ActivationInfo.GetActivationPredictionKey();
			ActorInfo->AbilitySystemComponent->AbilityTargetDataSetDelegate(Handle, ActivationPredictionKey).AddUObject(this, &UGameplayAbility_PlayerCursor::OnTargetDataReplicatedCallback);
			ActorInfo->AbilitySystemComponent->CallReplicatedTargetDataDelegatesIfSet(Handle, ActivationPredictionKey);
		}
	}
}

void UGameplayAbility_PlayerCursor::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (CurrentTargetActor.IsValid())
	{
		CurrentTargetActor->Destroy();
	}

	if (IsValid(NavToTask))
	{
		NavToTask->EndTask();
	}
}

void UGameplayAbility_PlayerCursor::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (CurrentTargetActor.IsValid())
	{
		CurrentTargetActor->InputPressed();
	}
}

void UGameplayAbility_PlayerCursor::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (CurrentTargetActor.IsValid())
	{
		CurrentTargetActor->InputReleased();
	}
}

void UGameplayAbility_PlayerCursor::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();

	const bool bShouldReplicateDataToServer = !Info->IsNetAuthority() && !CurrentTargetActor->ShouldProduceTargetDataOnServer;
	FScopedPredictionWindow	ScopedPrediction(Info->AbilitySystemComponent.Get(), bShouldReplicateDataToServer);

	if (IsPredictingClient())
	{
		FGameplayTag ApplicationTag;
		Info->AbilitySystemComponent->CallServerSetReplicatedTargetData(GetCurrentAbilitySpecHandle(), GetCurrentActivationInfoRef().GetActivationPredictionKey(), TargetDataHandle, ApplicationTag, Info->AbilitySystemComponent->ScopedPredictionKey);
	}

	NavToTask = UAbilityTask_NavTo::NavTo(this, NAME_None, TargetDataHandle);
	if (IsValid(NavToTask))
	{
		NavToTask->Activate();
	}
}

void UGameplayAbility_PlayerCursor::OnTargetDataReplicatedCallback(const FGameplayAbilityTargetDataHandle& TargetDataHandle, FGameplayTag ActivationTag)
{
	GetCurrentActorInfo()->AbilitySystemComponent->ConsumeClientReplicatedTargetData(GetCurrentAbilitySpecHandle(), GetCurrentActivationInfoRef().GetActivationPredictionKey());

	if (IsActive())
	{
		OnTargetDataReady(TargetDataHandle);
	}
}

//////////////////////////////////////////////////////////////////////////
//
//	GATargetActor_Cursor	
//

void AGameplayAbilityTargetActor_Cursor::InputPressed()
{
	bInputPressed = true;
}

void AGameplayAbilityTargetActor_Cursor::InputReleased()
{
	bInputPressed = false;
}

AGameplayAbilityTargetActor_Cursor::AGameplayAbilityTargetActor_Cursor()
{
	bDestroyOnConfirmation = false;
	TraceProfile = FName(TEXT("Selectable"));
}

void AGameplayAbilityTargetActor_Cursor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bInputPressed && ShouldProduceTargetData())
	{
		ConfirmTargeting();
	}
}

FHitResult AGameplayAbilityTargetActor_Cursor::PerformTrace(AActor* InSourceActor)
{
	FHitResult HitResult;

	bool bHasValidTarget = false;

	FVector ViewStart;
	FVector ViewDir;
	if (OwningAbility->GetCurrentActorInfo()->PlayerController->DeprojectMousePositionToWorld(ViewStart, ViewDir))
	{
		bool bTraceComplex = false;
		TArray<AActor*> ActorsToIgnore;

		ActorsToIgnore.Add(InSourceActor);

		FCollisionQueryParams Params(SCENE_QUERY_STAT(AGameplayAbilityTargetActor_Cursor), bTraceComplex);
		// Params.bReturnPhysicalMaterial = true;
		Params.AddIgnoredActors(ActorsToIgnore);

		FVector TraceStart = ViewStart;

		FVector ViewEnd = ViewStart + (ViewDir * MaxRange);

		ClipCameraRayToAbilityRange(ViewStart, ViewDir, TraceStart, MaxRange, ViewEnd);

		LineTraceWithFilter(HitResult, InSourceActor->GetWorld(), Filter, ViewStart, ViewEnd, TraceProfile.Name, Params);

		const bool bUseTraceResult = HitResult.bBlockingHit && (FVector::DistSquared(TraceStart, HitResult.Location) <= (FMath::Square(MaxRange)));

		const FVector AdjustedEnd = (bUseTraceResult) ? HitResult.Location : ViewEnd;

		if (RequiredGameplayTags.IsEmpty())
		{
			bHasValidTarget = HitResult.bBlockingHit;
		}
		else
		{
			const TScriptInterface<IGameplayTagAssetInterface> ActorTags = HitResult.bBlockingHit ? HitResult.Actor.Get() : nullptr;
			if (ActorTags)
			{
				bHasValidTarget = ActorTags->HasAnyMatchingGameplayTags(RequiredGameplayTags);
			}
		}

		if (AGameplayAbilityWorldReticle* LocalReticleActor = ReticleActor.Get())
		{
			const FVector ReticleLocation = (bHasValidTarget && LocalReticleActor->bSnapToTargetedActor) ? HitResult.Actor->GetActorLocation() : AdjustedEnd;

			LocalReticleActor->SetActorLocation(ReticleLocation);
			LocalReticleActor->SetIsTargetValid(bHasValidTarget);
			LocalReticleActor->SetIsTargetAnActor(bHasValidTarget);
		}

#if 1
		if (bDebug)
		{
			DrawDebugLine(GetWorld(), ViewStart, AdjustedEnd, FColor::Green);
			DrawDebugSphere(GetWorld(), AdjustedEnd, 100.0f, 16, FColor::Green);
		}
#endif // ENABLE_DRAW_DEBUG
	}

	if (bHasValidTarget)
	{
		if (!TargetDataHandle.IsValid(0))
		{
			TargetDataHandle = new FGameplayAbilityTargetData_SingleTargetHit();
		}

		static_cast<FGameplayAbilityTargetData_SingleTargetHit*>(TargetDataHandle.Get(0))->HitResult = HitResult;
	}
	else
	{
		TargetDataHandle.Clear();
	}

	return HitResult;
}

void AGameplayAbilityTargetActor_Cursor::ConfirmTargetingAndContinue()
{
	check(TargetDataHandle.IsValid(0));
	TargetDataReadyDelegate.Broadcast(TargetDataHandle);
}
