#include "AI/BTTask_LRSuspiciousScan.h"
#include "AI/LRBotAIController.h"
#include "AI/LRBot.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_LRSuspiciousScan::UBTTask_LRSuspiciousScan()
{
	NodeName = TEXT("LR Suspicious Scan");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_LRSuspiciousScan::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ElapsedTime = 0.f;

	// 스캔 시작 시점의 Yaw를 저장
	ALRBotAIController* Controller =
		Cast<ALRBotAIController>(OwnerComp.GetAIOwner());
	if (APawn* Bot = Controller ? Controller->GetPawn() : nullptr)
	{
		BaseYaw = Bot->GetActorRotation().Yaw;
	}

	return EBTNodeResult::InProgress;
}

void UBTTask_LRSuspiciousScan::TickTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	ElapsedTime += DeltaSeconds;

	ALRBotAIController* Controller =
		Cast<ALRBotAIController>(OwnerComp.GetAIOwner());
	APawn* Bot = Controller ? Controller->GetPawn() : nullptr;

	if (Bot)
	{
		// BaseYaw 기준으로 결정론적 회전
		float TargetYaw = BaseYaw;
		if      (ElapsedTime < 1.f) TargetYaw = BaseYaw - 90.f;
		else if (ElapsedTime < 2.f) TargetYaw = BaseYaw + 90.f;
		// 2~3초: 정면(BaseYaw) 유지

		FRotator Current = Bot->GetActorRotation();
		FRotator Target  = FRotator(Current.Pitch, TargetYaw, Current.Roll);

		Bot->SetActorRotation(
			FMath::RInterpTo(Current, Target, DeltaSeconds, 2.f));
	}

	if (ElapsedTime >= ScanDuration)
	{
		if (Controller)
		{
			Controller->EnterPatrol();
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
}