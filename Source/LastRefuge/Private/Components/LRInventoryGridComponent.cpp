#include "Components/LRInventoryGridComponent.h"
#include "Components/LRStatusComponent.h"
#include "Items/LRItemDataAsset.h"
#include "GameFramework/Actor.h"

ULRInventoryGridComponent::ULRInventoryGridComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULRInventoryGridComponent::BeginPlay()
{
	Super::BeginPlay();
	Grid.Init(INDEX_NONE, GridWidth * GridHeight);
}

// ──────────────────────────────────────────────────────────────
// CheckPlacement
// 범위 + 겹침 검사. Grid를 읽기만 하므로 const.
// ──────────────────────────────────────────────────────────────
bool ULRInventoryGridComponent::CheckPlacement(
	int32 X, int32 Y, const FLRGridItem& Item, bool bRotated) const
{
	const int32 W = bRotated ? Item.Height : Item.Width;
	const int32 H = bRotated ? Item.Width  : Item.Height;

	// 그리드 경계 체크
	if (X < 0 || Y < 0 || X + W > GridWidth || Y + H > GridHeight)
		return false;

	// 모든 점유 예정 셀이 비어 있는지 확인
	for (int32 dy = 0; dy < H; dy++)
	{
		for (int32 dx = 0; dx < W; dx++)
		{
			if (Grid[(Y + dy) * GridWidth + (X + dx)] != INDEX_NONE)
				return false;
		}
	}
	return true;
}

// ──────────────────────────────────────────────────────────────
// PlaceItem
// CheckPlacement 통과 시 Grid에 ID 기록 + Items 맵에 등록.
// ──────────────────────────────────────────────────────────────
int32 ULRInventoryGridComponent::PlaceItem(
	int32 X, int32 Y, FLRGridItem Item, bool bRotated)
{
	Item.bIsRotated = bRotated;

	if (!CheckPlacement(X, Y, Item, bRotated))
		return INDEX_NONE;

	Item.GridX = X;
	Item.GridY = Y;

	const int32 ID = NextItemID++;
	Items.Add(ID, Item);

	const int32 W = Item.GetEffectiveWidth();
	const int32 H = Item.GetEffectiveHeight();

	for (int32 dy = 0; dy < H; dy++)
		for (int32 dx = 0; dx < W; dx++)
			Grid[(Y + dy) * GridWidth + (X + dx)] = ID;

	OnGridChanged.Broadcast();
	return ID;
}

// ──────────────────────────────────────────────────────────────
// RemoveItem
// ID 기반이라 배열 인덱스 재조정 불필요. O(W*H) 셀 초기화.
// ──────────────────────────────────────────────────────────────
FLRGridItem ULRInventoryGridComponent::RemoveItem(int32 InItemID)
{
	FLRGridItem* Found = Items.Find(InItemID);
	if (!Found)
		return FLRGridItem{};

	const FLRGridItem Item = *Found;
	const int32 W = Item.GetEffectiveWidth();
	const int32 H = Item.GetEffectiveHeight();

	for (int32 dy = 0; dy < H; dy++)
		for (int32 dx = 0; dx < W; dx++)
			Grid[(Item.GridY + dy) * GridWidth + (Item.GridX + dx)] = INDEX_NONE;

	Items.Remove(InItemID);
	OnGridChanged.Broadcast();
	return Item;
}

// ──────────────────────────────────────────────────────────────
// UseItem
// Consumable 타입만 허용. 효과 적용 후 그리드에서 제거.
// ──────────────────────────────────────────────────────────────
bool ULRInventoryGridComponent::UseItem(int32 InItemID)
{
	FLRGridItem* Found = Items.Find(InItemID);
	if (!Found || !Found->ItemData)
		return false;

	if (Found->ItemData->ItemType != ELRItemType::Consumable)
		return false;

	// 소유 액터에서 StatusComponent를 찾아 효과 적용
	if (AActor* Owner = GetOwner())
	{
		if (ULRStatusComponent* Status = Owner->FindComponentByClass<ULRStatusComponent>())
		{
			if (Found->ItemData->HealthRestore > 0.f)
				Status->RestoreHealth(Found->ItemData->HealthRestore);

			if (Found->ItemData->StaminaRestore > 0.f)
				Status->RestoreStamina(Found->ItemData->StaminaRestore);
		}
	}

	// 수량 1 감소 — 0이 되면 그리드에서 제거
	Found->Quantity--;
	if (Found->Quantity <= 0)
		RemoveItem(InItemID);
	else
		OnGridChanged.Broadcast();

	return true;
}

