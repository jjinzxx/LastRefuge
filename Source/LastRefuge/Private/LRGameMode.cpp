#include "LRGameMode.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"

void ALRGameMode::OnPlayerDied(AController* DeadController)
{
	if (!DeadController) return;

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Player Died — respawn in %.1fs"), RespawnDelay);

	if (APawn* OldPawn = DeadController->GetPawn())
	{
		DeadController->UnPossess();
		OldPawn->Destroy();
	}

	FTimerHandle RespawnHandle;
	FTimerDelegate Del;
	Del.BindUObject(this, &ALRGameMode::RespawnPlayer, DeadController);
	GetWorld()->GetTimerManager().SetTimer(RespawnHandle, Del, RespawnDelay, false);
}

void ALRGameMode::RespawnPlayer(AController* Controller)
{
	if (!Controller) return;

	// 태그 "SafeZone"인 PlayerStart 사용
	AActor* SpawnPoint = FindPlayerStart(Controller, TEXT("SafeZone"));
	if (!SpawnPoint) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* NewPawn = GetWorld()->SpawnActor<APawn>(
		DefaultPawnClass,
		SpawnPoint->GetActorLocation(),
		SpawnPoint->GetActorRotation(),
		Params);

	if (NewPawn)
	{
		Controller->Possess(NewPawn);
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Respawned at SafeZone"));
	}
}