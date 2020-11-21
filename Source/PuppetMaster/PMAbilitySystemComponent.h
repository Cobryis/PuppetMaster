// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbilityTargetActor_Trace.h"

#include "PMAbilitySystemComponent.generated.h"

UCLASS()
class UPMAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:

	UPMAbilitySystemComponent(const FObjectInitializer& OI);

	void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

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


UCLASS()
class UCharacterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health = FGameplayAttributeData(1.f);

};