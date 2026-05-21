#include "UI/LRToolbarSlotWidget.h"
#include "Character/LRCharacter.h"
#include "UI/LRItemDragDropOperation.h"
#include "Items/LRItemDataAsset.h"
#include "Items/LRInventoryStructs.h"
#include "Components/LRInventoryGridComponent.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void ULRToolbarSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (IconImage)
		DefaultBrush = IconImage->GetBrush();
	if (SlotNumberText)
		SlotNumberText->SetText(FText::AsNumber(SlotIndex + 1));
}

void ULRToolbarSlotWidget::InitSlot(ALRCharacter* InCharacter, int32 InSlotIndex)
{
	OwnerCharacter = InCharacter;
	SlotIndex      = InSlotIndex;
	if (SlotNumberText)
		SlotNumberText->SetText(FText::AsNumber(SlotIndex + 1));
}

void ULRToolbarSlotWidget::RefreshSlot(ULRItemDataAsset* ItemData, int32 Quantity)
{
	if (ItemData && ItemData->ItemIcon)
	{
		// SetBrushFromTexture는 기존 브러시의 TintColor를 유지해 실루엣처럼 보일 수 있음.
		// FSlateBrush를 직접 생성해 TintColor를 White로 명시적으로 설정.
		FSlateBrush NewBrush;
		NewBrush.SetResourceObject(ItemData->ItemIcon);
		NewBrush.TintColor = FSlateColor(FLinearColor::White);
		NewBrush.DrawAs    = ESlateBrushDrawType::Image;
		IconImage->SetBrush(NewBrush);
		IconImage->SetColorAndOpacity(FLinearColor::White);
	}
	else
	{
		IconImage->SetBrush(DefaultBrush);
		IconImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.2f));
	}

	if (QuantityText)
	{
		if (ItemData && Quantity > 1)
		{
			QuantityText->SetText(FText::AsNumber(Quantity));
			QuantityText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			QuantityText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

FReply ULRToolbarSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && OwnerCharacter)
	{
		// 우클릭: 슬롯 비우기 + 인벤토리로 반환
		OwnerCharacter->ClearToolbarSlot(SlotIndex);
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// 좌클릭: 드래그 감지 시작
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return FReply::Unhandled();
}

void ULRToolbarSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (!OwnerCharacter) return;

	const TArray<FLRGridItem>& Items = OwnerCharacter->GetToolbarItems();
	if (!Items.IsValidIndex(SlotIndex) || Items[SlotIndex].IsEmpty()) return;

	// 슬롯에서 아이템 꺼내기 (인벤 반환 없이 제거)
	FLRGridItem Taken = OwnerCharacter->TakeToolbarItem(SlotIndex);

	ULRItemDragDropOperation* Op = NewObject<ULRItemDragDropOperation>(this);
	Op->DraggedItem  = Taken;
	Op->SourceGrid   = nullptr;   // 툴바는 그리드 컴포넌트 없음
	Op->SourceItemID = INDEX_NONE;
	Op->Pivot        = EDragPivot::TopLeft;

	OutOperation = Op;
}

bool ULRToolbarSlotWidget::NativeOnDragOver(const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	return Cast<ULRItemDragDropOperation>(InOperation) != nullptr;
}

bool ULRToolbarSlotWidget::NativeOnDrop(const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ULRItemDragDropOperation* Op = Cast<ULRItemDragDropOperation>(InOperation);
	if (!Op || !OwnerCharacter || !Op->DraggedItem.ItemData)
		return false;

	// 현재 슬롯에 아이템이 있으면 소스로 스왑
	const TArray<FLRGridItem>& CurrentItems = OwnerCharacter->GetToolbarItems();
	if (CurrentItems.IsValidIndex(SlotIndex) && !CurrentItems[SlotIndex].IsEmpty())
	{
		FLRGridItem Existing = CurrentItems[SlotIndex];
		if (Op->SourceGrid)
		{
			// 인벤→툴바 드롭 시: 기존 툴바 아이템을 소스 그리드 빈 자리로
			int32 OutX, OutY; bool bRot;
			if (Op->SourceGrid->FindEmptySpace(Existing, OutX, OutY, bRot))
				Op->SourceGrid->PlaceItem(OutX, OutY, Existing, bRot);
		}
		// 툴바→툴바 드롭 시: Existing은 TakeToolbarItem으로 이미 비워진 상태라 처리 불필요
	}

	// 인벤→툴바: 소스 그리드에서 이미 제거됨 (DragDetected에서 RemoveItem)
	// 툴바→툴바: TakeToolbarItem으로 이미 제거됨
	OwnerCharacter->SetToolbarSlot(SlotIndex, Op->DraggedItem);
	return true;
}
