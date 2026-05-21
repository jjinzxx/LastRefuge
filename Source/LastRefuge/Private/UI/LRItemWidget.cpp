#include "UI/LRItemWidget.h"
#include "UI/LRItemDragDropOperation.h"
#include "Components/LRInventoryGridComponent.h"
#include "Items/LRItemDataAsset.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
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

	// 아이콘 텍스처 적용 — 브러시 크기를 슬롯 크기에 맞춰 고정
	if (ItemIcon && InItem.ItemData && InItem.ItemData->ItemIcon)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(InItem.ItemData->ItemIcon);
		Brush.TintColor = FSlateColor(FLinearColor::White);
		Brush.DrawAs    = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(
			InItem.GetEffectiveWidth()  * InSlotSize,
			InItem.GetEffectiveHeight() * InSlotSize);
		ItemIcon->SetBrush(Brush);
	}

	// 수량 텍스트 — Quantity > 1일 때만 표시
	if (QuantityText)
	{
		if (InItem.Quantity > 1)
		{
			QuantityText->SetText(FText::AsNumber(InItem.Quantity));
			QuantityText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			QuantityText->SetVisibility(ESlateVisibility::Collapsed);
		}
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
	UE_LOG(LogTemp, Log, TEXT("[Item] MouseButtonDown — %s"), *GetName());

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
	UE_LOG(LogTemp, Log, TEXT("[Item] DragDetected 시작 — %s"), *GetName());

	ULRItemDragDropOperation* Op = NewObject<ULRItemDragDropOperation>(this);
	Op->DraggedItem  = GridItem;
	Op->SourceGrid   = SourceGridComponent;
	Op->SourceItemID = ItemID;
	Op->Pivot        = EDragPivot::TopLeft;

	// ULRItemWidget은 루트 캔버스 DesiredSize=0이라 드래그 비주얼이 안 보임.
	// UImage에 FSlateBrush.ImageSize를 명시하면 SImage의 DesiredSize가 아이템 크기가 됨.
	UImage* DragImage = NewObject<UImage>(GetOwningPlayer());
	{
		FSlateBrush Brush;
		if (GridItem.ItemData && GridItem.ItemData->ItemIcon)
			Brush.SetResourceObject(GridItem.ItemData->ItemIcon);
		Brush.ImageSize = FVector2D(
			GridItem.GetEffectiveWidth()  * SlotSize,
			GridItem.GetEffectiveHeight() * SlotSize);
		DragImage->SetBrush(Brush);
	}
	Op->DefaultDragVisual = DragImage;

	// 소스 그리드에서 제거 (드롭 취소 시 NativeOnDragCancelled에서 복원)
	if (SourceGridComponent)
		SourceGridComponent->RemoveItem(ItemID);

	OutOperation = Op;
}

// ──────────────────────────────────────────────────────────
// NativeOnDragCancelled — 아이템 그리드 밖에서 드롭 취소 시 원위치 복원
// ──────────────────────────────────────────────────────────
void ULRItemWidget::NativeOnDragCancelled(
	const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ULRItemDragDropOperation* Op = Cast<ULRItemDragDropOperation>(InOperation);
	if (Op && Op->SourceGrid)
	{
		Op->SourceGrid->PlaceItem(
			Op->DraggedItem.GridX, Op->DraggedItem.GridY,
			Op->DraggedItem, Op->DraggedItem.bIsRotated);
	}
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
