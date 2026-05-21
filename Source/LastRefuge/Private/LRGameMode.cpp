#include "LRGameMode.h"
#include "LRGameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "TimerManager.h"
#include "AI/LRBotAIController.h"
#include "Kismet/GameplayStatics.h"

void ALRGameMode::OnPlayerDied(AController* DeadController)
{
	if (!DeadController) return;

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Player Died — respawn in %.1fs"), RespawnDelay);

	// 사망 시 인벤/툴바 초기화, 창고(PersistentStorageItems)는 유지
	// bHasTravelData = true로 설정해 BeginPlay가 SaveGame 대신 GI에서 복원하도록 유도
	if (ULRGameInstance* GI = Cast<ULRGameInstance>(GetGameInstance()))
	{
		GI->PersistentInventory.Empty();
		GI->PersistentToolbarItems.Empty();
		GI->bHasTravelData = true;
	}

	// 모든 LRBot AI를 Patrol로 강제 복귀 (옛 사망 위치 응시 방지)
	for (TActorIterator<ALRBotAIController> It(GetWorld()); It; ++It)
	{
		if (ALRBotAIController* BotAI = *It)
		{
			BotAI->AbortToPatrol();
		}
	}

	if (APawn* OldPawn = DeadController->GetPawn())
	{
		DeadController->UnPossess();
		OldPawn->Destroy();
	}

	// 위험지대: RespawnLevelName이 설정되어 있으면 해당 레벨로 이동
	if (!RespawnLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, RespawnLevelName);
		return;
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