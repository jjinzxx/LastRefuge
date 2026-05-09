#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LRBotAIController.generated.h"

class UAIPerceptionComponent;

UCLASS()
class LASTREFUGE_API ALRBotAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALRBotAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	/** Bot의 PerceptionComponent에서 감지 이벤트를 받는 콜백 */
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/** 현재 빙의한 Bot의 PerceptionComponent 캐싱 */
	TObjectPtr<UAIPerceptionComponent> BotPerceptionComponent;
};