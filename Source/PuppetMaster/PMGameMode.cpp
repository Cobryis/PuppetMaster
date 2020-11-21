// Copyright Epic Games, Inc. All Rights Reserved.

#include "PMGameMode.h"

#include "PMPlayerController.h"
#include "PMCharacter.h"

#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

APMGameModeBase::APMGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = .2f;

	// use our custom PlayerController class
	PlayerControllerClass = APMPlayerController::StaticClass();
	PlayerStateClass = APMPlayerState::StaticClass();
	GameStateClass = APMGameState::StaticClass();

}

class APMGameState* APMGameModeBase::GetPMGameState() const
{
	return static_cast<APMGameState*>(GameState);
}

void APMGameModeBase::InitGameState()
{
	Super::InitGameState();

	check(GetGameState<APMGameState>() != nullptr); // make sure we're using the right gamestate class
}

void APMGameModeBase::StartPlay()
{
	Super::StartPlay();
}

namespace
{
	void ForEachPlayer(UWorld& World, const TFunction<void(APMPlayerController& PlayerController)>& DoThis)
	{
		for (FConstPlayerControllerIterator Iterator = World.GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APMPlayerController* PlayerController = static_cast<APMPlayerController*>(Iterator->Get());
			DoThis(*PlayerController);
		}
	}
}

void APMGameModeBase::Tick(float DeltaSeconds)
{
	const bool bServerTimerReached = GetPMGameState()->IsServerTimerActive() && (GetPMGameState()->GetServerTimerRemainingTime() == 0.f);

	UWorld& World = *GetWorld();
	
	if (GetPMGameState()->InMatchState(EMatchState::WaitingToStart))
	{
		int32 NumReadyPlayers = 0;
		bool bAllPlayersReady = true;
		for (const APlayerState* Player : GetPMGameState()->PlayerArray)
		{
			if (static_cast<const APMPlayerState*>(Player)->IsReady())
			{
				NumReadyPlayers += 1;
			}
			else
			{
				bAllPlayersReady = false;
				break;
			}
		}

		const bool bReadyToStart = bAllPlayersReady && (NumReadyPlayers >= NumReadyPlayers);
		if (bReadyToStart)
		{
			if (!GetPMGameState()->IsServerTimerActive())
			{
				GetPMGameState()->StartServerTimer(StartDelay);
			}
			else if (bServerTimerReached)
			{
				ForEachPlayer
				(
					World,
					[this](APMPlayerController& PlayerController)
					{
						if (PlayerCanRestart(&PlayerController))
						{
							RestartPlayer(&PlayerController);

							PlayerController.GetPlayerState<APMPlayerState>()->SetStatus(EPlayerMatchStatus::Alive);
						}
					}
				);

				EnterInvestigationState();
			}
		}
		else
		{
			GetPMGameState()->ClearServerTimer();
		}
	}
	else if (GetPMGameState()->InMatchState(EMatchState::Investigation))
	{

	}
	else if (GetPMGameState()->InMatchState(EMatchState::Discussion))
	{
		if (bServerTimerReached)
		{
			EnterVotingState();
		}
	}
	else if (GetPMGameState()->InMatchState(EMatchState::Voting))
	{
		int32 NumVotedPlayers = 0;
		bool bAllPlayersVoted = true;
		for (const APlayerState* Player : GetPMGameState()->PlayerArray)
		{
			if (static_cast<const APMPlayerState*>(Player)->IsReady())
			{
				NumVotedPlayers += 1;
			}
			else
			{
				bAllPlayersVoted = false;
				break;
			}
		}

		if (bAllPlayersVoted || bServerTimerReached)
		{
			EnterDeliberationState();
		}
	}
	else if (GetPMGameState()->InMatchState(EMatchState::Deliberation))
	{
		if (bServerTimerReached)
		{
			EnterInvestigationState(); // #todo: see if the game is over
		}
	}

	if (bServerTimerReached)
	{
		GetPMGameState()->ClearServerTimer();
	}
}

void APMGameModeBase::EnterInvestigationState()
{
	check(GetPMGameState()->InMatchState(EMatchState::WaitingToStart) || GetPMGameState()->InMatchState(EMatchState::Deliberation));
	GetPMGameState()->SetMatchState(EMatchState::Investigation);

	ForEachPlayer
	(
		*GetWorld(),
		[this](APMPlayerController& PlayerController)
		{
			PlayerController.EnableInput(&PlayerController);
		}
	);
}

