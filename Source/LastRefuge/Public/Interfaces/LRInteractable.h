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
	/** 상호작용 가능 여부 검사 (false면 TryInteract에서 차단) */
	virtual bool CanInteract(class ALRCharacter* Player) const { return true; }

	/** 상호작용 시작 (E키를 눌렀을 때) */
	virtual void BeginInteract(class ALRCharacter* Player) = 0;
    
	/** 상호작용 종료 또는 중단 */
	virtual void EndInteract(class ALRCharacter* Player) = 0;

	/** 수색에 걸리는 시간 반환 (0이면 즉시 완료) */
	virtual float GetInteractionDuration() const = 0;

	/** 조준 시 화면에 표시할 상호작용 프롬프트 텍스트 */
	virtual FText GetInteractionPrompt() const = 0;

	/** 상호작용 진행 중 화면에 표시할 텍스트 */
	virtual FText GetProgressText() const = 0;

	/** 상호작용 시작 시 표시할 텍스트 */
	virtual FText GetStartText() const = 0;

	/** 상호작용 취소 시 표시할 텍스트 */
	virtual FText GetCancelText() const = 0;

	/** 상호작용 완료 시 표시할 텍스트 */
	virtual FText GetCompleteText() const = 0;
};