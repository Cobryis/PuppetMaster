// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AttributeSet.h"

#include "PMAttributeSets.generated.h"

UCLASS()
class UCharacterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	UPROPERTY(ReplicatedUsing=OnRep_Health, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Health = FGameplayAttributeData(1.f);
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UCharacterAttributeSet, Health)
	GAMEPLAYATTRIBUTE_VALUE_GETTER(Health)

	UPROPERTY(ReplicatedUsing=OnRep_MaxHealth, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData MaxHealth = FGameplayAttributeData(2.f);
	GAMEPLAYATTRIBUTE_VALUE_GETTER(MaxHealth)

	UPROPERTY(ReplicatedUsing=OnRep_VisionRadius, BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData VisionRadius = FGameplayAttributeData(500.f);

	DECLARE_EVENT_TwoParams(UCharacterAttributeSet, FOnHealthChanged, float, AActor*)
	FOnHealthChanged OnHealthChanged;

private:

	/** Override to disable initialization for specific properties */
	// bool ShouldInitProperty(bool FirstInit, FProperty* PropertyToInit) const override;

	/**
	 *	Called just before modifying the value of an attribute. AttributeSet can make additional modifications here. Return true to continue, or false to throw out the modification.
	 *	Note this is only called during an 'execute'. E.g., a modification to the 'base value' of an attribute. It is not called during an application of a GameplayEffect, such as a 5 ssecond +10 movement speed buff.
	 */
	// bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;

	/**
	 *	Called just before a GameplayEffect is executed to modify the base value of an attribute. No more changes can be made.
	 *	Note this is only called during an 'execute'. E.g., a modification to the 'base value' of an attribute. It is not called during an application of a GameplayEffect, such as a 5 ssecond +10 movement speed buff.
	 */
	void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	/**
	 *	An "On Aggregator Change" type of event could go here, and that could be called when active gameplay effects are added or removed to an attribute aggregator.
	 *	It is difficult to give all the information in these cases though - aggregators can change for many reasons: being added, being removed, being modified, having a modifier change, immunity, stacking rules, etc.
	 */

	/**
	 *	Called just before any modification happens to an attribute. This is lower level than PreAttributeModify/PostAttribute modify.
	 *	There is no additional context provided here since anything can trigger this. Executed effects, duration based effects, effects being removed, immunity being applied, stacking rules changing, etc.
	 *	This function is meant to enforce things like "Health = Clamp(Health, 0, MaxHealth)" and NOT things like "trigger this extra thing if damage is applied, etc".
	 *	
	 *	NewValue is a mutable reference so you are able to clamp the newly applied value as well.
	 */
	void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/**
	 *	This is called just before any modification happens to an attribute's base value when an attribute aggregator exists.
	 *	This function should enforce clamping (presuming you wish to clamp the base value along with the final value in PreAttributeChange)
	 *	This function should NOT invoke gameplay related events or callbacks. Do those in PreAttributeChange() which will be called prior to the
	 *	final value of the attribute actually changing.
	 */
	// void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override { }

	/** Callback for when an FAggregator is created for an attribute in this set. Allows custom setup of FAggregator::EvaluationMetaData */
	// void OnAttributeAggregatorCreated(const FGameplayAttribute& Attribute, FAggregator* NewAggregator) const override;

	void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> & OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_VisionRadius(const FGameplayAttributeData& OldValue);

};
