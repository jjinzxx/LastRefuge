#include "UI/LRInventoryGridWidget.h"
#include "UI/LRItemWidget.h"
#include "UI/LRItemDragDropOperation.h"
#include "Components/LRInventoryGridComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Rendering/DrawElements.h"
#include "Components/Image.h"
#include "UI/LRTooltipWidget.h"

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

	GridChangedHandle = GridComponent->OnGridChanged.AddUObject(
		this, &ULRInventoryGridWidget::RebuildGrid);

	// 툴팁 위젯 생성 (뷰포트 최상단에 숨김 상태로 추가)
	if (TooltipWidgetClass && !ActiveTooltip)
	{
		ActiveTooltip = CreateWidget<ULRTooltipWidget>(GetOwningPlayer(), TooltipWidgetClass);
		if (ActiveTooltip)
		{
			ActiveTooltip->AddToViewport(50);
			ActiveTooltip->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	RebuildGrid();
}

void ULRInventoryGridWidget::NativeDestruct()
{
	if (GridComponent)
		GridComponent->OnGridChanged.Remove(GridChangedHandle);

	if (ActiveTooltip)
		ActiveTooltip->RemoveFromParent();

	Super::NativeDestruct();
}

// ──────────────────────────────────────────────────────────
// NativePaint — 그리드 선 + 호버 하이라이트
// ──────────────────────────────────────────────────────────
int32 ULRInventoryGridWidget::NativePaint(
	const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 Result = Super::NativePaint(
		Args, AllottedGeometry, MyCullingRect, OutDrawElements,
		LayerId, InWidgetStyle, bParentEnabled);

	if (!GridComponent) return Result;

	const int32 Cols = GridComponent->GridWidth;
	const int32 Rows = GridComponent->GridHeight;

	// GridCanvas가 Fill로 부모를 꽉 채우므로 오프셋은 항상 (0,0)
	const FVector2D Origin = FVector2D::ZeroVector;

	// ── 그리드 선 ────────────────────────────────────────
	for (int32 Col = 0; Col <= Cols; ++Col)
	{
		const float X = Origin.X + Col * SlotSize;
		TArray<FVector2D> Pts = { FVector2D(X, Origin.Y), FVector2D(X, Origin.Y + Rows * SlotSize) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(), Pts,
			ESlateDrawEffect::None, GridLineColor, true, 1.f);
	}
	for (int32 Row = 0; Row <= Rows; ++Row)
	{
		const float Y = Origin.Y + Row * SlotSize;
		TArray<FVector2D> Pts = { FVector2D(Origin.X, Y), FVector2D(Origin.X + Cols * SlotSize, Y) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(), Pts,
			ESlateDrawEffect::None, GridLineColor, true, 1.f);
	}

	// ── 호버 셀 하이라이트 ───────────────────────────────
	if (HoveredCell.X >= 0)
	{
		const FVector2D TL(
			Origin.X + HoveredCell.X * SlotSize,
			Origin.Y + HoveredCell.Y * SlotSize);

		TArray<FVector2D> Box = {
			TL,
			FVector2D(TL.X + SlotSize, TL.Y),
			FVector2D(TL.X + SlotSize, TL.Y + SlotSize),
			FVector2D(TL.X,            TL.Y + SlotSize),
			TL
		};
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2,
			AllottedGeometry.ToPaintGeometry(), Box,
			ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.35f), true, 2.f);
	}

	// ── 드래그 프리뷰 ────────────────────────────────────
	if (bPreviewVisible && PreviewGridPos.X >= 0)
	{
		const FLinearColor PreviewColor = bPreviewCanPlace
			? FLinearColor(0.f, 1.f, 0.f, 0.35f)
			: FLinearColor(1.f, 0.f, 0.f, 0.35f);

		const FVector2D TL(Origin.X + PreviewGridPos.X * SlotSize,
		                   Origin.Y + PreviewGridPos.Y * SlotSize);
		const FVector2D Sz(PreviewItemW * SlotSize, PreviewItemH * SlotSize);

		FSlateColorBrush SolidBrush(FLinearColor::White);
		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId + 3,
			AllottedGeometry.ToPaintGeometry(TL, Sz),
			&SolidBrush,
			ESlateDrawEffect::None,
			PreviewColor);
	}

	return Result;
}

