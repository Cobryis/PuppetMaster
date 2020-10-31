// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

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
{
	GENERATED_BODY()

public:

	bool IsAlive() const { return Health > 0; }

	void MoveTo(const FVector& Location);
	void MoveToActorAndPerformAction(APMCharacter& Victim);

	bool TryToKill(const APMCharacter& Perpetrator, int32 HitPoints);
	void AdjustHealth(const AActor& DamageCauser, int32 AdjustAmount);

	class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	class UDecalComponent* GetCursorToWorld() { return CursorToWorld; }

protected:

	APMCharacter(const FObjectInitializer& OI);

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

	void PossessedBy(AController* NewController) override;

	void Tick(float DeltaSeconds) override;

	void PassOut();
	void Die(const APMCharacter& Perpetrator);

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
	int32 HealthMax = 1;

	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FVector_NetQuantize> ReplicatedPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UDecalComponent* CursorToWorld;

	UPROPERTY(Transient)
	class UPathFollowingComponent* PathFollowingComponent = nullptr;

	TWeakObjectPtr<APMCharacter> CurrentTarget;
	FDelegateHandle FollowHandle;

};
