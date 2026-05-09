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

private:
	UFUNCTION()
	void RespawnPlayer(AController* Controller);
};