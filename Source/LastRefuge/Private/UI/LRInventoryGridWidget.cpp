#include "UI/LRInventoryGridWidget.h"
#include "UI/LRItemWidget.h"
#include "UI/LRDragPreviewWidget.h"
#include "UI/LRItemDragDropOperation.h"
#include "Components/LRInventoryGridComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"

// ──────────────────────────────────────────────────────────
// InitGrid
// ──────────────────────────────────────────────────────────
void ULRInventoryGridWidget::InitGrid(
	ULRInventoryGridComponent* InGridComponent,
	ULRInventoryGridComponent* InStorageGrid,
	float InSlotSize)
{
	GridComponent = InGridComponent;
	StorageGrid   = InStorageGrid;
	SlotSize      = InSlotSize;

	if (!GridComponent) return;

	// 그리드 변경 시 자동 리빌드 구독
	GridChangedHandle = GridComponent->OnGridChanged.AddUObject(
		this, &ULRInventoryGridWidget::RebuildGrid);

	RebuildGrid();
}

void ULRInventoryGridWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 프리뷰 위젯 미리 생성 (숨김 상태)
	if (PreviewWidgetClass && !PreviewWidget)
	{
		PreviewWidget = CreateWidget<ULRDragPreviewWidget>(this, PreviewWidgetClass);
		if (PreviewWidget && GridCanvas)
		{
			UCanvasPanelSlot* PreviewCanvasSlot = GridCanvas->AddChildToCanvas(PreviewWidget);
			if (PreviewCanvasSlot)
			{
				PreviewCanvasSlot->SetSize(FVector2D(SlotSize, SlotSize));
				PreviewCanvasSlot->SetZOrder(10);
			}
			PreviewWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ULRInventoryGridWidget::NativeDestruct()
{
	if (GridComponent)
		GridComponent->OnGridChanged.Remove(GridChangedHandle);

	Super::NativeDestruct();
}

// ──────────────────────────────────────────────────────────
// RebuildGrid — 캔버스를 완전히 다시 그림
// OnGridChanged 때마다 호출. 아이템 수가 적으므로 비용 허용 범위.
// ──────────────────────────────────────────────────────────
void ULRInventoryGridWidget::RebuildGrid()
{
	if (!GridCanvas || !GridComponent || !ItemWidgetClass) return;

	// 기존 아이템 위젯 모두 제거 (프리뷰는 유지)
	TArray<UWidget*> Children = GridCanvas->GetAllChildren();
	for (UWidget* Child : Children)
	{
		if (Child && Child != PreviewWidget)
			Child->RemoveFromParent();
	}

	// 아이템마다 위젯 생성 및 배치
	for (auto& [ID, Item] : GridComponent->GetItems())
	{
		ULRItemWidget* ItemWidget = CreateWidget<ULRItemWidget>(this, ItemWidgetClass);
		if (!ItemWidget) continue;

		ItemWidget->Init(Item, ID, GridComponent, StorageGrid, SlotSize);

		UCanvasPanelSlot* CanvasSlot = GridCanvas->AddChildToCanvas(ItemWidget);
		if (CanvasSlot)
		{
			CanvasSlot->SetPosition(GridToLocal(Item.GridX, Item.GridY));
			CanvasSlot->SetSize(FVector2D(
				Item.GetEffectiveWidth()  * SlotSize,
				Item.GetEffectiveHeight() * SlotSize));
			CanvasSlot->SetZOrder(1);
		}
	}
}

// ──────────────────────────────────────────────────────────
// NativeOnDragOver — 프리뷰 갱신
// ──────────────────────────────────────────────────────────
bool ULRInventoryGridWidget::NativeOnDragOver(
	const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	ULRItemDragDropOperation* Op = Cast<ULRItemDragDropOperation>(InOperation);
	if (!Op || !GridComponent) return false;

	// 마우스 위치 → 위젯 로컬 픽셀 좌표
	FVector2D LocalPx = InGeometry.AbsoluteToLocal(
		InDragDropEvent.GetScreenSpacePosition());

	// GrabOffset을 빼서 아이템 좌상단 기준으로 보정
	LocalPx -= Op->GrabOffsetSlots * SlotSize;

	const FIntPoint GridPos = GetGridIndexFromMouse(LocalPx);
	const bool bCanPlace = GridComponent->CheckPlacement(
		GridPos.X, GridPos.Y, Op->DraggedItem, Op->DraggedItem.bIsRotated);

	ShowPreview(GridPos.X, GridPos.Y, Op->DraggedItem, bCanPlace);
	return true;
}

// ──────────────────────────────────────────────────────────
// NativeOnDrop — 배치 처리
// ──────────────────────────────────────────────────────────
bool ULRInventoryGridWidget::NativeOnDrop(
	const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	HidePreview();

	ULRItemDragDropOperation* Op = Cast<ULRItemDragDropOperation>(InOperation);
	if (!Op || !GridComponent) return false;

	FVector2D LocalPx = InGeometry.AbsoluteToLocal(
		InDragDropEvent.GetScreenSpacePosition());
	LocalPx -= Op->GrabOffsetSlots * SlotSize;

	const FIntPoint GridPos = GetGridIndexFromMouse(LocalPx);
	const bool bRotated     = Op->DraggedItem.bIsRotated;

	if (!GridComponent->CheckPlacement(GridPos.X, GridPos.Y, Op->DraggedItem, bRotated))
	{
		// 배치 불가 → 원위치 복원 (소스 그리드가 다를 수도 있으므로 소스에 복원)
		if (Op->SourceGrid)
		{
			Op->SourceGrid->PlaceItem(
				Op->DraggedItem.GridX, Op->DraggedItem.GridY,
				Op->DraggedItem, bRotated);
		}
		return false;
	}

	// 타겟 그리드에 배치 (소스에서는 이미 DragDetected 시점에 제거됨)
	GridComponent->PlaceItem(GridPos.X, GridPos.Y, Op->DraggedItem, bRotated);
	return true;
}

void ULRInventoryGridWidget::NativeOnDragLeave(
	const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	HidePreview();
}

void ULRInventoryGridWidget::NativeOnDragCancelled(
	const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	HidePreview();

	// 드롭 취소 시 소스 위치에 원복
	ULRItemDragDropOperation* Op = Cast<ULRItemDragDropOperation>(InOperation);
	if (Op && Op->SourceGrid)
	{
		Op->SourceGrid->PlaceItem(
			Op->DraggedItem.GridX, Op->DraggedItem.GridY,
			Op->DraggedItem, Op->DraggedItem.bIsRotated);
	}
}

// ──────────────────────────────────────────────────────────
// 좌표 변환
// ──────────────────────────────────────────────────────────
FIntPoint ULRInventoryGridWidget::GetGridIndexFromMouse(FVector2D LocalPx) const
{
	return FIntPoint(
		FMath::FloorToInt(LocalPx.X / SlotSize),
		FMath::FloorToInt(LocalPx.Y / SlotSize));
}

FVector2D ULRInventoryGridWidget::GridToLocal(int32 GridX, int32 GridY) const
{
	return FVector2D(GridX * SlotSize, GridY * SlotSize);
}

// ──────────────────────────────────────────────────────────
// 프리뷰 표시/숨김
// ──────────────────────────────────────────────────────────
void ULRInventoryGridWidget::ShowPreview(
	int32 GridX, int32 GridY, const FLRGridItem& Item, bool bCanPlace)
{
	if (!PreviewWidget || !GridCanvas) return;

	UCanvasPanelSlot* PreviewCanvasSlot = Cast<UCanvasPanelSlot>(PreviewWidget->Slot);
	if (PreviewCanvasSlot)
	{
		// 그리드 좌표에 스냅된 픽셀 위치로 이동
		PreviewCanvasSlot->SetPosition(GridToLocal(GridX, GridY));
		PreviewCanvasSlot->SetSize(FVector2D(
			Item.GetEffectiveWidth()  * SlotSize,
			Item.GetEffectiveHeight() * SlotSize));
	}

	PreviewWidget->SetHighlight(bCanPlace);
	PreviewWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULRInventoryGridWidget::HidePreview()
{
	if (PreviewWidget)
		PreviewWidget->SetVisibility(ESlateVisibility::Hidden);
}
