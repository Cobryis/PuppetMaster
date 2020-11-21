// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/GameInstance.h"

#include "PMGameInstance.generated.h"

UCLASS()
class UPMGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	void Init() override;

};