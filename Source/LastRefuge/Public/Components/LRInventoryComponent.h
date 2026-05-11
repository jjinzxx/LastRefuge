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

	// 링커 에러 방지를 위해 정의가 확실히 존재하는 함수들만 선언합니다.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(ULRItemDataAsset* Item, int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(ULRItemDataAsset* Item, int32 Amount);

	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryUpdated OnInventoryUpdated;
    
	// 슬롯 인덱스를 받아 아이템 효과를 적용하고 수량을 차감합니다.
	UFUNCTION(BlueprintCallable, Category = "LR|Inventory")
	bool UseItem(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "LR|Inventory")
	const TArray<FLRItemSlot>& GetInventorySlots() const { return InventorySlots; }

	UFUNCTION(BlueprintCallable, Category = "LR|Inventory")
	void ClearInventory();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 InventorySlotsMax = 20;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FLRItemSlot> InventorySlots;
    
private:
	/** 실제 아이템 효과를 캐릭터에게 전달하는 내부 보조 함수 */
	void ApplyItemEffects(const class ULRItemDataAsset* ItemData);
};