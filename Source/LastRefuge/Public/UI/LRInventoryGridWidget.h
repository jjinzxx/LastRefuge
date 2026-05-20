#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/LRInventoryStructs.h"
#include "LRInventoryGridWidget.generated.h"

class ULRInventoryGridComponent;
class ULRItemWidget;
class ULRTooltipWidget;
class ULRContextMenuWidget;
class UCanvasPanel;

/**
 * 그리드 전체를 표시하는 루트 위젯.
 * Blueprint 서브클래스(WBP_InventoryGrid)에서 반드시 바인딩:
 *   - GridCanvas      (UCanvasPanel)
 *   - ItemWidgetClass (WBP_Item 서브클래스)
 *
 * 사용법:
 *   1. CreateWidget으로 생성
 *   2. InitGrid(GridComponent, [SlotSize]) 호출
 *   3. AddToViewport
 */
UCLASS()
class LASTREFUGE_API ULRInventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 그리드 컴포넌트를 연결하고 초기 빌드.
	 * @param InStorageGrid  Shift+클릭 전송 대상 (인벤토리 위젯이면 창고 그리드, 반대도 마찬가지)
	 */
	void InitGrid(ULRInventoryGridComponent* InGridComponent,
	              ULRInventoryGridComponent* InStorageGrid = nullptr,
	              float InSlotSize = 50.f);

	// BP에서 BindWidget
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> GridCanvas;

	// 그리드 배경 오버레이 — InitGrid에서 자동으로 숨김 처리
	// (배경은 NativePaint에서 직접 그리므로 WBP에서 삭제해도 무방)
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DimOverlay;

	// BP에서 설정
	UPROPERTY(EditDefaultsOnly, Category = "LR|Inventory")
	TSubclassOf<ULRItemWidget> ItemWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "LR|Inventory")
	TSubclassOf<ULRTooltipWidget> TooltipWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "LR|Inventory")
	TSubclassOf<ULRContextMenuWidget> ContextMenuWidgetClass;

	// 단독 표시 시 배경 패널 표시 여부 (WBP_Storage 내부에서 사용할 땐 false로 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LR|Style")
	bool bShowBackground = true;

	// ── 미니멀 라인아트 스타일 파라미터 ────────────────────────
	// NativePaint에서 직접 그리는 그리드 배경색
	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	FLinearColor GridBackgroundColor = FLinearColor(0.04f, 0.06f, 0.10f, 0.88f);

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	FLinearColor GridLineColor = FLinearColor(0.75f, 0.88f, 1.0f, 0.18f);

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	float GridLineThickness = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	FLinearColor HoverLineColor = FLinearColor(0.88f, 0.95f, 1.0f, 0.7f);

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	float HoverLineThickness = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	FLinearColor PreviewCanPlaceColor = FLinearColor(0.25f, 0.95f, 0.6f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	FLinearColor PreviewBlockedColor = FLinearColor(1.0f, 0.28f, 0.28f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	float PreviewLineThickness = 2.0f;

protected:
	virtual void NativeDestruct() override;

	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	                           const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                           int32 LayerId, const FWidgetStyle& InWidgetStyle,
	                           bool bParentEnabled) const override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry,
	                                        const FPointerEvent& InMouseEvent) override;

	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry,
	                                      const FPointerEvent& InMouseEvent) override;

	virtual void NativeOnDragDetected(const FGeometry& InGeometry,
	                                   const FPointerEvent& InMouseEvent,
	                                   UDragDropOperation*& OutOperation) override;

	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry,
	                                  const FPointerEvent& InMouseEvent) override;

	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	virtual bool NativeOnDragOver(const FGeometry& InGeometry,
	                               const FDragDropEvent& InDragDropEvent,
	                               UDragDropOperation* InOperation) override;

	virtual bool NativeOnDrop(const FGeometry& InGeometry,
	                           const FDragDropEvent& InDragDropEvent,
	                           UDragDropOperation* InOperation) override;

	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent,
	                                UDragDropOperation* InOperation) override;

	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent,
	                                    UDragDropOperation* InOperation) override;

private:
	UPROPERTY()
	TObjectPtr<ULRInventoryGridComponent> GridComponent;

	// Shift+클릭 전송 대상
	UPROPERTY()
	TObjectPtr<ULRInventoryGridComponent> StorageGrid;

	UPROPERTY()
	TObjectPtr<ULRTooltipWidget> ActiveTooltip;

	// 드래그 프리뷰 상태 (NativePaint로 직접 렌더링)
	FIntPoint PreviewGridPos  = FIntPoint(-1, -1);
	int32     PreviewItemW    = 1;
	int32     PreviewItemH    = 1;
	bool      bPreviewVisible = false;
	bool      bPreviewCanPlace = false;

	float SlotSize = 50.f;

	// 마우스 호버 중인 셀 (-1,-1 = 없음)
	FIntPoint HoveredCell = FIntPoint(-1, -1);

	// 드래그 대기 상태
	int32 PendingDragItemID  = INDEX_NONE;
	bool  bRightClickPending = false; // 우클릭 마우스업 시 컨텍스트메뉴 표시 여부

	// 활성 컨텍스트 메뉴
	UPROPERTY()
	TObjectPtr<ULRContextMenuWidget> ActiveContextMenu;

	// OnGridChanged 델리게이트 핸들
	FDelegateHandle GridChangedHandle;

	// ── 좌표 변환 ───────────────────────────────────────
	FIntPoint GetGridIndexFromMouse(FVector2D LocalPx) const;
	FVector2D GridToLocal(int32 GridX, int32 GridY)   const;

	// NativePaint에서 매 프레임 갱신 — GridCanvas의 위젯 내 오프셋
	// GetGridIndexFromMouse/NativeOnMouseMove에서 입력 좌표 보정에 사용
	mutable FVector2D CachedGridOrigin = FVector2D::ZeroVector;

	// ── UI 빌드 ─────────────────────────────────────────
	void RebuildGrid();
	void ShowPreview(int32 GridX, int32 GridY, const FLRGridItem& Item, bool bCanPlace);
	void HidePreview();
	void ShowContextMenu(int32 ItemID, const FLRGridItem& Item);
	void CloseContextMenu();
};
