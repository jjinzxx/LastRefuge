#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/LRInteractable.h"
#include "LROpenableDoor.generated.h"

/**
 * 열고닫을 수 있는 문.
 * - E키 즉시 토글 (GetInteractionDuration = 0)
 * - Tick에서 FInterpTo로 부드러운 회전 애니메이션
 * - 선택적 키카드 잠금 (bRequiresKeyCard + RequiredKeyCardID)
 */
UCLASS()
class LASTREFUGE_API ALROpenableDoor : public AActor, public ILRInteractable
{
	GENERATED_BODY()

public:
	ALROpenableDoor();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	virtual bool  CanInteract(class ALRCharacter* Player) const override;
	virtual void  BeginInteract(class ALRCharacter* Player) override;
	virtual void  EndInteract(class ALRCharacter* Player) override;
	virtual float GetInteractionDuration() const override { return 0.f; }
	virtual FText GetInteractionPrompt()   const override;
	virtual FText GetProgressText()        const override { return FText::GetEmpty(); }
	virtual FText GetStartText()           const override { return FText::GetEmpty(); }
	virtual FText GetCancelText()          const override { return FText::GetEmpty(); }
	virtual FText GetCompleteText()        const override { return FText::GetEmpty(); }

protected:
	/** 경첩 피벗 — 이 컴포넌트를 기준으로 문이 회전함 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USceneComponent> HingeRoot;

	/** 문짝 메시 — HingeRoot에 부착되어 오프셋만큼 치우침 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> MeshComponent;

	// === 동작 ===
	/**
	 * 경첩에서 문짝 중심까지의 Y축 오프셋 (cm).
	 * 문 메시 폭의 절반값으로 설정하면 경첩이 문 끝에 위치함.
	 * 예) 문 폭 100cm → 50 입력
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float HingeOffsetY = 50.f;

	/** 열렸을 때 Yaw 회전각 (도). 음수면 반대 방향으로 열림 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float OpenAngle = 90.f;

	/** 열기/닫기 보간 속도 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float OpenSpeed = 3.f;

	// === 잠금 ===
	/** true면 RequiredKeyCardID 키카드가 인벤토리에 있어야 열림 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock")
	bool bRequiresKeyCard = false;

	/** bRequiresKeyCard = true일 때 필요한 키카드 ItemID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock",
		meta = (EditCondition = "bRequiresKeyCard", EditConditionHides))
	FString RequiredKeyCardID;

private:
	bool  bIsOpen = false;
	float TargetYaw  = 0.f;   // 목표 Yaw (열림: OpenAngle, 닫힘: 0)
	float ClosedYaw  = 0.f;   // BeginPlay에서 초기 Yaw 캐싱
};
