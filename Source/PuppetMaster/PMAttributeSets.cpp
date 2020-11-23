// Copyright Epic Games, Inc. All Rights Reserved.

#include "PMAttributeSets.h"

#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectTypes.h"

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
