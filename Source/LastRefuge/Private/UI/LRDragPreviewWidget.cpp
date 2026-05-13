#include "UI/LRDragPreviewWidget.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"

const FLinearColor ULRDragPreviewWidget::ColorCanPlace    = FLinearColor(0.f, 1.f, 0.f, 0.35f);
const FLinearColor ULRDragPreviewWidget::ColorCannotPlace = FLinearColor(1.f, 0.f, 0.f, 0.35f);

void ULRDragPreviewWidget::SetHighlight(bool bCanPlace)
{
	if (HighlightImage)
	{
		HighlightImage->SetColorAndOpacity(bCanPlace ? ColorCanPlace : ColorCannotPlace);
	}
}

void ULRDragPreviewWidget::SetGridSize(int32 W, int32 H, float InSlotSize)
{
	// 위젯 자체 크기 — Canvas Panel Slot의 Size를 직접 설정
	SetDesiredSizeInViewport(FVector2D(W * InSlotSize, H * InSlotSize));
}
