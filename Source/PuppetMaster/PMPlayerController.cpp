// Copyright Epic Games, Inc. All Rights Reserved.

#include "PMPlayerController.h"

#include "PMCharacter.h"

#include "AbilitySystemComponent.h"

#include "DrawDebugHelpers.h"
#include "GameplayTagAssetInterface.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"

DECLARE_LOG_CATEGORY_CLASS(LogPMPlayerController, Warning, All)

APMPlayerController::APMPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void APMPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	StateName = NAME_Inactive;

	if (HasAuthority())
	{
		AbilitySystemActor = GetWorld()->SpawnActor<AAbilitySystemActor>();
	}
}

void APMPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	AController::GetLifetimeReplicatedProps(OutLifetimeProps);

	ResetReplicatedLifetimeProperty(StaticClass(), AController::StaticClass(), TEXT("Pawn"), COND_Never, OutLifetimeProps);

	DOREPLIFETIME(APMPlayerController, SimulatedPawn);
	DOREPLIFETIME_CONDITION(APMPlayerController, AbilitySystemActor, COND_InitialOnly);
}

void APMPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void APMPlayerController::SetSimulatedPawn(APawn* InPawn)
{
	check(!IsValid(InPawn) || InPawn->IsA<APMCharacter>());

	SimulatedPawn = Cast<APMCharacter>(InPawn);

	if (IsValid(AbilitySystemActor))
	{
		AbilitySystemActor->GetAbilitySystemComponent()->SetAvatarActor(SimulatedPawn);
	}

	if (SimulatedPawn)
	{
		SetViewTarget(SimulatedPawn);
	}
}

APawn* APMPlayerController::GetSimulatedPawn() const
{
	return SimulatedPawn;
}

UAbilitySystemComponent* APMPlayerController::GetAbilitySystemComponent() const
{
	return AbilitySystemActor ? AbilitySystemActor->GetAbilitySystemComponent() : nullptr;
}

void APMPlayerController::OnRep_SimulatedPawn()
{
	SetSimulatedPawn(SimulatedPawn);
}

void APMPlayerController::OnRep_AbilitySystemActor()
{
	if (IsValid(AbilitySystemActor))
	{
		if (IsValid(SimulatedPawn))
		{
			AbilitySystemActor->GetAbilitySystemComponent()->SetAvatarActor(SimulatedPawn);
		}

		AbilitySystemActor->SetupInputComponent(InputComponent);
	}
}

void APMPlayerController::ChangeState(FName NewState)
{
	Super::ChangeState((NewState == NAME_Spectating) ? NAME_Inactive : NewState);
}

void APMPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
}

void APMPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	if (HasAuthority())
	{
		AbilitySystemActor->SetupInputComponent(InputComponent);
	}

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &APMPlayerController::InputAction_SelectPressed);
}

void APMPlayerController::SetNewMoveDestination(const FVector& DestLocation)
{
	check(IsValid(SimulatedPawn));

	if (HasAuthority())
	{
		SimulatedPawn->MoveTo(DestLocation);
		
	}
	else
	{
		ServerSetNewMoveDestination(DestLocation);
	}
}

void APMPlayerController::ServerSetNewMoveDestination_Implementation(const FVector& DestLocation)
{
	if (!SimulatedPawn->IsAlive())
	{
		UE_LOG(LogPMPlayerController, Error, TEXT("Attempted to move dead pawn"));
		return;
	}

	SetNewMoveDestination(DestLocation);
}

void APMPlayerController::SetFollowTarget(APMCharacter* Target)
{
	check(IsValid(SimulatedPawn) && SimulatedPawn->IsAlive());

	// #todo: we may want to move to a target regardless of its health
	if (!IsValid(Target) || !Target->IsAlive())
	{
		return;
	}

	if (HasAuthority() && IsValid(Target))
	{
		SimulatedPawn->MoveToActorAndPerformAction(*Target);
	}
	else
	{
		ServerSetFollowTarget(Target);
	}
}

void APMPlayerController::ServerSetFollowTarget_Implementation(APMCharacter* Target)
{
	if (!SimulatedPawn->IsAlive())
	{
		UE_LOG(LogPMPlayerController, Error, TEXT("Attempted to move dead pawn"));
		return;
	}

	SetFollowTarget(Target);
}

void APMPlayerController::InputAction_SelectPressed()
{
	if (IsValid(SimulatedPawn))
	{
		if (!SimulatedPawn->IsAlive())
		{
			UE_LOG(LogPMPlayerController, Error, TEXT("Attempted to move dead pawn"));
			return;
		}

		constexpr auto ECC_Selectable = ECC_GameTraceChannel1;

		// Trace to see what is under the mouse cursor
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Selectable, false, Hit);

		if (Hit.bBlockingHit)
		{
			APMCharacter* PMCharacter = Cast<APMCharacter>(Hit.GetActor());
			if (IsValid(PMCharacter) && (PMCharacter != SimulatedPawn) && PMCharacter->IsAlive())
			{
				SetFollowTarget(static_cast<APMCharacter*>(Hit.GetActor()));
			}
			else
			{
				// We hit something, move there
				SetNewMoveDestination(Hit.ImpactPoint);
			}
		}
	}
}


void APMPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APMPlayerState, MatchStatus);
}

void APMPlayerState::SetReady()
{
	if (MatchStatus == EPlayerMatchStatus::NotReady)
	{
		if (HasAuthority())
		{
			MatchStatus = EPlayerMatchStatus::Ready;
		}
		else
		{
			ServerSetReady();
		}
	}
}

void APMPlayerState::ServerSetReady_Implementation()
{
	SetReady();
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

// 		FVector AdjustedAimDir = (AdjustedEnd - TraceStart).GetSafeNormal();
// 		if (AdjustedAimDir.IsZero())
// 		{
// 			AdjustedAimDir = ViewDir;
// 		}
// 
// 		FVector TraceEnd = TraceStart + (AdjustedAimDir * MaxRange);
// 
// 		// ------------------------------------------------------
// 
// 		LineTraceWithFilter(HitResult, InSourceActor->GetWorld(), Filter, TraceStart, TraceEnd, TraceProfile.Name, Params);
		//Default to end of trace line if we don't hit anything.
// 		if (!HitResult.bBlockingHit)
// 		{
// 			HitResult.Location = HitResult.lo;
// 		}

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

AAbilitySystemActor::AAbilitySystemActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SetReplicates(true);
	SetReplicateMovement(false);

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
}

void AAbilitySystemActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	AbilitySystemComponent->InitStats(UCharacterAttributeSet::StaticClass(), nullptr);
}

bool AAbilitySystemActor::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	return true;
}

void AAbilitySystemActor::SetupInputComponent(UInputComponent* Input)
{
	FGameplayAbilityInputBinds AbilityBinds(TEXT("ConfirmAbility"), TEXT("CancelAbility"), TEXT("EAbilityBindings"));
	AbilitySystemComponent->BindAbilityActivationToInputComponent(Input, AbilityBinds);
}