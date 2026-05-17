#include "UI/LRInventoryGridWidget.h"
#include "UI/LRItemWidget.h"
#include "UI/LRItemDragDropOperation.h"
#include "UI/LRContextMenuWidget.h"
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

	// GridCanvas를 그리드 실제 크기로 설정하고 화면 중앙에 배치
	if (GridCanvas)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(GridCanvas->Slot))
		{
			const FVector2D GridPxSize(InGridComponent->GridWidth * InSlotSize,
			                           InGridComponent->GridHeight * InSlotSize);
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetPosition(FVector2D::ZeroVector);
			CanvasSlot->SetSize(GridPxSize);
			CanvasSlot->SetAutoSize(false);
		}
	}

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
	{
		ActiveTooltip->RemoveFromParent();
		ActiveTooltip = nullptr;
	}

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

	// GridCanvas의 실제 화면 위치를 기준으로 그리드 선을 그림
	FVector2D Origin = FVector2D::ZeroVector;
	if (GridCanvas)
		Origin = AllottedGeometry.AbsoluteToLocal(GridCanvas->GetCachedGeometry().GetAbsolutePosition());

	// ── 그리드 선 ────────────────────────────────────────
	for (int32 Col = 0; Col <= Cols; ++Col)
	{
		const float X = Origin.X + Col * SlotSize;
		TArray<FVector2D> Pts = { FVector2D(X, Origin.Y), FVector2D(X, Origin.Y + Rows * SlotSize) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(), Pts,
			ESlateDrawEffect::None, GridLineColor, true, GridLineThickness);
	}
	for (int32 Row = 0; Row <= Rows; ++Row)
	{
		const float Y = Origin.Y + Row * SlotSize;
		TArray<FVector2D> Pts = { FVector2D(Origin.X, Y), FVector2D(Origin.X + Cols * SlotSize, Y) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(), Pts,
			ESlateDrawEffect::None, GridLineColor, true, GridLineThickness);
	}

	// ── 호버 셀 — 외곽선만, 모서리 강조 ──────────────────
	if (HoveredCell.X >= 0)
	{
		const FVector2D TL(
			Origin.X + HoveredCell.X * SlotSize,
			Origin.Y + HoveredCell.Y * SlotSize);
		const FVector2D BR(TL.X + SlotSize, TL.Y + SlotSize);

		// 외곽 사각형 라인
		TArray<FVector2D> Box = { TL, FVector2D(BR.X, TL.Y), BR, FVector2D(TL.X, BR.Y), TL };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2,
			AllottedGeometry.ToPaintGeometry(), Box,
			ESlateDrawEffect::None, HoverLineColor, true, HoverLineThickness);

		// 네 모서리 강조 (라인아트 포인트)
		const float C = SlotSize * 0.22f;
		TArray<TArray<FVector2D>> Corners = {
			{ TL,                              FVector2D(TL.X + C, TL.Y), FVector2D(TL.X, TL.Y + C) },
			{ FVector2D(BR.X, TL.Y),          FVector2D(BR.X - C, TL.Y), FVector2D(BR.X, TL.Y + C) },
			{ BR,                              FVector2D(BR.X - C, BR.Y), FVector2D(BR.X, BR.Y - C) },
			{ FVector2D(TL.X, BR.Y),          FVector2D(TL.X + C, BR.Y), FVector2D(TL.X, BR.Y - C) },
		};
		for (auto& CornerPts : Corners)
		{
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3,
				AllottedGeometry.ToPaintGeometry(), CornerPts,
				ESlateDrawEffect::None, HoverLineColor, true, HoverLineThickness + 1.0f);
		}
	}

	// ── 드래그 프리뷰 — 외곽선 only (라인아트) ───────────
	if (bPreviewVisible && PreviewGridPos.X >= 0 && PreviewGridPos.Y >= 0)
	{
		const FLinearColor PreviewColor = bPreviewCanPlace ? PreviewCanPlaceColor : PreviewBlockedColor;

		const FVector2D TL(Origin.X + PreviewGridPos.X * SlotSize,
		                   Origin.Y + PreviewGridPos.Y * SlotSize);
		const FVector2D BR(TL.X + PreviewItemW * SlotSize, TL.Y + PreviewItemH * SlotSize);

		// 외곽 사각형 라인
		TArray<FVector2D> PreviewBox = { TL, FVector2D(BR.X, TL.Y), BR, FVector2D(TL.X, BR.Y), TL };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3,
			AllottedGeometry.ToPaintGeometry(), PreviewBox,
			ESlateDrawEffect::None, PreviewColor, true, PreviewLineThickness);

		// 내부 대각선 크로스 (배치 가능 여부를 직관적으로 표시)
		TArray<FVector2D> DiagA = { TL, BR };
		TArray<FVector2D> DiagB = { FVector2D(BR.X, TL.Y), FVector2D(TL.X, BR.Y) };
		const FLinearColor DiagColor = FLinearColor(PreviewColor.R, PreviewColor.G, PreviewColor.B, 0.2f);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3,
			AllottedGeometry.ToPaintGeometry(), DiagA,
			ESlateDrawEffect::None, DiagColor, true, 1.0f);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3,
			AllottedGeometry.ToPaintGeometry(), DiagB,
			ESlateDrawEffect::None, DiagColor, true, 1.0f);
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
		CloseContextMenu();

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

			// 드래그 감지 준비
			PendingDragItemID = HitItemID;
			return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
		}
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		CloseContextMenu();

		const FIntPoint Cell = GetGridIndexFromMouse(LocalPx);
		const int32 HitItemID = GridComponent->GetItemIDAt(Cell.X, Cell.Y);
		if (HitItemID != INDEX_NONE)
		{
			// 우클릭 드래그 감지 대기 + 마우스업 시 컨텍스트 메뉴 표시
			PendingDragItemID  = HitItemID;
			bRightClickPending = true;
			return FReply::Handled().DetectDrag(TakeWidget(), EKeys::RightMouseButton);
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply ULRInventoryGridWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && bRightClickPending)
	{
		// 드래그 없이 우클릭 → 컨텍스트 메뉴 표시
		bRightClickPending = false;
		if (PendingDragItemID != INDEX_NONE && GridComponent->GetItems().Contains(PendingDragItemID))
		{
			const FLRGridItem& Item = GridComponent->GetItems()[PendingDragItemID];
			ShowContextMenu(PendingDragItemID, Item);
		}
		PendingDragItemID = INDEX_NONE;
		return FReply::Handled();
	}

	PendingDragItemID  = INDEX_NONE;
	bRightClickPending = false;
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

	FLRGridItem DraggedItemData = Items[PendingDragItemID];
	const bool bRightDrag = bRightClickPending
		|| InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton;
	bRightClickPending = false;

	// 우클릭 드래그: 스택에서 1개만 분리
	if (bRightDrag && DraggedItemData.Quantity > 1)
	{
		GridComponent->ReduceQuantity(PendingDragItemID, 1);
		DraggedItemData.Quantity = 1;
		// SourceItemID = INDEX_NONE → 드롭 취소 시 원위치 복원 없음 (이미 분리됨)
	}
	else
	{
		// 좌클릭 or 수량 1짜리 우클릭 드래그 → 전체 이동
		GridComponent->RemoveItem(PendingDragItemID);
	}

	ULRItemDragDropOperation* Op = NewObject<ULRItemDragDropOperation>(this);
	Op->DraggedItem  = DraggedItemData;
	Op->SourceGrid   = bRightDrag && DraggedItemData.Quantity == 1 ? nullptr : GridComponent;
	Op->SourceItemID = PendingDragItemID;
	const FVector2D ItemPxSize(
		DraggedItemData.GetEffectiveWidth()  * SlotSize,
		DraggedItemData.GetEffectiveHeight() * SlotSize);

	UImage* DragImage = NewObject<UImage>(GetOwningPlayer());
	{
		FSlateBrush Brush;
		if (DraggedItemData.ItemData && DraggedItemData.ItemData->ItemIcon)
			Brush.SetResourceObject(DraggedItemData.ItemData->ItemIcon);
		Brush.ImageSize = ItemPxSize;
		DragImage->SetBrush(Brush);
	}
	Op->DefaultDragVisual = DragImage;
	Op->Pivot             = EDragPivot::TopLeft;

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
			const float DPI = UWidgetLayoutLibrary::GetViewportScale(GetWorld());
			ActiveTooltip->SetPositionInViewport((MouseViewportPos + FVector2D(16.f, 16.f)) / DPI, false);
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

	const FVector2D LocalPx = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
	const FIntPoint GridPos = GetGridIndexFromMouse(LocalPx);
	const bool bCanPlace = GridComponent->CanStack(GridPos.X, GridPos.Y, Op->DraggedItem)
		|| GridComponent->CheckPlacement(GridPos.X, GridPos.Y, Op->DraggedItem, Op->DraggedItem.bIsRotated);

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

	const FVector2D LocalPx = InGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
	const FIntPoint GridPos = GetGridIndexFromMouse(LocalPx);
	const bool bRotated = Op->DraggedItem.bIsRotated;

	// 스태킹 우선 시도
	const int32 ExistingID = GridComponent->GetItemIDAt(GridPos.X, GridPos.Y);
	if (GridComponent->CanStack(GridPos.X, GridPos.Y, Op->DraggedItem))
	{
		GridComponent->AddToStack(ExistingID, Op->DraggedItem.Quantity);
		return true;
	}

	// 일반 배치
	const bool bCanPlace = GridComponent->CheckPlacement(GridPos.X, GridPos.Y, Op->DraggedItem, bRotated);
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

