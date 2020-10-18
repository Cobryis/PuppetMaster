// Copyright Epic Games, Inc. All Rights Reserved.

#include "PMPlayerController.h"

#include "PMCharacter.h"

#include "Blueprint/AIBlueprintHelperLibrary.h"
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
	SimulatedPawn = InPawn;

	if (SimulatedPawn)
	{
		SetViewTarget(SimulatedPawn);
	}
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

	// keep updating the destination every tick while desired
	if (bMoveToMouseCursor)
	{
		MoveToMouseCursor();
	}
}

void APMPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("SetDestination", IE_Pressed, this, &APMPlayerController::OnSetDestinationPressed);
	InputComponent->BindAction("SetDestination", IE_Released, this, &APMPlayerController::OnSetDestinationReleased);
}

void APMPlayerController::MoveToMouseCursor()
{
	// Trace to see what is under the mouse cursor
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	if (Hit.bBlockingHit)
	{
		// We hit something, move there
		SetNewMoveDestination(Hit.ImpactPoint);
	}
}

void APMPlayerController::SetNewMoveDestination(const FVector DestLocation)
{
	if (HasAuthority())
	{
		if (SimulatedPawn)
		{
			float const Distance = FVector::Dist(DestLocation, SimulatedPawn->GetActorLocation());

			// We need to issue move command only if far enough in order for walk animation to play correctly
			if ((Distance > 120.0f))
			{
				UAIBlueprintHelperLibrary::SimpleMoveToLocation(SimulatedPawn->GetController(), DestLocation);
			}
		}
	}
	else
	{
		ServerSetNewMoveDestination(DestLocation);
	}
}

void APMPlayerController::ServerSetNewMoveDestination_Implementation(const FVector DestLocation)
{
	SetNewMoveDestination(DestLocation);
}

void APMPlayerController::OnSetDestinationPressed()
{
	// set flag to keep updating destination until released
	bMoveToMouseCursor = true;
}

void APMPlayerController::OnSetDestinationReleased()
{
	// clear flag to indicate we should stop updating the destination
	bMoveToMouseCursor = false;
}

void APMPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APMPlayerState, Status);
}

void APMPlayerState::SetReady()
{
	if (Status == EPlayerStatus::NotReady)
	{
		if (HasAuthority())
		{
			Status = EPlayerStatus::Ready;
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
