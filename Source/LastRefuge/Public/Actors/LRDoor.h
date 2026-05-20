#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/LRInteractable.h"
#include "LRDoor.generated.h"

UCLASS()
class LASTREFUGE_API ALRDoor : public AActor, public ILRInteractable
{
	GENERATED_BODY()

public:
	ALRDoor();

	virtual bool  CanInteract(class ALRCharacter* Player) const override;
	virtual void BeginInteract(class ALRCharacter* Player) override;
	virtual void EndInteract(class ALRCharacter* Player) override;
	virtual float GetInteractionDuration() const override;
	virtual FText GetInteractionPrompt() const override;
	virtual FText GetProgressText() const override;
	virtual FText GetStartText() const override;
	virtual FText GetCancelText() const override;
	virtual FText GetCompleteText() const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> MeshComponent;

	// 이동할 레벨 이름 (예: "L_Base", "L_DangerZone")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	FName TargetLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	FText PromptText;

	// 이동 전 대기 시간 (기본 5초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float InteractionDuration = 5.f;

	// true = 이동 전에 현재 맵의 ALRStorage 내용도 GameInstance에 저장
	// 기지 출구 문에만 true로 설정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	bool bSaveStorage = false;

	// === 잠금 ===
	/** true면 RequiredKeyCardID에 해당하는 키카드가 인벤토리에 있어야 통과 가능 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock")
	bool bRequiresKeyCard = false;

	/** bRequiresKeyCard = true일 때 필요한 키카드 ItemID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lock",
		meta = (EditCondition = "bRequiresKeyCard", EditConditionHides))
	FString RequiredKeyCardID;
};
