#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/LRInventoryStructs.h"
#include "LRItemWidget.generated.h"

class UImage;
class UTextBlock;
class UBorder;
class ULRInventoryGridComponent;

/**
 * 그리드 위에 배치되는 개별 아이템 위젯.
 * 크기 = (Item.GetEffectiveWidth() * SlotSize, Item.GetEffectiveHeight() * SlotSize).
 *
 * Blueprint 서브클래스(WBP_Item)에서 반드시 바인딩:
 *   - ItemIcon  (UImage)
 */
UCLASS()
class LASTREFUGE_API ULRItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 생성 직후 호출. 아이콘 텍스처 및 크기 적용.
	 * @param InTargetStorageGrid  Shift+클릭 시 전송할 대상 그리드 (nullptr 가능)
	 */
	void Init(const FLRGridItem& InItem, int32 InItemID,
	          ULRInventoryGridComponent* InSourceGrid,
	          ULRInventoryGridComponent* InTargetStorageGrid,
	          float InSlotSize);

	// BP에서 BindWidget
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry,
	                                        const FPointerEvent& InMouseEvent) override;

	virtual void NativeOnDragDetected(const FGeometry& InGeometry,
	                                   const FPointerEvent& InMouseEvent,
	                                   UDragDropOperation*& OutOperation) override;

	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry,
	                                      const FPointerEvent& InMouseEvent) override;

	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent,
	                                    UDragDropOperation* InOperation) override;

private:
	FLRGridItem GridItem;
	int32 ItemID = INDEX_NONE;
	float SlotSize = 50.f;

	// 위젯 로컬 좌표 내 클릭 지점 → 슬롯 단위 오프셋
	FVector2D GrabOffsetSlots = FVector2D::ZeroVector;

	UPROPERTY()
	TObjectPtr<ULRInventoryGridComponent> SourceGridComponent;

	// Shift+클릭으로 빠른 전송할 대상 (인벤토리↔창고)
	UPROPERTY()
	TObjectPtr<ULRInventoryGridComponent> TargetStorageGrid;

	void QuickTransferToTarget();
};
