// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include "AttributeSet.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTargetActor_Trace.h"

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
class UCharacterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health = FGameplayAttributeData(1.f);

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

	UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:

	UPROPERTY(Transient, ReplicatedUsing=OnRep_SimulatedPawn)
	APMCharacter* SimulatedPawn = nullptr;

	UFUNCTION()
	void OnRep_SimulatedPawn();

	UPROPERTY(Transient, ReplicatedUsing = OnRep_AbilitySystemActor)
	class AAbilitySystemActor* AbilitySystemActor = nullptr;

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

UCLASS()
class AAbilitySystemActor : public AActor
	, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:

	void SetupInputComponent(UInputComponent* Input);
		
	UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

protected:

	AAbilitySystemActor();

	void PostInitializeComponents() override;

	bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UAbilitySystemComponent* AbilitySystemComponent = nullptr;

};

UCLASS()
class AGameplayAbilityTargetActor_Cursor : public AGameplayAbilityTargetActor_Trace
{
	GENERATED_BODY()

protected:

	// the hit target is valid if it has any of these gameplay tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	FGameplayTagContainer RequiredGameplayTags;

	FHitResult PerformTrace(AActor* InSourceActor) override;

	bool IsConfirmTargetingAllowed() override { return bHasValidTarget; }

private:

	bool bHasValidTarget = false;

};
