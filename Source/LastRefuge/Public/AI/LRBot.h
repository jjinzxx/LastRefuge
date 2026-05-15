#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LRBot.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UBehaviorTree;

// ELRBotState
UENUM(BlueprintType)
enum class ELRBotState : uint8
{
	Patrol      UMETA(DisplayName = "Patrol"),
	Suspicious  UMETA(DisplayName = "Suspicious"),
	Combat      UMETA(DisplayName = "Combat")
};

UCLASS()
class LASTREFUGE_API ALRBot : public ACharacter
{
	GENERATED_BODY()

public:
	ALRBot();
	
	/** 봇이 사용할 BehaviorTree 자산. BP에서 BT_LRBot 할당 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LR|AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	/**
	 * 봇이 순찰할 웨이포인트 액터 배열.
	 * 맵에 배치된 ATargetPoint(또는 임의 액터)를 BP_LRBot 인스턴스에 직접 할당.
	 * 인덱스 0 → 1 → 2 → 0 순환.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LR|AI")
	TArray<TObjectPtr<AActor>> PatrolPoints;

	/** AIController가 BT 시작 시 호출. 자산이 비었으면 nullptr 반환 */
	UFUNCTION(BlueprintCallable, Category = "LR|AI")
	UBehaviorTree* GetBehaviorTreeAsset() const { return BehaviorTreeAsset; }

	/** 인덱스에 해당하는 웨이포인트의 월드 위치. 잘못된 인덱스면 ZeroVector + bIsValid=false */
	FVector GetPatrolPointLocation(int32 Index, bool& bIsValid) const;

	/** 순찰 지점 개수 */
	int32 GetPatrolPointCount() const { return PatrolPoints.Num(); }
	
	UAIPerceptionComponent* GetPerceptionComponent() const
	{
		return PerceptionComponent;
	}
	
	// bot 순찰 걷기 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LR|AI|Movement")
	float PatrolSpeed = 180.f;

	// bot 순찰 뛰기 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LR|AI|Movement")
	float ChaseSpeed = 250.f;
	
	virtual void Tick(float DeltaTime) override;
	
	// 상태
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|State")
	ELRBotState CurrentState = ELRBotState::Patrol;

	// LKL
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|State")
	FVector LastKnownLocation = FVector::ZeroVector;

	// 근접 공격 수치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float MeleeDamage = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float MeleeInterval = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	float MeleeRange = 150.f;

	// 근접 공격 애니메이션 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	TObjectPtr<UAnimMontage> MeleeAttackMontage;

	// 사망 애니메이션 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
	TObjectPtr<UAnimMontage> DeathMontage;

	// 제압당해 사망 처리
	void TakedownKill();

	bool bIsDead = false;

	// 상태 전환
	void SetBotState(ELRBotState NewState);
	ELRBotState GetBotState() const { return CurrentState; }

	// AI 감지 보이스
	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> SFX_Alert;

	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> SFX_Suspicious;

	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> SFX_ReturnToPatrol;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> PerceptionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception|Sight")
	float SightRadius = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception|Sight")
	float LoseSightRadius = 1800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception|Sight")
	float PeripheralVisionHalfAngleDegrees = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception|Hearing")
	float HearingRange = 1800.f;
	
	
};