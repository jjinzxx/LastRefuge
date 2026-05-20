#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/LRInventoryStructs.h"
#include "LRInventoryGridComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnGridChanged);

/**
 * 그리드 기반 인벤토리 컴포넌트.
 * 내부적으로 1차원 배열을 2차원처럼 인덱싱 [y * GridWidth + x].
 * 아이템은 고유 ID(int32)로 관리되어 배열 재배치 없이 O(1) 삭제.
 */
UCLASS(ClassGroup=(LR), meta=(BlueprintSpawnableComponent))
class LASTREFUGE_API ULRInventoryGridComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULRInventoryGridComponent();
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LR|Grid")
	int32 GridWidth = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LR|Grid")
	int32 GridHeight = 5;

	// ── 핵심 조작 ─────────────────────────────────────────

	/** 배치 가능 여부 검사 (Grid 변경 없음, const) */
	bool CheckPlacement(int32 X, int32 Y, const FLRGridItem& Item, bool bRotated) const;

	/** 실제 배치. 성공 시 할당된 ItemID 반환, 실패 시 INDEX_NONE */
	int32 PlaceItem(int32 X, int32 Y, FLRGridItem Item, bool bRotated);

	/** 아이템 제거. 제거된 FLRGridItem 반환 */
	FLRGridItem RemoveItem(int32 InItemID);

	/** 소비 아이템 사용 — 수량 1 감소, 0이면 제거 */
	bool UseItem(int32 InItemID);

	/** (X,Y) 셀의 아이템에 스택 가능한지 확인 */
	bool CanStack(int32 X, int32 Y, const FLRGridItem& InItem) const;

	/** ItemID 아이템에 수량 추가. 실제 추가된 수량 반환 */
	int32 AddToStack(int32 InItemID, int32 Amount);

	/** 수량 감소. 0이 되면 제거. 남은 수량 반환 (제거 시 0) */
	int32 ReduceQuantity(int32 InItemID, int32 Amount);

	// ── 빈 공간 탐색 ──────────────────────────────────────

	/**
	 * First-fit 알고리즘으로 빈 공간 탐색.
	 * 원본 방향 → 회전 순서로 시도.
	 * 반환: 공간 있으면 true, OutX/OutY/bOutRotated에 결과 기록.
	 */
	bool FindEmptySpace(const FLRGridItem& Item,
	                    int32& OutX, int32& OutY, bool& bOutRotated) const;

	// ── 조회 ──────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "LR|Grid")
	int32 GetItemIDAt(int32 X, int32 Y) const;

	const TMap<int32, FLRGridItem>& GetItems() const { return Items; }
	const TArray<int32>& GetGrid()             const { return Grid; }

	void ClearGrid();

	/** 인벤토리 전체 무게 합산 (kg) */
	float GetTotalWeight() const;

	// UI가 구독하는 변경 델리게이트
	FOnGridChanged OnGridChanged;

private:
	// [y * GridWidth + x] = ItemID,  INDEX_NONE = 빈 셀
	TArray<int32> Grid;

	// ItemID → FLRGridItem
	TMap<int32, FLRGridItem> Items;

	int32 NextItemID = 0;
};
