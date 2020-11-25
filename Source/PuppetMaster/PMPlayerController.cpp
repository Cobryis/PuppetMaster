// Copyright Epic Games, Inc. All Rights Reserved.

#include "PMPlayerController.h"

#include "PMAbilitySystemComponent.h"
#include "PMCharacter.h"

#include "AbilitySystemGlobals.h"

#include "Net/UnrealNetwork.h"

DECLARE_LOG_CATEGORY_CLASS(LogPMPlayerController, Warning, All)

APMPlayerController::APMPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void APMPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// StateName = NAME_Inactive;
}

void APMPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	AController::GetLifetimeReplicatedProps(OutLifetimeProps);

	// ResetReplicatedLifetimeProperty(StaticClass(), AController::StaticClass(), TEXT("Pawn"), COND_Never, OutLifetimeProps);
}

void APMPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void APMPlayerController::SetPawn(APawn* InPawn)
{
	Super::SetPawn(InPawn);
}

APMCharacter* APMPlayerController::GetControlledPawn() const
{
	return GetPawn<APMCharacter>();
}

UAbilitySystemComponent* APMPlayerController::GetAbilitySystemComponent() const
{
	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetControlledPawn());
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

	FInputActionBinding& Binding = InputComponent->BindAction("SetDestination", IE_Pressed, this, &APMPlayerController::InputAction_SelectPressed);
	Binding.bConsumeInput = false; // hack to get confirm ability to work on LMB
}

void APMPlayerController::SetNewMoveDestination(const FVector& DestLocation)
{
	check(IsValid(GetControlledPawn()));

	if (HasAuthority())
	{
		GetControlledPawn()->MoveTo(DestLocation);
		
	}
	else
	{
		if (IsLocalController() && GetControlledPawn() == GetPawn())
		{
			GetControlledPawn()->MoveTo(DestLocation);
		}
		ServerSetNewMoveDestination(DestLocation);
	}
}

void APMPlayerController::ServerSetNewMoveDestination_Implementation(const FVector& DestLocation)
{
	if (!GetControlledPawn()->IsAlive())
	{
		UE_LOG(LogPMPlayerController, Error, TEXT("Attempted to move dead pawn"));
		return;
	}

	SetNewMoveDestination(DestLocation);
}

void APMPlayerController::SetFollowTarget(APMCharacter* Target)
{
	check(IsValid(GetControlledPawn()) && GetControlledPawn()->IsAlive());

	// #todo: we may want to move to a target regardless of its health
	if (!IsValid(Target) || !Target->IsAlive())
	{
		return;
	}

	if (HasAuthority() && IsValid(Target))
	{
		GetControlledPawn()->MoveToActorAndPerformAction(*Target);
	}
	else
	{
		ServerSetFollowTarget(Target);
	}
}

void APMPlayerController::ServerSetFollowTarget_Implementation(APMCharacter* Target)
{
	if (!GetControlledPawn()->IsAlive())
	{
		UE_LOG(LogPMPlayerController, Error, TEXT("Attempted to move dead pawn"));
		return;
	}

	SetFollowTarget(Target);
}

void APMPlayerController::InputAction_SelectPressed()
{
	if (IsValid(GetControlledPawn()))
	{
		if (!GetControlledPawn()->IsAlive())
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
			if (IsValid(PMCharacter) && (PMCharacter != GetControlledPawn()) && PMCharacter->IsAlive())
			{
				// SetFollowTarget(static_cast<APMCharacter*>(Hit.GetActor()));
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