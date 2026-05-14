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
};
