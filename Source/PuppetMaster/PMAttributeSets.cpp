// Copyright Epic Games, Inc. All Rights Reserved.

#include "PMAttributeSets.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectTypes.h"

#include "Net/UnrealNetwork.h"

void UCharacterAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	const FName ModifiedAttributeName = Data.EvaluatedData.Attribute.GetUProperty()->GetFName();
	if (ModifiedAttributeName == GET_MEMBER_NAME_CHECKED(UCharacterAttributeSet, Health))
	{
		OnHealthChanged.Broadcast(GetHealth(), Data.EffectSpec.GetEffectContext().GetInstigator());
	}
}

void UCharacterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	const FName ModifiedAttributeName = Attribute.GetUProperty()->GetFName();
	if (ModifiedAttributeName == GET_MEMBER_NAME_CHECKED(UCharacterAttributeSet, Health))
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
}

void UCharacterAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCharacterAttributeSet, Health);
	DOREPLIFETIME(UCharacterAttributeSet, MaxHealth);
	DOREPLIFETIME(UCharacterAttributeSet, VisionRadius);
}

void UCharacterAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, Health, OldValue);
}

void UCharacterAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, MaxHealth, OldValue);
}

void UCharacterAttributeSet::OnRep_VisionRadius(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCharacterAttributeSet, VisionRadius, OldValue);
}
