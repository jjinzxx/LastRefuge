#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LRToolbarSlotWidget.generated.h"

class UImage;
class UTextBlock;
class ALRCharacter;
class ULRItemDataAsset;

UCLASS()
class LASTREFUGE_API ULRToolbarSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitSlot(ALRCharacter* InCharacter, int32 InSlotIndex);
	void RefreshSlot(ULRItemDataAsset* ItemData, int32 Quantity);

protected:
	virtual void NativeConstruct() override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	// WBP_ToolbarSlot 에서 반드시 배치
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;

	// 선택적 — 수량 표시 텍스트
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuantityText;

	// 선택적 — "1" "2" "3" "4" 단축키 번호
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotNumberText;

private:
	UPROPERTY()
	TObjectPtr<ALRCharacter> OwnerCharacter;

	int32 SlotIndex = 0;
};
