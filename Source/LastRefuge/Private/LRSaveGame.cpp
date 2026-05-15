#include "LRSaveGame.h"
#include "Components/LRInventoryGridComponent.h"
#include "Items/LRItemDataAsset.h"
#include "Kismet/GameplayStatics.h"

const FString ULRSaveGame::SlotName  = TEXT("LRInventorySave");
const int32   ULRSaveGame::UserIndex = 0;

void ULRSaveGame::Save(
	ULRInventoryGridComponent* InvGrid,
	ULRInventoryGridComponent* StorageGrid,
	const TArray<FLRGridItem>& ToolbarItems,
	UObject* WorldCtx)
{
	if (!WorldCtx) return;

	ULRSaveGame* SaveObj = Cast<ULRSaveGame>(
		UGameplayStatics::CreateSaveGameObject(ULRSaveGame::StaticClass()));
	if (!SaveObj) return;

	auto SerializeGrid = [&](ULRInventoryGridComponent* Grid, bool bIsStorage)
	{
		if (!Grid) return;
		for (const auto& [ID, Item] : Grid->GetItems())
		{
			if (!Item.ItemData) continue;
			FLRSavedItem SI;
			SI.ItemID     = Item.ItemData->ItemID;
			SI.GridX      = Item.GridX;
			SI.GridY      = Item.GridY;
			SI.bIsRotated = Item.bIsRotated;
			SI.bIsStorage = bIsStorage;
			SI.Quantity   = Item.Quantity;
			SaveObj->SavedItems.Add(SI);
		}
	};

	SerializeGrid(InvGrid,     false);
	SerializeGrid(StorageGrid, true);

	// 툴바 직렬화
	for (int32 i = 0; i < ToolbarItems.Num(); ++i)
	{
		const FLRGridItem& Item = ToolbarItems[i];
		if (Item.IsEmpty() || !Item.ItemData) continue;
		FLRSavedItem SI;
		SI.ItemID      = Item.ItemData->ItemID;
		SI.bIsToolbar  = true;
		SI.ToolbarSlot = i;
		SI.Quantity    = Item.Quantity;
		SaveObj->SavedToolbarItems.Add(SI);
	}

	const bool bOK = UGameplayStatics::SaveGameToSlot(
		SaveObj, ULRSaveGame::SlotName, ULRSaveGame::UserIndex);

	UE_LOG(LogTemp, Log, TEXT("[SaveGame] %s — 인벤/창고: %d, 툴바: %d"),
		bOK ? TEXT("저장 성공") : TEXT("저장 실패"),
		SaveObj->SavedItems.Num(),
		SaveObj->SavedToolbarItems.Num());
}

void ULRSaveGame::Load(
	ULRInventoryGridComponent* InvGrid,
	ULRInventoryGridComponent* StorageGrid,
	TArray<FLRGridItem>& OutToolbarItems,
	const TMap<FString, ULRItemDataAsset*>& ItemRegistry,
	UObject* WorldCtx)
{
	if (!WorldCtx || !InvGrid || !StorageGrid) return;

	if (!UGameplayStatics::DoesSaveGameExist(ULRSaveGame::SlotName, ULRSaveGame::UserIndex))
	{
		UE_LOG(LogTemp, Log, TEXT("[SaveGame] 저장 파일 없음 — 빈 인벤토리로 시작"));
		return;
	}

	ULRSaveGame* SaveObj = Cast<ULRSaveGame>(
		UGameplayStatics::LoadGameFromSlot(ULRSaveGame::SlotName, ULRSaveGame::UserIndex));
	if (!SaveObj) return;

	InvGrid->ClearGrid();
	StorageGrid->ClearGrid();
	OutToolbarItems.SetNum(4);

	int32 Restored = 0, Skipped = 0;

	// 인벤토리 + 창고 복원
	for (const FLRSavedItem& SI : SaveObj->SavedItems)
	{
		const ULRItemDataAsset* const* DataPtr = ItemRegistry.Find(SI.ItemID);
		if (!DataPtr || !(*DataPtr)) { Skipped++; continue; }

		FLRGridItem Item;
		Item.ItemData = const_cast<ULRItemDataAsset*>(*DataPtr);
		Item.Width    = (*DataPtr)->GridWidth;
		Item.Height   = (*DataPtr)->GridHeight;
		Item.Quantity = FMath::Max(1, SI.Quantity);

		ULRInventoryGridComponent* Target = SI.bIsStorage ? StorageGrid : InvGrid;
		if (Target->PlaceItem(SI.GridX, SI.GridY, Item, SI.bIsRotated) != INDEX_NONE)
		{
			Restored++;
		}
		else
		{
			int32 OutX, OutY; bool bRot;
			if (Target->FindEmptySpace(Item, OutX, OutY, bRot))
				{ Target->PlaceItem(OutX, OutY, Item, bRot); Restored++; }
			else
				Skipped++;
		}
	}

	// 툴바 복원
	for (const FLRSavedItem& SI : SaveObj->SavedToolbarItems)
	{
		if (!SI.bIsToolbar || !OutToolbarItems.IsValidIndex(SI.ToolbarSlot)) continue;
		const ULRItemDataAsset* const* DataPtr = ItemRegistry.Find(SI.ItemID);
		if (!DataPtr || !(*DataPtr)) continue;

		FLRGridItem Item;
		Item.ItemData = const_cast<ULRItemDataAsset*>(*DataPtr);
		Item.Width    = (*DataPtr)->GridWidth;
		Item.Height   = (*DataPtr)->GridHeight;
		Item.Quantity = FMath::Max(1, SI.Quantity);
		OutToolbarItems[SI.ToolbarSlot] = Item;
	}

	UE_LOG(LogTemp, Log, TEXT("[SaveGame] 로드 완료 — 복원: %d, 스킵: %d"), Restored, Skipped);
}
