#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/LRInventoryStructs.h"
#include "LRInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LASTREFUGE_API ULRInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRInventoryComponent();

protected:
	// 아래 두 줄의 선언이 추가되어야 합니다.
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(ULRItemDataAsset* Item, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(ULRItemDataAsset* Item, int32 Amount);

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 InventorySlotsMax = 20;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FLRItemSlot> InventorySlots;
};