// ──────────────────────────────────────────────────────────
// NativeOnMouseButtonDown — 클릭한 셀의 아이템 확인 후 드래그 준비
// 아이템 위젯은 HitTestInvisible이므로 그리드 위젯이 직접 처리
// ──────────────────────────────────────────────────────────
FReply ULRInventoryGridWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!GridComponent) return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	FVector2D LocalPx = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	// GridCanvas가 Fill이므로 위젯 로컬 좌표 = 그리드 좌표

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FIntPoint Cell = GetGridIndexFromMouse(LocalPx);
		const int32 HitItemID = GridComponent->GetItemIDAt(Cell.X, Cell.Y);

		if (HitItemID != INDEX_NONE)
		{
			if (InMouseEvent.IsShiftDown())
			{
				// Shift+클릭: 빠른 전송
				if (StorageGrid)
				{
					const FLRGridItem& Item = GridComponent->GetItems()[HitItemID];
					int32 OutX, OutY; bool bRot;
					if (StorageGrid->FindEmptySpace(Item, OutX, OutY, bRot))
					{
						GridComponent->RemoveItem(HitItemID);
						StorageGrid->PlaceItem(OutX, OutY, Item, bRot);
					}
				}
				return FReply::Handled();
			}

			// 드래그 감지 준비 — 아이템 내 클릭 위치(슬롯 단위) 기록
			const FLRGridItem& Item = GridComponent->GetItems()[HitItemID];
			const FVector2D ItemOriginPx = GridToLocal(Item.GridX, Item.GridY);
			GrabOffsetSlots = (LocalPx - ItemOriginPx) / SlotSize;
			PendingDragItemID = HitItemID;

			UE_LOG(LogTemp, Warning, TEXT("[Grab] ItemOrigin=(%d,%d) ItemSize=(%d×%d) ClickPx=(%.0f,%.0f) GrabOffset=(%.2f,%.2f)"),
				Item.GridX, Item.GridY, Item.GetEffectiveWidth(), Item.GetEffectiveHeight(),
				LocalPx.X, LocalPx.Y, GrabOffsetSlots.X, GrabOffsetSlots.Y);

			return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
		}
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		const FIntPoint Cell = GetGridIndexFromMouse(LocalPx);
		const int32 HitItemID = GridComponent->GetItemIDAt(Cell.X, Cell.Y);
		if (HitItemID != INDEX_NONE)
			GridComponent->UseItem(HitItemID);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULRInventoryGridWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	PendingDragItemID = INDEX_NONE;
	return FReply::Handled();
}

// ──────────────────────────────────────────────────────────
// NativeOnDragDetected — DragDropOperation 생성
// ──────────────────────────────────────────────────────────
void ULRInventoryGridWidget::NativeOnDragDetected(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	if (PendingDragItemID == INDEX_NONE || !GridComponent) return;

	const TMap<int32, FLRGridItem>& Items = GridComponent->GetItems();
	if (!Items.Contains(PendingDragItemID)) return;

	const FLRGridItem DraggedItemData = Items[PendingDragItemID];

	ULRItemDragDropOperation* Op = NewObject<ULRItemDragDropOperation>(this);
	Op->DraggedItem     = DraggedItemData;
	Op->SourceGrid      = GridComponent;
	Op->SourceItemID    = PendingDragItemID;
	Op->GrabOffsetSlots = GrabOffsetSlots;
	Op->Pivot           = EDragPivot::MouseDown;

	// UImage + FSlateBrush.ImageSize → 아이템 실제 픽셀 크기로 드래그 비주얼 생성
	UImage* DragImage = NewObject<UImage>(GetOwningPlayer());
	{
		FSlateBrush Brush;
		if (DraggedItemData.ItemData && DraggedItemData.ItemData->ItemIcon)
			Brush.SetResourceObject(DraggedItemData.ItemData->ItemIcon);
		Brush.ImageSize = FVector2D(
			DraggedItemData.GetEffectiveWidth()  * SlotSize,
			DraggedItemData.GetEffectiveHeight() * SlotSize);
		DragImage->SetBrush(Brush);
	}
	Op->DefaultDragVisual = DragImage;

	GridComponent->RemoveItem(PendingDragItemID);
	PendingDragItemID = INDEX_NONE;
	HidePreview();

	if (ActiveTooltip)
		ActiveTooltip->SetVisibility(ESlateVisibility::Hidden);

	UE_LOG(LogTemp, Warning, TEXT("[Grid] DragDetected — %s"),
		DraggedItemData.ItemData ? *DraggedItemData.ItemData->GetName() : TEXT("?"));

	OutOperation = Op;
}

