#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LRGameMode.generated.h"

UCLASS()
class LASTREFUGE_API ALRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "GameMode")
	void OnPlayerDied(AController* DeadController);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameMode")
	float RespawnDelay = 3.f;

	// 설정 시 사망하면 해당 레벨로 이동 (위험지대용, 기지는 비워두면 제자리 리스폰)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameMode")
	FName RespawnLevelName;

private:
	UFUNCTION()
	void RespawnPlayer(AController* Controller);
};