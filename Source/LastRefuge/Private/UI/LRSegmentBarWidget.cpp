#include "UI/LRSegmentBarWidget.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateBrush.h"

void ULRSegmentBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (TSharedPtr<SWidget> SafeWidget = GetCachedWidget())
	{
		SafeWidget->RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateWeakLambda(this,
			[this](double, float) -> EActiveTimerReturnType
			{
				Invalidate(EInvalidateWidgetReason::Paint);
				return EActiveTimerReturnType::Continue;
			}
		));
	}
}

void ULRSegmentBarWidget::SetPercent(float InPercent)
{
	CurrentPercent = FMath::Clamp(InPercent, 0.f, 1.f);
}

int32 ULRSegmentBarWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (SegmentCount <= 0) return LayerId;

	const FVector2f Size = AllottedGeometry.GetLocalSize();
	const float TotalGap = GapSize * (SegmentCount - 1);
	const float SegW = (Size.X - TotalGap) / SegmentCount;
	const float SegH = Size.Y;

	const int32 FilledCount = FMath::RoundToInt(CurrentPercent * SegmentCount);

	FSlateColorBrush FilledBrush(FilledColor);
	FSlateColorBrush EmptyBrush(EmptyColor);

	for (int32 i = 0; i < SegmentCount; i++)
	{
		const float X = i * (SegW + GapSize);
		const FSlateColorBrush& Brush = (i < FilledCount) ? FilledBrush : EmptyBrush;

		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2f(SegW, SegH), FSlateLayoutTransform(FVector2f(X, 0.f))),
			&Brush);
	}

	return LayerId + 1;
}
