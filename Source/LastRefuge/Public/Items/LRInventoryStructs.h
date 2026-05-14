#pragma once

#include "CoreMinimal.h"
#include "Items/LRItemDataAsset.h"
#include "LRInventoryStructs.generated.h"

// ──────────────────────────────────────────────────────────
// 레거시 슬롯 (현재 미사용 — 향후 제거 예정)
// ──────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FLRItemSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TObjectPtr<ULRItemDataAsset> ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Quantity;

	FLRItemSlot() : ItemData(nullptr), Quantity(0) {}

	bool IsEmpty() const { return ItemData == nullptr || Quantity <= 0; }
};

// ──────────────────────────────────────────────────────────
// 그리드 기반 아이템 슬롯 (핵심 구조체)
// ──────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FLRGridItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	TObjectPtr<ULRItemDataAsset> ItemData = nullptr;

	// 그리드 내 좌상단 기준 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 GridX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 GridY = 0;

	// DataAsset 기준 원본 크기 (회전 전)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 Width = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 Height = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	bool bIsRotated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
	int32 Quantity = 1;

	bool IsEmpty() const { return ItemData == nullptr; }

	// 회전 반영된 실제 점유 크기
	int32 GetEffectiveWidth()  const { return bIsRotated ? Height : Width; }
	int32 GetEffectiveHeight() const { return bIsRotated ? Width  : Height; }
};

// ──────────────────────────────────────────────────────────
// 저장 파일용 직렬화 구조체
// ──────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FLRSavedItem
{
	GENERATED_BODY()

	UPROPERTY()
	FString ItemID;       // ULRItemDataAsset::ItemID 필드와 매핑

	UPROPERTY()
	int32 GridX = 0;

	UPROPERTY()
	int32 GridY = 0;

	UPROPERTY()
	bool bIsRotated = false;

	UPROPERTY()
	bool bIsStorage = false; // true = 창고, false = 인벤토리

	UPROPERTY()
	int32 Quantity = 1;
};
