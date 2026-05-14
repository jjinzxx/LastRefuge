#include "LRSaveGame.h"
#include "Components/LRInventoryGridComponent.h"
#include "Items/LRItemDataAsset.h"
#include "Kismet/GameplayStatics.h"

const FString ULRSaveGame::SlotName  = TEXT("LRInventorySave");
const int32   ULRSaveGame::UserIndex = 0;

// ──────────────────────────────────────────────────────────
// Save
// 두 그리드를 순회하며 FLRSavedItem으로 직렬화 → USaveGame 슬롯에 기록.
// ──────────────────────────────────────────────────────────
void ULRSaveGame::Save(
	ULRInventoryGridComponent* InvGrid,
	ULRInventoryGridComponent* StorageGrid,
	UObject* WorldCtx)
{
	if (!WorldCtx) return;

	ULRSaveGame* SaveObj = Cast<ULRSaveGame>(
		UGameplayStatics::CreateSaveGameObject(ULRSaveGame::StaticClass()));
	if (!SaveObj) return;

	// 직렬화 람다 — 그리드 컴포넌트 하나를 SavedItems에 추가
	auto Serialize = [&](ULRInventoryGridComponent* Grid, bool bIsStorage)
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

	Serialize(InvGrid,     false);
	Serialize(StorageGrid, true);

	const bool bOK = UGameplayStatics::SaveGameToSlot(
		SaveObj, ULRSaveGame::SlotName, ULRSaveGame::UserIndex);

	UE_LOG(LogTemp, Log, TEXT("[SaveGame] %s — %d 아이템"),
		bOK ? TEXT("저장 성공") : TEXT("저장 실패"),
		SaveObj->SavedItems.Num());
}

// ──────────────────────────────────────────────────────────
// Load
// SaveGame 슬롯을 읽어 두 그리드를 복원.
// ItemID가 ItemRegistry에 없으면 경고 후 스킵 (데이터 무결성 보호).
// ──────────────────────────────────────────────────────────
void ULRSaveGame::Load(
	ULRInventoryGridComponent* InvGrid,
	ULRInventoryGridComponent* StorageGrid,
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

	int32 Restored = 0;
	int32 Skipped  = 0;

	for (const FLRSavedItem& SI : SaveObj->SavedItems)
	{
		const ULRItemDataAsset* const* DataPtr = ItemRegistry.Find(SI.ItemID);
		if (!DataPtr || !(*DataPtr))
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[SaveGame] ItemID '%s' 없음 — 스킵"), *SI.ItemID);
			Skipped++;
			continue;
		}

		FLRGridItem Item;
		Item.ItemData  = const_cast<ULRItemDataAsset*>(*DataPtr);
		Item.Width     = (*DataPtr)->GridWidth;
		Item.Height    = (*DataPtr)->GridHeight;
		Item.Quantity  = FMath::Max(1, SI.Quantity);

		ULRInventoryGridComponent* Target = SI.bIsStorage ? StorageGrid : InvGrid;
		const int32 PlacedID = Target->PlaceItem(SI.GridX, SI.GridY, Item, SI.bIsRotated);

		if (PlacedID != INDEX_NONE)
			Restored++;
		else
		{
			// 저장된 좌표에 배치 실패 시 빈 공간에 재배치 시도
			int32 OutX, OutY;
			bool bOutRot;
			if (Target->FindEmptySpace(Item, OutX, OutY, bOutRot))
			{
				Target->PlaceItem(OutX, OutY, Item, bOutRot);
				Restored++;
				UE_LOG(LogTemp, Warning,
					TEXT("[SaveGame] ItemID '%s' 좌표 충돌 — 빈 공간 (%d,%d)으로 이동"),
					*SI.ItemID, OutX, OutY);
			}
			else
			{
				UE_LOG(LogTemp, Error,
					TEXT("[SaveGame] ItemID '%s' 배치 공간 없음 — 유실"), *SI.ItemID);
				Skipped++;
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[SaveGame] 로드 완료 — 복원: %d, 스킵: %d"),
		Restored, Skipped);
}
