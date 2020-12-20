// Copyright Epic Games, Inc. All Rights Reserved.

#include "PMCharacter.h"

#include "PMAbilitySystemComponent.h"
#include "PMAttributeSets.h"

#include "AbilitySystemGlobals.h"

#include "DrawDebugHelpers.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Navigation/PathFollowingComponent.h"
#include "Net/UnrealNetwork.h"

APMCharacter::APMCharacter(const FObjectInitializer& OI)
	: Super(OI)
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create a camera boom...
	CameraBoomComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoomComponent->SetupAttachment(RootComponent);
	CameraBoomComponent->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoomComponent->TargetArmLength = 800.f;
	CameraBoomComponent->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoomComponent->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level
	CameraBoomComponent->bEnableCameraLag = true;

	// Create a camera...
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	CameraComponent->SetupAttachment(CameraBoomComponent, USpringArmComponent::SocketName);
	CameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	AbilitySystemComponent = CreateDefaultSubobject<UPMAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attributes = CreateDefaultSubobject<UCharacterAttributeSet>(TEXT("Attributes"));

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void APMCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	ACharacter::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APMCharacter, ReplicatedPath);
}

void APMCharacter::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);

// 	ReplicatedPath.Reset();
// 	if (IsValid(PathFollowingComponent) && PathFollowingComponent->GetPath().IsValid())
// 	{
// 		int32 CurrentPathIndex = PathFollowingComponent->GetCurrentPathIndex();
// 		const TArray<FNavPathPoint>& NavPoints = PathFollowingComponent->GetPath()->GetPathPoints();
// 		ReplicatedPath.Reset(NavPoints.Num());
// 		for (int32 i = 0; i < NavPoints.Num(); ++i)
// 		{
// 			if (i > CurrentPathIndex)
// 			{
// 				const FNavPathPoint& NavPoint = NavPoints[i];
// 				ReplicatedPath.Add(NavPoint.Location);
// 			}
// 		}
// 	}
}

void APMCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	for (auto TagIt = DefaultGameplayTags.CreateConstIterator(); TagIt; ++TagIt)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(*TagIt, 1);
	}
}

void APMCharacter::BeginPlay()
{
	Super::BeginPlay();

}

void APMCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	check(NewController);

	PathFollowingComponent = NewController->FindComponentByClass<UPathFollowingComponent>();

	AbilitySystemComponent->RefreshAbilityActorInfo();
}

void APMCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	AbilitySystemComponent->RefreshAbilityActorInfo();
}

void APMCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	AbilitySystemComponent->SetupPlayerInputComponent(*InputComponent);
}

void APMCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

// 	if (ReplicatedPath.Num() > 0)
// 	{
// 		CursorToWorld->SetVisibility(true);
// 		CursorToWorld->SetWorldLocation(ReplicatedPath.Last());
// 	}
// 	else
// 	{
// 		CursorToWorld->SetVisibility(false);
// 	}
// 
// 	FVector PrevPoint = GetActorLocation();
// 	for (const FVector& CurrentPoint : ReplicatedPath)
// 	{
// 		DrawDebugLine(GetWorld(), PrevPoint, CurrentPoint, FColor::Green, false, -1.f, 1, 5.f);
// 		PrevPoint = CurrentPoint;
// 	}
}

void APMCharacter::MoveTo(const FVector& Location)
{
	check(HasAuthority() || IsLocallyControlled());
	check(GetController());
	check(IsAlive());

	float const Distance = FVector::Dist(Location, GetActorLocation());

	// We need to issue move command only if far enough in order for walk animation to play correctly
	if ((Distance > 120.0f))
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), Location);
	}
}

void APMCharacter::MoveToActorAndPerformAction(APMCharacter& Target)
{
// 	check(HasAuthority());
// 	check(GetController());
// 	check(IsAlive());
// 
// 	check(&Target != this);
// 	check(PathFollowingComponent);
// 
// 	CurrentTarget = &Target;
// 
// 	PathFollowingComponent->AbortMove(*GetController(), FPathFollowingResultFlags::NewRequest, FAIRequestID::CurrentRequest, EPathFollowingVelocityMode::Keep);
// 	FollowHandle = PathFollowingComponent->OnRequestFinished.AddLambda
// 	(
// 		[this](FAIRequestID RequestID, const FPathFollowingResult& Result)
// 		{
// 			if (Result.Code == EPathFollowingResult::Success)
// 			{
// 				APMCharacter* Victim = Cast<APMCharacter>(CurrentTarget.Get());
// 				if (IsValid(Victim) && !Victim->IsIncapacitated())
// 				{
// 					Victim->TryToKill(*this, 1);
// 				}
// 				else if (IsValid(Victim) && Victim->IsIncapacitated() && Victim->IsAlive())
// 				{
// 					Victim->Revived();
// 				}
// 			}
// 
// 			CurrentTarget.Reset();
// 			FollowHandle.Reset();
// 		}
// 	);
// 	UAIBlueprintHelperLibrary::SimpleMoveToActor(GetController(), &Target);
}

UAbilitySystemComponent* APMCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// bool APMCharacter::TryToKill(const APMCharacter& Perpetrator, int32 HitPoints)
// {
// 	check(HasAuthority());
// 	check(GetController());
// 	check(IsAlive());
// 
// 	AdjustHealth(Perpetrator, -HitPoints);
// 
// 	if (IsAlive())
// 	{
// 		PassOut();
// 		return false;
// 	}
// 	else
// 	{
// 		Die(Perpetrator);
// 		return true;
// 	}
// }
// 
// void APMCharacter::AdjustHealth(const AActor& DamageCauser, int32 AdjustAmount)
// {
// 	check(HasAuthority());
// 	check(GetController());
// 	check(IsAlive());
// 
// 	Health = FMath::Clamp(Health + AdjustAmount, 0, HealthMax);
// }


void APMCharacter::Die(const APMCharacter& Perpetrator)
{
	check(GetController()); // we shouldn't be able to die if we're already dead

	GetController()->Destroy();
}

// void APMCharacter::Incapacitated()
// {
// 	bIncapacitated = true;
// 
// 	OnIncapacitated.Broadcast();
// 
// 	GetCharacterMovement()->StopActiveMovement();
// 	GetMovementComponent()->Deactivate();
// 
// 	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
// 	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
// 
// }
// 
// void APMCharacter::Revived()
// {
// 	bIncapacitated = false;
// 
// 	GetMovementComponent()->Activate();
// 
// 	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
// 	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
// 
// 	OnRevived.Broadcast();
// }


bool APMCharacter::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	return GetAbilitySystemComponent()->HasMatchingGameplayTag(TagToCheck);
}

bool APMCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	return GetAbilitySystemComponent()->HasAllMatchingGameplayTags(TagContainer);
}

bool APMCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer) const
{
	return GetAbilitySystemComponent()->HasAnyMatchingGameplayTags(TagContainer);
}

void APMCharacter::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	GetAbilitySystemComponent()->GetOwnedGameplayTags(TagContainer);
}

