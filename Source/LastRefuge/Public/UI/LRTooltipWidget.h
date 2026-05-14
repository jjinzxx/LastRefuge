#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/LRInventoryStructs.h"
#include "LRTooltipWidget.generated.h"

class UTextBlock;

/**
 * 아이템 호버 시 표시되는 툴팁 위젯.
 * Blueprint 서브클래스(WBP_Tooltip)에서 반드시 바인딩:
 *   - NameText        (UTextBlock) — 아이템 이름
 *   - TypeText        (UTextBlock) — 아이템 타입
 *   - SizeText        (UTextBlock) — 크기 / 무게
 *   - DescriptionText (UTextBlock) — 설명
 */
UCLASS()
class LASTREFUGE_API ULRTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetItem(const FLRGridItem& Item);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TypeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SizeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;
};
