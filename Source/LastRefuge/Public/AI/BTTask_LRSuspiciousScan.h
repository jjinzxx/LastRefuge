#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "AI/LRBot.h"                  // ELRBotState 포함
#include "BTTask_LRSuspiciousScan.generated.h"

UCLASS()
class LASTREFUGE_API UBTTask_LRSuspiciousScan : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_LRSuspiciousScan();

	UPROPERTY(EditAnywhere, Category = "AI")
	float ScanDuration = 3.f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickTask(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	float ElapsedTime = 0.f;
	float BaseYaw = 0.f;
};