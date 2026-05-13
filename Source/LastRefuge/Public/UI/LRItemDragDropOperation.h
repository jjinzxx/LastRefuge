#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Items/LRInventoryStructs.h"
#include "LRItemDragDropOperation.generated.h"

/**
 * 인벤토리 아이템 드래그 앤 드롭 오퍼레이션.
 * NativeOnDragDetected에서 생성되고, NativeOnDrop에서 소비된다.
 */
UCLASS()
class LASTREFUGE_API ULRItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// 드래그 중인 아이템 데이터 (복사본 — 소스 그리드에서 이미 제거됨)
	UPROPERTY()
	FLRGridItem DraggedItem;

	// 아이템이 원래 있던 그리드 컴포넌트
	UPROPERTY()
	TObjectPtr<class ULRInventoryGridComponent> SourceGrid;

	// 소스 그리드에서의 ItemID (드롭 취소 시 원위치 복원에 사용)
	int32 SourceItemID = INDEX_NONE;

	/**
	 * 아이템 위젯 내에서 마우스가 클릭된 슬롯 단위 오프셋.
	 * 드래그 중 아이템이 클릭 지점을 기준으로 따라오도록 하는 핵심 값.
	 * 예: 2x3 아이템의 (1, 1) 슬롯을 클릭했으면 GrabOffsetSlots = (1.f, 1.f)
	 */
	FVector2D GrabOffsetSlots = FVector2D::ZeroVector;
};
