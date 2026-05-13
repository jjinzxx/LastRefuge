#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LRDragPreviewWidget.generated.h"

class UImage;

/**
 * 드래그 중 그리드 위에 표시되는 하이라이트 오버레이.
 * Blueprint에서 WBP_DragPreview로 서브클래싱하여 HighlightImage 바인딩.
 * SetHighlight(true) → 초록, SetHighlight(false) → 빨강.
 */
UCLASS()
class LASTREFUGE_API ULRDragPreviewWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 배치 가능 여부에 따라 색상 전환 */
	void SetHighlight(bool bCanPlace);

	/**
	 * 드래그 중인 아이템 크기에 맞게 위젯 크기 조정.
	 * CanvasPanel의 SizeBox 또는 오버레이 크기를 직접 설정.
	 */
	void SetGridSize(int32 W, int32 H, float InSlotSize);

	// BP에서 BindWidget으로 연결
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> HighlightImage;

private:
	static const FLinearColor ColorCanPlace;    // 초록 35% 불투명
	static const FLinearColor ColorCannotPlace; // 빨강 35% 불투명
};
