#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_LRSelectNextPatrolPoint.generated.h"

/**
 * Blackboard의 PatrolIndex를 다음으로 증가(순환)시키고
 * 해당 웨이포인트 위치를 PatrolTarget에 기록한다.
 */
UCLASS()
class LASTREFUGE_API UBTTask_LRSelectNextPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_LRSelectNextPatrolPoint();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** BT 노드 설명 텍스트 (에디터에서 노드 위에 표시됨) */
	virtual FString GetStaticDescription() const override;
};