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

	// 아이템 추가 시도
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(ULRItemDataAsset* Item, int32 Amount);

	// 아이템 제거 시도
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