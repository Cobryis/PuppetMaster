// Copyright Epic Games, Inc. All Rights Reserved.

#include "PMPlayerController.h"

#include "PMCharacter.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"

APMPlayerController::APMPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void APMPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	StateName = NAME_Inactive;
}

void APMPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	AController::GetLifetimeReplicatedProps(OutLifetimeProps);

	ResetReplicatedLifetimeProperty(StaticClass(), AController::StaticClass(), TEXT("Pawn"), COND_Never, OutLifetimeProps);

	DOREPLIFETIME(APMPlayerController, SimulatedPawn);
}

void APMPlayerController::SetSimulatedPawn(APawn* InPawn)
{
	check(!IsValid(InPawn) || InPawn->IsA<APMCharacter>());

	SimulatedPawn = Cast<APMCharacter>(InPawn);

	if (SimulatedPawn)
	{
		SetViewTarget(SimulatedPawn);
	}
}

APawn* APMPlayerController::GetSimulatedPawn() const
{
	return SimulatedPawn;
}

void APMPlayerController::OnRep_SimulatedPawn()
{
	SetSimulatedPawn(SimulatedPawn);
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

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &APMPlayerController::InputAction_SelectPressed);
}

void APMPlayerController::SetNewMoveDestination(const FVector& DestLocation)
{
	check(IsValid(SimulatedPawn) && SimulatedPawn->IsAlive());

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
	SetNewMoveDestination(DestLocation);
}

void APMPlayerController::SetFollowTarget(const APMCharacter* Target)
{
	check(IsValid(SimulatedPawn) && SimulatedPawn->IsAlive());

	if (HasAuthority() && IsValid(Target))
	{
		SimulatedPawn->Follow(*Target);
	}
	else
	{
		ServerSetFollowTarget(Target);
	}
}

void APMPlayerController::ServerSetFollowTarget_Implementation(const APMCharacter* Target)
{
	SetFollowTarget(Target);
}

void APMPlayerController::InputAction_SelectPressed()
{
	// Trace to see what is under the mouse cursor
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	if (Hit.bBlockingHit)
	{
		if (Hit.Actor.IsValid() && Hit.Actor->IsA<APMCharacter>())
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
