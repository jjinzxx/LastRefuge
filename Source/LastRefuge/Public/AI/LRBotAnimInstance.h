#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AI/LRBot.h"
#include "LRBotAnimInstance.generated.h"

UCLASS()
class LASTREFUGE_API ULRBotAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// 이동 속도 (State Machine 조건에 사용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LR|Anim")
	float Speed = 0.f;

	// 봇 상태 (Combat 진입 시 Run 전환 등)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LR|Anim")
	ELRBotState BotState = ELRBotState::Patrol;
};
