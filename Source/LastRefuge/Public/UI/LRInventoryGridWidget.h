#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/LRInventoryStructs.h"
#include "LRInventoryGridWidget.generated.h"

class ULRInventoryGridComponent;
class ULRDragPreviewWidget;
class ULRItemWidget;
class UCanvasPanel;

/**
 * 그리드 전체를 표시하는 루트 위젯.
 * Blueprint 서브클래스(WBP_InventoryGrid)에서 반드시 바인딩:
 *   - GridCanvas      (UCanvasPanel)
 *   - ItemWidgetClass (WBP_Item 서브클래스)
 *   - PreviewWidgetClass (WBP_DragPreview 서브클래스)
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

	// BP에서 설정
	UPROPERTY(EditDefaultsOnly, Category = "LR|Inventory")
	TSubclassOf<ULRItemWidget> ItemWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "LR|Inventory")
	TSubclassOf<ULRDragPreviewWidget> PreviewWidgetClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct()  override;

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
	TObjectPtr<ULRDragPreviewWidget> PreviewWidget;

	float SlotSize = 50.f;

	// OnGridChanged 델리게이트 핸들
	FDelegateHandle GridChangedHandle;

	// ── 좌표 변환 ───────────────────────────────────────
	FIntPoint GetGridIndexFromMouse(FVector2D LocalPx) const;
	FVector2D GridToLocal(int32 GridX, int32 GridY)   const;

	// ── UI 빌드 ─────────────────────────────────────────
	void RebuildGrid();
	void ShowPreview(int32 GridX, int32 GridY, const FLRGridItem& Item, bool bCanPlace);
	void HidePreview();
};