// ──────────────────────────────────────────────────────────
// NativeOnMouseMove — 호버 셀 갱신
// ──────────────────────────────────────────────────────────
FReply ULRInventoryGridWidget::NativeOnMouseMove(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (GridComponent)
	{
		FVector2D LocalPx = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		// GridCanvas가 Fill이므로 위젯 로컬 좌표 = 그리드 좌표

		const FIntPoint Cell(
			FMath::FloorToInt(LocalPx.X / SlotSize),
			FMath::FloorToInt(LocalPx.Y / SlotSize));

		const bool bInBounds =
			Cell.X >= 0 && Cell.X < GridComponent->GridWidth &&
			Cell.Y >= 0 && Cell.Y < GridComponent->GridHeight;

		const FIntPoint Next = bInBounds ? Cell : FIntPoint(-1, -1);
		if (Next != HoveredCell)
		{
			HoveredCell = Next;
			Invalidate(EInvalidateWidgetReason::Paint);

			// 툴팁 갱신
			if (ActiveTooltip)
			{
				if (HoveredCell.X >= 0)
				{
					const int32 HovItemID = GridComponent->GetItemIDAt(HoveredCell.X, HoveredCell.Y);
					if (HovItemID != INDEX_NONE)
					{
						ActiveTooltip->SetItem(GridComponent->GetItems()[HovItemID]);
						ActiveTooltip->SetVisibility(ESlateVisibility::HitTestInvisible);
					}
					else
					{
						ActiveTooltip->SetVisibility(ESlateVisibility::Hidden);
					}
				}
				else
				{
					ActiveTooltip->SetVisibility(ESlateVisibility::Hidden);
				}
			}
		}

		// 툴팁 위치를 마우스 커서 옆으로 매 프레임 갱신
		if (ActiveTooltip && ActiveTooltip->GetVisibility() != ESlateVisibility::Hidden)
		{
			FVector2D MouseViewportPos;
			if (APlayerController* PC = GetOwningPlayer())
				PC->GetMousePosition(MouseViewportPos.X, MouseViewportPos.Y);
			ActiveTooltip->SetPositionInViewport(MouseViewportPos + FVector2D(16.f, 16.f), false);
		}
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void ULRInventoryGridWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	if (HoveredCell.X >= 0)
	{
		HoveredCell = FIntPoint(-1, -1);
		Invalidate(EInvalidateWidgetReason::Paint);
	}
	if (ActiveTooltip)
		ActiveTooltip->SetVisibility(ESlateVisibility::Hidden);

	Super::NativeOnMouseLeave(InMouseEvent);
}

// ──────────────────────────────────────────────────────────
// RebuildGrid
// ──────────────────────────────────────────────────────────
void ULRInventoryGridWidget::RebuildGrid()
{
	if (!GridCanvas)      { UE_LOG(LogTemp, Error, TEXT("[Grid] GridCanvas null")); return; }
	if (!GridComponent)   { UE_LOG(LogTemp, Error, TEXT("[Grid] GridComponent null")); return; }
	if (!ItemWidgetClass) { UE_LOG(LogTemp, Error, TEXT("[Grid] ItemWidgetClass null")); return; }

	UE_LOG(LogTemp, Warning, TEXT("[Grid] RebuildGrid — 아이템 수: %d"), GridComponent->GetItems().Num());

	TArray<UWidget*> Children = GridCanvas->GetAllChildren();
	for (UWidget* Child : Children)
	{
		if (Child)
			Child->RemoveFromParent();
	}

	for (auto& [ID, Item] : GridComponent->GetItems())
	{
		ULRItemWidget* ItemWidget = CreateWidget<ULRItemWidget>(this, ItemWidgetClass);
		if (!ItemWidget) continue;

		ItemWidget->Init(Item, ID, GridComponent, StorageGrid, SlotSize);

		// 입력은 그리드 위젯이 처리하므로 아이템 위젯은 렌더링만 담당
		ItemWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

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

	FVector2D LocalPx = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
	// GridCanvas가 Fill이므로 위젯 로컬 좌표 = 그리드 좌표
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

	FVector2D LocalPx = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
	// GridCanvas가 Fill이므로 위젯 로컬 좌표 = 그리드 좌표
	LocalPx -= Op->GrabOffsetSlots * SlotSize;

	const FIntPoint GridPos = GetGridIndexFromMouse(LocalPx);
	const bool bRotated = Op->DraggedItem.bIsRotated;
	const bool bCanPlace = GridComponent->CheckPlacement(GridPos.X, GridPos.Y, Op->DraggedItem, bRotated);

	UE_LOG(LogTemp, Warning, TEXT("[Grid] NativeOnDrop — GridPos: (%d,%d) CanPlace: %s"),
		GridPos.X, GridPos.Y, bCanPlace ? TEXT("YES") : TEXT("NO"));

	if (!bCanPlace)
	{
		if (Op->SourceGrid)
			Op->SourceGrid->PlaceItem(
				Op->DraggedItem.GridX, Op->DraggedItem.GridY,
				Op->DraggedItem, bRotated);
		return false;
	}

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
// 프리뷰 표시/숨김 (NativePaint에서 직접 렌더링)
// ──────────────────────────────────────────────────────────
void ULRInventoryGridWidget::ShowPreview(
	int32 GridX, int32 GridY, const FLRGridItem& Item, bool bCanPlace)
{
	PreviewGridPos   = FIntPoint(GridX, GridY);
	PreviewItemW     = Item.GetEffectiveWidth();
	PreviewItemH     = Item.GetEffectiveHeight();
	bPreviewVisible  = true;
	bPreviewCanPlace = bCanPlace;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void ULRInventoryGridWidget::HidePreview()
{
	if (bPreviewVisible)
	{
		bPreviewVisible = false;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}
