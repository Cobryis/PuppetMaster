// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"

#include "PMGameMode.generated.h"

class APMCharacter;

UENUM(BlueprintType)
enum class EMatchState : uint8
{
	WaitingToStart,
	Investigation,
	Discussion,
	Voting,
	Deliberation,
	PostMatch
};

UCLASS(minimalapi)
class APMGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	APMGameModeBase();

protected:

	class APMGameState* GetPMGameState() const;

	void InitGameState() override;
	void StartPlay() override;

	void Tick(float DeltaSeconds) override;

	void EnterInvestigationState();
	void EnterDiscussionState();
	void EnterVotingState();
	void EnterDeliberationState();

	void ReportBody(const APMCharacter& ReportingCharacter, const APMCharacter& DeadCharacter);
	void CallMeeting(const APMCharacter& ReportingCharacter);

	void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot) override;
	APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

protected:

	UPROPERTY(config)
	int32 MinNumPlayers = 2;

	UPROPERTY(config)
	float StartDelay = 3.f;

	UPROPERTY(config)
	float DiscussionLength = 30.f;

	UPROPERTY(config)
	float VotingLength = 30.f;

	UPROPERTY(config)
	float DeliberationLength = 10.f;

};

UCLASS(minimalAPI)
class APMGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	/** The time at which the server's current timer will end. */
	UPROPERTY(Replicated, BlueprintReadOnly)
	float ServerTimerEnd = 0.f;

	UPROPERTY(Replicated, BlueprintReadOnly)
	EMatchState MatchState = EMatchState::WaitingToStart;

	EMatchState PrevMatchState = EMatchState::PostMatch;

	bool InMatchState(EMatchState State) const { return State == MatchState; }
	void SetMatchState(EMatchState State);

	UFUNCTION(BlueprintPure)
	bool IsServerTimerActive() const;

	UFUNCTION(BlueprintPure)
	float GetServerTimerRemainingTime() const;

	void StartServerTimer(float TimerLength);
	void ClearServerTimer();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;

};

