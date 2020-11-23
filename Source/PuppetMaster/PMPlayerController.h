// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include "AttributeSet.h"
#include "AbilitySystemInterface.h"

#include "PMPlayerController.generated.h"

class APMCharacter;
class UAbilitySystemComponent;

UENUM()
enum class EAbilityBindings
{
	Ability1,
	Ability2,
	Ability3,
	Ability4,
	Ability5
};

UCLASS()
class APMPlayerController : public APlayerController
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APMPlayerController();

	void PostInitializeComponents() override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void BeginPlay() override;

	void SetSimulatedPawn(APawn* InPawn);
	APawn* GetSimulatedPawn() const;

	void SetPawn(APawn* InPawn);

	APMCharacter* GetControlledPawn() const;

	UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:

	UPROPERTY(Transient, ReplicatedUsing=OnRep_SimulatedPawn)
	APMCharacter* SimulatedPawn = nullptr;

	UFUNCTION()
	void OnRep_SimulatedPawn();

	UFUNCTION()
	void OnRep_AbilitySystemActor();

	// Begin PlayerController interface
	void ChangeState(FName NewState) override;
	void PlayerTick(float DeltaTime) override;
	void SetupInputComponent() override;
	ASpectatorPawn* SpawnSpectatorPawn() override { return nullptr; }
	// End PlayerController interface
	
	/** Navigate player to the given world location. */
	void SetNewMoveDestination(const FVector& DestLocation);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetNewMoveDestination(const FVector& DestLocation);
	void ServerSetNewMoveDestination_Implementation(const FVector& DestLocation);
	bool ServerSetNewMoveDestination_Validate(const FVector& DestLocation) const { return true; }

	void SetFollowTarget(APMCharacter* Target);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetFollowTarget(APMCharacter* Target);
	void ServerSetFollowTarget_Implementation(APMCharacter* Target);
	bool ServerSetFollowTarget_Validate(APMCharacter* Target) const { return true; }

	/** Input handlers for SetDestination action. */
	void InputAction_SelectPressed();
};

UENUM(BlueprintType)
enum class EPlayerMatchStatus : uint8
{
	NotReady,
	Ready,
	Alive,
	Dead,
	Disconnected
};

UENUM(BlueprintType)
enum class EPlayerVoteStatus : uint8
{
	NoVote,
	Voted,
};

UCLASS()
class APMPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
	void SetReady();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetReady();
	void ServerSetReady_Implementation();
	bool ServerSetReady_Validate() const { return true; }

	bool IsReady() const { return MatchStatus == EPlayerMatchStatus::Ready; }

	void SetStatus(EPlayerMatchStatus NewStatus) { MatchStatus = NewStatus; }

	UFUNCTION(BlueprintPure)
	EPlayerMatchStatus GetStatus() const { return MatchStatus; }

protected:

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

private:

	UPROPERTY(Replicated)
	EPlayerMatchStatus MatchStatus;

	UPROPERTY(Replicated)
	EPlayerVoteStatus VoteStatus;

};