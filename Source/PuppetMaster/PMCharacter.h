// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagAssetInterface.h"

#include "PMCharacter.generated.h"

class UCharacterAttributeSet;

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

 	bool IsAlive() const { return IsValid(GetController()); }
// 	bool IsIncapacitated() const { return bIncapacitated; }

	void MoveTo(const FVector& Location);
	void MoveToActorAndPerformAction(APMCharacter& Victim);

	// bool TryToKill(const APMCharacter& Perpetrator, int32 HitPoints);
	// void AdjustHealth(const AActor& DamageCauser, int32 AdjustAmount);

	UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure)
	UCharacterAttributeSet* GetCharacterAttributes() const { return const_cast<UCharacterAttributeSet*>(Attributes); }

protected:

	APMCharacter(const FObjectInitializer& OI);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

	void PostInitializeComponents() override;
	void BeginPlay() override;
	void PossessedBy(AController* NewController) override;
	void PawnClientRestart() override;
	void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void Tick(float DeltaSeconds) override;

	void Die(const APMCharacter& Perpetrator);

private:

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
	const UCharacterAttributeSet* Attributes = nullptr;

	TWeakObjectPtr<APMCharacter> CurrentTarget;
	FDelegateHandle FollowHandle;

	UPROPERTY(EditAnywhere)
	FGameplayTagContainer DefaultGameplayTags;

	bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;

	bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const override;

	void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

};
