#pragma once

#include "CoreMinimal.h"
#include "Items/LRItemDataAsset.h"
#include "LRInventoryStructs.generated.h"

USTRUCT(BlueprintType)
struct FLRItemSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<ULRItemDataAsset> ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Quantity;

	FLRItemSlot() : ItemData(nullptr), Quantity(0) {}
    
	// 유효성 확인을 위한 헬퍼 함수
	bool IsEmpty() const { return ItemData == nullptr || Quantity <= 0; }
};