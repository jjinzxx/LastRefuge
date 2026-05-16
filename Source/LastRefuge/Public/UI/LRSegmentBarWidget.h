#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LRSegmentBarWidget.generated.h"

UCLASS()
class LASTREFUGE_API ULRSegmentBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPercent(float InPercent);

	// 세그먼트 개수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SegmentBar")
	int32 SegmentCount = 10;

	// 채워진 세그먼트 색
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SegmentBar")
	FLinearColor FilledColor = FLinearColor(0.75f, 0.04f, 0.04f, 1.f);

	// 비어있는 세그먼트 색
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SegmentBar")
	FLinearColor EmptyColor = FLinearColor(0.12f, 0.02f, 0.02f, 1.f);

	// 세그먼트 사이 간격 (px)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SegmentBar")
	float GapSize = 4.f;

	virtual void NativeConstruct() override;

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	float CurrentPercent = 1.f;
};
