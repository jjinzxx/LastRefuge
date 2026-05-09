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

	// 1순위: 태그 "SafeZone"인 PlayerStart
	AActor* SpawnPoint = FindPlayerStart(Controller, TEXT("SafeZone"));

	// 2순위: 아무 PlayerStart라도 사용 (SafeZone 태그 미설정 시 폴백)
	if (!SpawnPoint)
	{
		SpawnPoint = FindPlayerStart(Controller, FString());
		if (SpawnPoint)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GameMode] SafeZone 태그 PlayerStart 없음 — 기본 PlayerStart로 폴백"));
		}
	}

	if (!SpawnPoint)
	{
		UE_LOG(LogTemp, Error, TEXT("[GameMode] 사용 가능한 PlayerStart 없음 — 리스폰 실패"));
		return;
	}

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