#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Items/LRInventoryStructs.h"
#include "LRGameInstance.generated.h"

UCLASS()
class LASTREFUGE_API ULRGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 레벨 이동 시 플레이어 인벤토리 보존 (UPROPERTY 필수 — GC가 DataAsset 포인터를 날리지 않도록)
	UPROPERTY()
	TArray<FLRItemSlot> PersistentInventory;

	// 기지 보관함 내용 보존
	UPROPERTY()
	TArray<FLRItemSlot> PersistentStorageItems;

	// 레벨 이동이 한 번이라도 일어났는지 (초기 스폰과 구분)
	bool bHasTravelData = false;
};
