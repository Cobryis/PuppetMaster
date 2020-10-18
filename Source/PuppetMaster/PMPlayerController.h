// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include "PMPlayerController.generated.h"

UCLASS()
class APMPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	APMPlayerController();

	void PostInitializeComponents() override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetSimulatedPawn(APawn* InPawn);
	APawn* GetSimulatedPawn() const { return SimulatedPawn; }

protected:

	UPROPERTY(Transient, ReplicatedUsing=OnRep_SimulatedPawn)
	APawn* SimulatedPawn = nullptr;

	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	UFUNCTION()
	void OnRep_SimulatedPawn();

	// Begin PlayerController interface
	void ChangeState(FName NewState) override;
	void PlayerTick(float DeltaTime) override;
	void SetupInputComponent() override;
	ASpectatorPawn* SpawnSpectatorPawn() override { return nullptr; }
	// End PlayerController interface

	/** Navigate player to the current mouse cursor location. */
	void MoveToMouseCursor();
	
	/** Navigate player to the given world location. */
	void SetNewMoveDestination(const FVector DestLocation);

	UFUNCTION(Server, Unreliable, WithValidation)
	void ServerSetNewMoveDestination(const FVector DestLocation);
	void ServerSetNewMoveDestination_Implementation(const FVector DestLocation);
	bool ServerSetNewMoveDestination_Validate(const FVector DestLocation) const { return true; }

	/** Input handlers for SetDestination action. */
	void OnSetDestinationPressed();
	void OnSetDestinationReleased();
};

UENUM(BlueprintType)
enum class EPlayerStatus : uint8
{
	NotReady,
	Ready,
	Alive,
	Dead,
	Disconnected
};

UCLASS()
class APMPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

	UFUNCTION(BlueprintCallable)
	void SetReady();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetReady();
	void ServerSetReady_Implementation();
	bool ServerSetReady_Validate() const { return true; }

	bool IsReady() const { return Status == EPlayerStatus::Ready; }

	void SetStatus(EPlayerStatus NewStatus) { Status = NewStatus; }

	UFUNCTION(BlueprintPure)
	EPlayerStatus GetStatus() const { return Status; }

private:

	UPROPERTY(Replicated)
	EPlayerStatus Status;

};
