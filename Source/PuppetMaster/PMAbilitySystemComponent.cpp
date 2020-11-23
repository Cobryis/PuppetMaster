// Copyright Epic Games, Inc. All Rights Reserved.

#include "PMAbilitySystemComponent.h"

#include "GameplayTagAssetInterface.h"

#include "DrawDebugHelpers.h"

UPMAbilitySystemComponent::UPMAbilitySystemComponent(const FObjectInitializer& OI)
	: Super(OI)
{

}

void UPMAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	// #hack: so the avatar actor isn't set until its the one we want (the character)
	if (InAvatarActor == InOwnerActor)
	{
		InAvatarActor = nullptr;
	}

	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
}


FHitResult AGameplayAbilityTargetActor_Cursor::PerformTrace(AActor* InSourceActor)
{
	FHitResult HitResult;

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

		bHasValidTarget = false;

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

	return HitResult;
}
