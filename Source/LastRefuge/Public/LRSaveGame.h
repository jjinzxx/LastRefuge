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

	/**
	 * 두 그리드의 현재 상태를 SaveGame 슬롯에 기록.
	 * @param WorldCtx  GetWorld() 가능한 임의의 UObject
	 */
	static void Save(ULRInventoryGridComponent* InvGrid,
	                 ULRInventoryGridComponent* StorageGrid,
	                 UObject* WorldCtx);

	/**
	 * SaveGame 슬롯에서 두 그리드를 복원.
	 * ItemRegistry에 없는 ItemID는 경고 후 스킵.
	 * @param ItemRegistry  "ItemID" → DataAsset* 맵 (호출 측에서 구성)
	 */
	static void Load(ULRInventoryGridComponent* InvGrid,
	                 ULRInventoryGridComponent* StorageGrid,
	                 const TMap<FString, ULRItemDataAsset*>& ItemRegistry,
	                 UObject* WorldCtx);
};
