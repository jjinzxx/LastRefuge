#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LRInteractable.generated.h"

UINTERFACE(MinimalAPI)
class ULRInteractable : public UInterface
{
	GENERATED_BODY()
};

class LASTREFUGE_API ILRInteractable
{
	GENERATED_BODY()

public:
	/** 상호작용 시작 (E키를 눌렀을 때) */
	virtual void BeginInteract(class ALRCharacter* Player) = 0;
    
	/** 상호작용 종료 또는 중단 */
	virtual void EndInteract(class ALRCharacter* Player) = 0;

	/** 수색에 걸리는 시간 반환 (0이면 즉시 완료) */
	virtual float GetInteractionDuration() const = 0;
};