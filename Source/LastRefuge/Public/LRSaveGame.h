#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Items/LRInventoryStructs.h"
#include "LRSaveGame.generated.h"

class ULRInventoryGridComponent;
class ULRItemDataAsset;

/**
 * 인벤토리 + 창고 전체를 한 파일로 저장.
 * SlotName = "LRInventorySave", UserIndex = 0.
 *
 * 저장: ULRSaveGame::Save(InvGrid, StorageGrid, WorldCtx)
 * 로드: ULRSaveGame::Load(InvGrid, StorageGrid, ItemRegistry, WorldCtx)
 *
 * ItemRegistry: "ItemID 문자열" → ULRItemDataAsset* 매핑.
 * ULRGameInstance에 TMap으로 보관하여 전달하거나,
 * AssetManager로 동기 로드하여 전달한다.
 */
UCLASS()
class LASTREFUGE_API ULRSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static const FString SlotName;
	static const int32   UserIndex;

	// 직렬화된 아이템 목록 (인벤토리 + 창고 모두 포함)
	UPROPERTY()
	TArray<FLRSavedItem> SavedItems;

	// 직렬화된 툴바 슬롯 (최대 4개)
	UPROPERTY()
	TArray<FLRSavedItem> SavedToolbarItems;

	/**
	 * StorageGrid가 null(현재 맵에 창고 없음)이고 FallbackStorageItems가 있으면
	 * 그 배열을 창고 데이터로 직렬화한다 (DangerZone → GI->PersistentStorageItems 전달).
	 */
	static void Save(ULRInventoryGridComponent* InvGrid,
	                 ULRInventoryGridComponent* StorageGrid,
	                 const TArray<FLRGridItem>& ToolbarItems,
	                 UObject* WorldCtx,
	                 const TArray<FLRGridItem>* FallbackStorageItems = nullptr);

	static void Load(ULRInventoryGridComponent* InvGrid,
	                 ULRInventoryGridComponent* StorageGrid,
	                 TArray<FLRGridItem>& OutToolbarItems,
	                 const TMap<FString, ULRItemDataAsset*>& ItemRegistry,
	                 UObject* WorldCtx);
};
