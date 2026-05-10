#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AI/LRBot.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "LRBotAIController.generated.h"

UCLASS()
class LASTREFUGE_API ALRBotAIController : public AAIController
{
	GENERATED_BODY()

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

public:
	void EnterPatrol();

	/** 플레이어 사망 시 GameMode가 호출 — Combat/Suspicious를 끊고 Patrol로 강제 복귀 */
	void AbortToPatrol();

private:
	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void EnterCombat(AActor* PlayerActor);
	void EnterSuspicious(const FVector& InLKL);

	void StartFiring();
	void StopFiring();
	void FireAtPlayer();

	// Combat 중 실시간 위치 추적 타이머
	void UpdateCombatTarget();

	UPROPERTY()
	ALRBot* ControlledBot = nullptr;

	UPROPERTY()
	AActor* TrackedPlayer = nullptr;

	FTimerHandle FireTimerHandle;
	FTimerHandle CombatTrackingHandle;  // ← 추가

	static const FName BB_PatrolTarget;
	static const FName BB_PatrolIndex;
	static const FName BB_PlayerActor;
	static const FName BB_BotState;
	static const FName BB_LKL;
};