void APMGameModeBase::EnterDiscussionState()
{
	check(GetPMGameState()->InMatchState(EMatchState::Investigation));
	GetPMGameState()->SetMatchState(EMatchState::Discussion);

	GetPMGameState()->StartServerTimer(DiscussionLength);

	ForEachPlayer
	(
		*GetWorld(),
		[this](APMPlayerController& PlayerController)
		{
			PlayerController.DisableInput(&PlayerController);
		}
	);
}

void APMGameModeBase::EnterVotingState()
{
	check(GetPMGameState()->InMatchState(EMatchState::Discussion));
	GetPMGameState()->SetMatchState(EMatchState::Voting);

	GetPMGameState()->StartServerTimer(VotingLength);
}

void APMGameModeBase::EnterDeliberationState()
{
	check(GetPMGameState()->InMatchState(EMatchState::Voting));
	GetPMGameState()->SetMatchState(EMatchState::Deliberation);

	GetPMGameState()->StartServerTimer(DeliberationLength);
}

void APMGameModeBase::ReportBody(const APMCharacter& ReportingCharacter, const APMCharacter& DeadCharacter)
{
	check(GetPMGameState()->InMatchState(EMatchState::Investigation));

	// #todo: broadcast report message

	EnterDeliberationState();
}

void APMGameModeBase::CallMeeting(const APMCharacter& ReportingCharacter)
{
	check(GetPMGameState()->InMatchState(EMatchState::Investigation));

	// #todo: broadcast meeting message

	EnterDiscussionState();
}

void APMGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// only start players if the match is waiting to start
	if (GetPMGameState()->InMatchState(EMatchState::WaitingToStart) && PlayerCanRestart(NewPlayer))
	{
		RestartPlayer(NewPlayer);
	}
}

void APMGameModeBase::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	Super::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
// 	if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
// 	{
// 		return;
// 	}
// 
// 	APMPlayerController* RecastNewPlayer = Cast<APMPlayerController>(NewPlayer);
// 	check(RecastNewPlayer);
// 
// 	if (!StartSpot)
// 	{
// 		UE_LOG(LogGameMode, Warning, TEXT("RestartPlayerAtPlayerStart: Player start not found"));
// 		return;
// 	}
// 
// 	FRotator SpawnRotation = StartSpot->GetActorRotation();
// 
// 	UE_LOG(LogGameMode, Verbose, TEXT("RestartPlayerAtPlayerStart %s"), (RecastNewPlayer && RecastNewPlayer->PlayerState) ? *RecastNewPlayer->PlayerState->GetPlayerName() : TEXT("Unknown"));
// 
// 	if (MustSpectate(Cast<APlayerController>(NewPlayer)))
// 	{
// 		UE_LOG(LogGameMode, Verbose, TEXT("RestartPlayerAtPlayerStart: Tried to restart a spectator-only player!"));
// 		return;
// 	}
// 
// 	if (RecastNewPlayer->GetSimulatedPawn() != nullptr)
// 	{
// 		RecastNewPlayer->GetSimulatedPawn()->Destroy();
// 		RecastNewPlayer->SetSimulatedPawn(nullptr);
// 	}
// 
// 	if (GetDefaultPawnClassForController(RecastNewPlayer) != nullptr)
// 	{
// 		// Try to create a pawn to use of the default class for this player
// 		RecastNewPlayer->SetSimulatedPawn(SpawnDefaultPawnFor(RecastNewPlayer, StartSpot));
// 	}
// 
// 	if (RecastNewPlayer->GetSimulatedPawn() == nullptr)
// 	{
// 		RecastNewPlayer->FailedToSpawnPawn();
// 	}
// 	else
// 	{
// 		InitStartSpot(StartSpot, RecastNewPlayer);
// 	}
}




void APMGameState::SetMatchState(EMatchState State)
{
	if (MatchState != State)
	{
		PrevMatchState = MatchState;
		MatchState = State;
	}
}

bool APMGameState::IsServerTimerActive() const
{
	return ServerTimerEnd != 0.f;
}

float APMGameState::GetServerTimerRemainingTime() const
{
	return FMath::Max(0.f, ServerTimerEnd - GetServerWorldTimeSeconds());
}

void APMGameState::StartServerTimer(float TimerLength)
{
	ServerTimerEnd = GetServerWorldTimeSeconds() + TimerLength;
}

void APMGameState::ClearServerTimer()
{
	ServerTimerEnd = 0.f;
}

void APMGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APMGameState, ServerTimerEnd);
	DOREPLIFETIME(APMGameState, MatchState);
}
