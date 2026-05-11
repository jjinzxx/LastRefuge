#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/LRInteractable.h"
#include "Items/LRInventoryStructs.h"
#include "LRStorage.generated.h"

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

	UFUNCTION(BlueprintPure, Category = "LR|Storage")
	const TArray<FLRItemSlot>& GetStoredItems() const { return StoredItems; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Storage")
	TArray<FLRItemSlot> StoredItems;
};
