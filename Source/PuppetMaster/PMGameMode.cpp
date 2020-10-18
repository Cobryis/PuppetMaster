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

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/TopDownCPP/Blueprints/TopDownCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void APMGameModeBase::InitGameState()
{
	Super::InitGameState();

	check(GetGameState<APMGameState>()); // make sure we're using the right gamestate class
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
	APMGameState* PMGameState = static_cast<APMGameState*>(GameState);

	const bool bServerTimerReached = PMGameState->IsServerTimerActive() && (PMGameState->GetServerTimerRemainingTime() == 0.f);

	UWorld& World = *GetWorld();
	
	if (PMGameState->InMatchState(EMatchState::WaitingToStart))
	{
		int32 NumReadyPlayers = 0;
		bool bAllPlayersReady = true;
		for (const APlayerState* Player : GetGameState<APMGameState>()->PlayerArray)
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
			if (!PMGameState->IsServerTimerActive())
			{
				PMGameState->StartServerTimer(StartDelay);
			}
			else if (bServerTimerReached)
			{
				PMGameState->SetMatchState(EMatchState::Investigation);

				ForEachPlayer
				(
					World,
					[this](APMPlayerController& PlayerController)
					{
						if (PlayerCanRestart(&PlayerController))
						{
							RestartPlayer(&PlayerController);

							if (IsValid(PlayerController.GetSimulatedPawn()))
							{
								PlayerController.GetPlayerState<APMPlayerState>()->SetStatus(EPlayerStatus::Alive);
							}
						}
					}
				);
			}
		}
		else
		{
			PMGameState->ClearServerTimer();
		}
	}
	else if (PMGameState->InMatchState(EMatchState::Investigation))
	{
		ForEachPlayer
		(
			World,
			[this](APMPlayerController& PlayerController)
			{
				PlayerController.EnableInput(&PlayerController);
			}
		);
	}
	else if (PMGameState->InMatchState(EMatchState::Voting))
	{
		check(PMGameState->PrevMatchState == EMatchState::Investigation);

		ForEachPlayer
		(
			World,
			[this](APMPlayerController& PlayerController)
			{
				PlayerController.DisableInput(&PlayerController);
			}
		);
	}
	else if (PMGameState->InMatchState(EMatchState::Deliberation))
	{
		check(PMGameState->PrevMatchState == EMatchState::Voting);


	}
	else if (PMGameState->InMatchState(EMatchState::PostMatch))
	{


	}

	if (bServerTimerReached)
	{
		PMGameState->ClearServerTimer();
	}
}

void APMGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// only start players if the match is waiting to start
	if (GetGameState<APMGameState>()->InMatchState(EMatchState::WaitingToStart) && PlayerCanRestart(NewPlayer))
	{
		RestartPlayer(NewPlayer);
	}
}

void APMGameModeBase::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
	{
		return;
	}

	APMPlayerController* RecastNewPlayer = Cast<APMPlayerController>(NewPlayer);
	check(RecastNewPlayer);

	if (!StartSpot)
	{
		UE_LOG(LogGameMode, Warning, TEXT("RestartPlayerAtPlayerStart: Player start not found"));
		return;
	}

	FRotator SpawnRotation = StartSpot->GetActorRotation();

	UE_LOG(LogGameMode, Verbose, TEXT("RestartPlayerAtPlayerStart %s"), (RecastNewPlayer && RecastNewPlayer->PlayerState) ? *RecastNewPlayer->PlayerState->GetPlayerName() : TEXT("Unknown"));

	if (MustSpectate(Cast<APlayerController>(NewPlayer)))
	{
		UE_LOG(LogGameMode, Verbose, TEXT("RestartPlayerAtPlayerStart: Tried to restart a spectator-only player!"));
		return;
	}

	if (RecastNewPlayer->GetSimulatedPawn() != nullptr)
	{
		RecastNewPlayer->GetSimulatedPawn()->Destroy();
		RecastNewPlayer->SetSimulatedPawn(nullptr);
	}

	if (GetDefaultPawnClassForController(RecastNewPlayer) != nullptr)
	{
		// Try to create a pawn to use of the default class for this player
		RecastNewPlayer->SetSimulatedPawn(SpawnDefaultPawnFor(RecastNewPlayer, StartSpot));
	}

	if (RecastNewPlayer->GetSimulatedPawn() == nullptr)
	{
		RecastNewPlayer->FailedToSpawnPawn();
	}
	else
	{
		InitStartSpot(StartSpot, RecastNewPlayer);
	}
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
