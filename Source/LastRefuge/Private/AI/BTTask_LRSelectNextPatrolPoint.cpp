#include "AI/BTTask_LRSelectNextPatrolPoint.h"
#include "AI/LRBot.h"
#include "AI/LRBotAIController.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

// BB 키 이름 — ALRBotAIController의 상수와 반드시 일치
static const FName BB_PatrolTarget = TEXT("PatrolTarget");
static const FName BB_PatrolIndex  = TEXT("PatrolIndex");

UBTTask_LRSelectNextPatrolPoint::UBTTask_LRSelectNextPatrolPoint()
{
	NodeName = TEXT("LR Select Next Patrol Point");
}

EBTNodeResult::Type UBTTask_LRSelectNextPatrolPoint::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// ── 1. 필요한 객체 가져오기 ───────────────────────────
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	ALRBotAIController* Controller = Cast<ALRBotAIController>(OwnerComp.GetAIOwner());
	if (!BB || !Controller)
	{
		return EBTNodeResult::Failed;
	}

	ALRBot* Bot = Cast<ALRBot>(Controller->GetPawn());
	if (!Bot || Bot->GetPatrolPointCount() == 0)
	{
		return EBTNodeResult::Failed;
	}

	// ── 2. 현재 인덱스 읽기 → 다음 인덱스 계산 (순환) ───
	const int32 CurrentIndex = BB->GetValueAsInt(BB_PatrolIndex);
	const int32 NextIndex    = (CurrentIndex + 1) % Bot->GetPatrolPointCount();

	// ── 3. 다음 웨이포인트 위치 가져오기 ─────────────────
	bool bValid = false;
	const FVector NextLocation = Bot->GetPatrolPointLocation(NextIndex, bValid);
	if (!bValid)
	{
		return EBTNodeResult::Failed;
	}

	// ── 4. Blackboard 갱신 ────────────────────────────────
	BB->SetValueAsInt(BB_PatrolIndex, NextIndex);
	BB->SetValueAsVector(BB_PatrolTarget, NextLocation);

	return EBTNodeResult::Succeeded;
}

FString UBTTask_LRSelectNextPatrolPoint::GetStaticDescription() const
{
	return TEXT("PatrolIndex 를 순환 증가시키고\nPatrolTarget 에 다음 위치를 기록");
}