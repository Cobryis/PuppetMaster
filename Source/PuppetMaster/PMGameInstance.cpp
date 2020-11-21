// Copyright Epic Games, Inc. All Rights Reserved.

#include "PMGameInstance.h"

#include "AbilitySystemGlobals.h"

void UPMGameInstance::Init()
{
	UAbilitySystemGlobals::Get().InitGlobalData();
}
