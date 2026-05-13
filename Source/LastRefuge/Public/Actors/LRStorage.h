#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/LRInteractable.h"
#include "LRStorage.generated.h"

class ULRInventoryGridComponent;

UCLASS()
class LASTREFUGE_API ALRStorage : public AActor, public ILRInteractable
{
	GENERATED_BODY()

public:
	ALRStorage();

	virtual void BeginPlay() override;
	virtual void BeginInteract(class ALRCharacter* Player) override;
	virtual void EndInteract(class ALRCharacter* Player) override;
	virtual float GetInteractionDuration() const override;
	virtual FText GetInteractionPrompt() const override;
	virtual FText GetProgressText() const override;
	virtual FText GetStartText() const override;
	virtual FText GetCancelText() const override;
	virtual FText GetCompleteText() const override;

	UFUNCTION(BlueprintPure, Category = "LR|Storage")
	ULRInventoryGridComponent* GetStorageGrid() const { return StorageGrid; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> MeshComponent;

	// 창고 그리드 (10x5 기본)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storage")
	TObjectPtr<ULRInventoryGridComponent> StorageGrid;
};
