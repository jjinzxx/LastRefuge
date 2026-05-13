#include "UI/LRItemWidget.h"
#include "UI/LRItemDragDropOperation.h"
#include "Components/LRInventoryGridComponent.h"
#include "Items/LRItemDataAsset.h"
#include "Components/Image.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void ULRItemWidget::Init(const FLRGridItem& InItem, int32 InItemID,
                          ULRInventoryGridComponent* InSourceGrid,
                          ULRInventoryGridComponent* InTargetStorageGrid,
                          float InSlotSize)
{
	GridItem           = InItem;
	ItemID             = InItemID;
	SourceGridComponent = InSourceGrid;
	TargetStorageGrid  = InTargetStorageGrid;
	SlotSize           = InSlotSize;

	// 아이콘 텍스처 적용
	if (ItemIcon && InItem.ItemData && InItem.ItemData->ItemIcon)
	{
		ItemIcon->SetBrushFromTexture(InItem.ItemData->ItemIcon);
	}
}

// ──────────────────────────────────────────────────────────
// NativeOnMouseButtonDown
// 좌클릭: 드래그 감지 시작 + GrabOffset 기록
// Shift+좌클릭: 빠른 전송
// 우클릭: 아이템 사용 (Consumable)
// ──────────────────────────────────────────────────────────
FReply ULRItemWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (InMouseEvent.IsShiftDown())
		{
			QuickTransferToTarget();
			return FReply::Handled();
		}

		// 위젯 내 클릭 위치를 슬롯 단위로 변환하여 기록
		const FVector2D LocalPx = InGeometry.AbsoluteToLocal(
			InMouseEvent.GetScreenSpacePosition());
		GrabOffsetSlots = LocalPx / SlotSize;

		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (SourceGridComponent)
			SourceGridComponent->UseItem(ItemID);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULRItemWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return FReply::Handled();
}

// ──────────────────────────────────────────────────────────
// NativeOnDragDetected
// DragDropOperation 생성. DefaultDragVisual = 이 위젯 자체.
// Pivot = MouseDown → 클릭 지점이 마우스를 따라다님.
// ──────────────────────────────────────────────────────────
void ULRItemWidget::NativeOnDragDetected(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	ULRItemDragDropOperation* Op = NewObject<ULRItemDragDropOperation>(this);
	Op->DraggedItem      = GridItem;
	Op->SourceGrid       = SourceGridComponent;
	Op->SourceItemID     = ItemID;
	Op->GrabOffsetSlots  = GrabOffsetSlots;
	Op->DefaultDragVisual = this;
	Op->Pivot            = EDragPivot::MouseDown;

	// 드래그 시작 시 소스에서 제거 (드롭 취소 시 NativeOnDragCancelled에서 복원)
	if (SourceGridComponent)
		SourceGridComponent->RemoveItem(ItemID);

	OutOperation = Op;
}

// ──────────────────────────────────────────────────────────
// QuickTransferToTarget — Shift+클릭 빠른 전송
// ──────────────────────────────────────────────────────────
void ULRItemWidget::QuickTransferToTarget()
{
	if (!SourceGridComponent || !TargetStorageGrid) return;

	int32 OutX, OutY;
	bool bOutRotated;

	if (!TargetStorageGrid->FindEmptySpace(GridItem, OutX, OutY, bOutRotated))
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemWidget] 창고 공간 부족 — 전송 취소"));
		return;
	}

	SourceGridComponent->RemoveItem(ItemID);
	TargetStorageGrid->PlaceItem(OutX, OutY, GridItem, bOutRotated);
}
