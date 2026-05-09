#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "LRBotAIController.generated.h"

class ALRBot;
class UBehaviorTree;
class UBlackboardComponent;

UCLASS()
class LASTREFUGE_API ALRBotAIController : public AAIController
{
	GENERATED_BODY()

public:
	ALRBotAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

private:
	/** 빙의한 Bot 캐시 */
	TObjectPtr<ALRBot> ControlledBot;

	/** Perception 콜백 — Day 4와 동일 */
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// ── Blackboard 키 이름 상수 ──────────────────────────
	// BB_LRBot의 키 이름과 반드시 일치해야 함
	static const FName BB_PatrolTarget;   // "PatrolTarget"
	static const FName BB_PatrolIndex;    // "PatrolIndex"
};