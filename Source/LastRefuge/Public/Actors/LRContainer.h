#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/LRInteractable.h"
#include "Items/LRInventoryStructs.h"
#include "LRContainer.generated.h"

UCLASS()
class LASTREFUGE_API ALRContainer : public AActor, public ILRInteractable
{
	GENERATED_BODY()

public:
	ALRContainer();

	// --- ILRInteractable 인터페이스 구현부 ---
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
	class UStaticMeshComponent* MeshComponent;

	// 이 컨테이너에서 획득할 수 있는 아이템 목록 (에디터에서 배치할 때 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TArray<FLRItemSlot> LootTable;

	// 수색에 필요한 시간 (기획서 기준 3초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	float SearchDuration = 3.0f;

	// 한 번 수색하면 다시 못 하도록 막는 플래그
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	bool bSearched = false;
};