// ──────────────────────────────────────────────────────────
// 컨텍스트 메뉴
// ──────────────────────────────────────────────────────────
void ULRInventoryGridWidget::ShowContextMenu(int32 ItemID, const FLRGridItem& Item)
{
	CloseContextMenu();

	if (!ContextMenuWidgetClass) return;

	ActiveContextMenu = CreateWidget<ULRContextMenuWidget>(GetOwningPlayer(), ContextMenuWidgetClass);
	if (!ActiveContextMenu) return;

	ActiveContextMenu->InitMenu(Item, ItemID, GridComponent);
	ActiveContextMenu->OnMenuClosed.AddLambda([this]() { ActiveContextMenu = nullptr; });
	// Z=200: 그리드(5)·툴팁(50) 위. 위젯이 전체화면을 채워 BackdropButton이 동작함.
	ActiveContextMenu->AddToViewport(200);

	// MenuBox를 마우스 커서 옆으로 배치 (DPI 스케일 적용)
	FVector2D MousePos;
	if (APlayerController* PC = GetOwningPlayer())
		PC->GetMousePosition(MousePos.X, MousePos.Y);
	const float DPI = UWidgetLayoutLibrary::GetViewportScale(GetWorld());
	ActiveContextMenu->PositionNearMouse(MousePos / DPI + FVector2D(12.f, 4.f));
}

void ULRInventoryGridWidget::CloseContextMenu()
{
	if (ActiveContextMenu)
	{
		ActiveContextMenu->RemoveFromParent();
		ActiveContextMenu = nullptr;
	}
}
