#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/LRInteractable.h"
#include "LRContainer.generated.h"

class ULRInventoryGridComponent;

UENUM(BlueprintType)
enum class ELRContainerType : uint8
{
	Normal   UMETA(DisplayName = "일반"),
	Locked   UMETA(DisplayName = "잠금 (키카드 필요)"),
	Alarmed  UMETA(DisplayName = "경보 (수색 완료 시 AI 경보)")
};

UCLASS()
class LASTREFUGE_API ALRContainer : public AActor, public ILRInteractable
{
	GENERATED_BODY()

public:
	ALRContainer();

	virtual void BeginPlay() override;
	virtual bool CanInteract(class ALRCharacter* Player) const override;
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

	// LootTable 아이템을 담는 그리드 (BeginPlay에서 자동 초기화)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULRInventoryGridComponent> ContainerGrid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TArray<TObjectPtr<class ULRItemDataAsset>> LootTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	float SearchDuration = 3.0f;

	// === 보안 등급 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security")
	ELRContainerType ContainerType = ELRContainerType::Normal;

	/** Locked 타입일 때 필요한 키카드 ItemID (에디터에서 설정) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Security",
		meta = (EditCondition = "ContainerType == ELRContainerType::Locked", EditConditionHides))
	FString RequiredKeyCardID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot")
	bool bSearched = false;

	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> SFX_SearchStart;

	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> SFX_SearchLoop;

	UPROPERTY(EditAnywhere, Category = "Sound")
	TObjectPtr<USoundBase> SFX_SearchComplete;

	UPROPERTY(VisibleAnywhere, Category = "Sound")
	TObjectPtr<UAudioComponent> SearchLoopAudioComp;
};
