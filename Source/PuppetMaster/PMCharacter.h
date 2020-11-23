// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"

#include "PMCharacter.generated.h"

// #todo: this is for art - what state is this body in
UENUM(BlueprintType)
enum class EBodyState : uint8
{
	Alive,
	PassedOut,
	Killer,
	Dead
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIncapacitated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRevived);

UCLASS(Blueprintable)
class APMCharacter : public ACharacter
	, public IAbilitySystemInterface
	, public IGameplayCueInterface
	, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:

	bool IsAlive() const { return Health > 0; }
	bool IsIncapacitated() const { return bIncapacitated; }

	void MoveTo(const FVector& Location);
	void MoveToActorAndPerformAction(APMCharacter& Victim);

	// bool TryToKill(const APMCharacter& Perpetrator, int32 HitPoints);
	// void AdjustHealth(const AActor& DamageCauser, int32 AdjustAmount);

	UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:

	APMCharacter(const FObjectInitializer& OI);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

	void PostInitializeComponents() override;
	void BeginPlay() override;
	void PossessedBy(AController* NewController) override;
	void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void Tick(float DeltaSeconds) override;

	void OnHealthChanged(float NewHealth, AActor* EventInstigator);
	void PassOut();
	void Die(const APMCharacter& Perpetrator);

	void Incapacitated();
	void Revived();

	UPROPERTY(BlueprintAssignable)
	FIncapacitated OnIncapacitated;

	UPROPERTY(BlueprintAssignable)
	FRevived OnRevived;

private:

	UPROPERTY(ReplicatedUsing=OnRep_Incapacitated)
	bool bIncapacitated = false;

	UFUNCTION()
	void OnRep_Incapacitated();

	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 Health = 1;

	// #todo
	int32 HealthMax = 2;

	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FVector_NetQuantize> ReplicatedPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoomComponent;

	UPROPERTY()
	class UPMAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(Transient)
	class UPathFollowingComponent* PathFollowingComponent = nullptr;

	UPROPERTY(Transient)
	const class UCharacterAttributeSet* Attributes = nullptr;

	TWeakObjectPtr<APMCharacter> CurrentTarget;
	FDelegateHandle FollowHandle;

	UPROPERTY(EditAnywhere)
	FGameplayTagContainer GameplayTags;

	void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

};