// ──────────────────────────────────────────────────────────────
// CanStack — (X,Y) 셀에 같은 종류 아이템이 있고 MaxStackSize 미만이면 true
// ──────────────────────────────────────────────────────────────
bool ULRInventoryGridComponent::CanStack(int32 X, int32 Y, const FLRGridItem& InItem) const
{
	const int32 ExistingID = GetItemIDAt(X, Y);
	if (ExistingID == INDEX_NONE) return false;

	const FLRGridItem* Existing = Items.Find(ExistingID);
	return Existing
		&& Existing->ItemData == InItem.ItemData
		&& InItem.ItemData
		&& InItem.ItemData->MaxStackSize > 1
		&& Existing->Quantity + InItem.Quantity <= InItem.ItemData->MaxStackSize;
}

// ──────────────────────────────────────────────────────────────
// AddToStack — ItemID 아이템에 수량 추가. 실제 추가된 수량 반환.
// ──────────────────────────────────────────────────────────────
int32 ULRInventoryGridComponent::AddToStack(int32 InItemID, int32 Amount)
{
	FLRGridItem* Existing = Items.Find(InItemID);
	if (!Existing || !Existing->ItemData) return 0;

	const int32 CanAdd = Existing->ItemData->MaxStackSize - Existing->Quantity;
	const int32 Added  = FMath::Min(Amount, CanAdd);
	Existing->Quantity += Added;
	OnGridChanged.Broadcast();
	return Added;
}

// ──────────────────────────────────────────────────────────────
// FindEmptySpace — First-fit, 원본 → 회전 순서
// ──────────────────────────────────────────────────────────────
bool ULRInventoryGridComponent::FindEmptySpace(
	const FLRGridItem& Item,
	int32& OutX, int32& OutY, bool& bOutRotated) const
{
	for (int32 bRotate = 0; bRotate < 2; bRotate++)
	{
		const bool bTry = (bRotate == 1);
		const int32 W   = bTry ? Item.Height : Item.Width;
		const int32 H   = bTry ? Item.Width  : Item.Height;

		// 그리드보다 큰 크기는 스킵
		if (W > GridWidth || H > GridHeight) continue;

		for (int32 y = 0; y <= GridHeight - H; y++)
		{
			for (int32 x = 0; x <= GridWidth - W; x++)
			{
				if (CheckPlacement(x, y, Item, bTry))
				{
					OutX       = x;
					OutY       = y;
					bOutRotated = bTry;
					return true;
				}
			}
		}
	}
	return false;
}

int32 ULRInventoryGridComponent::ReduceQuantity(int32 InItemID, int32 Amount)
{
	FLRGridItem* Found = Items.Find(InItemID);
	if (!Found) return 0;

	Found->Quantity = FMath::Max(0, Found->Quantity - Amount);
	if (Found->Quantity <= 0)
	{
		RemoveItem(InItemID);
		return 0;
	}
	OnGridChanged.Broadcast();
	return Found->Quantity;
}

int32 ULRInventoryGridComponent::GetItemIDAt(int32 X, int32 Y) const
{
	if (X < 0 || Y < 0 || X >= GridWidth || Y >= GridHeight)
		return INDEX_NONE;
	return Grid[Y * GridWidth + X];
}

void ULRInventoryGridComponent::ClearGrid()
{
	Grid.Init(INDEX_NONE, GridWidth * GridHeight);
	Items.Empty();
	OnGridChanged.Broadcast();
}
