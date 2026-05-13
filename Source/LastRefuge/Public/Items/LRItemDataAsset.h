#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LRItemDataAsset.generated.h"

UENUM(BlueprintType)
enum class ELRItemType : uint8
{
	Resource    UMETA(DisplayName = "Resource"),
	Consumable  UMETA(DisplayName = "Consumable"),
	Equipment   UMETA(DisplayName = "Equipment"),
	KeyItem     UMETA(DisplayName = "Key Item")
};

UCLASS(BlueprintType)
class LASTREFUGE_API ULRItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data", meta = (MultiLine = true))
	FText ItemDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	TObjectPtr<UTexture2D> ItemIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	ELRItemType ItemType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	float Weight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	int32 MaxStackSize;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Effect")
	float HealthRestore; // 체력 회복량

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Effect")
	float StaminaRestore; // 스테미나 회복량

	// 저장/로드 식별자 — 에디터에서 에셋마다 고유값으로 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FString ItemID;

	// 그리드 점유 크기 (슬롯 단위)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Grid", meta = (ClampMin = "1"))
	int32 GridWidth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Grid", meta = (ClampMin = "1"))
	int32 GridHeight = 1;
};