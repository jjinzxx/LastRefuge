#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/LRInventoryStructs.h"
#include "LRContextMenuWidget.generated.h"

class UVerticalBox;
class UButton;
class UTextBlock;
class ULRInventoryGridComponent;

/**
 * 우클릭 컨텍스트 메뉴.
 * BP 서브클래스(WBP_ContextMenu)에서 BindWidget:
 *   - MenuBox (UVerticalBox)
 */
UCLASS()
class LASTREFUGE_API ULRContextMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> MenuBox;

	// BP에서 이름 'BackdropButton'으로 연결 — 전체화면 투명 버튼, 클릭 시 메뉴 닫힘
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackdropButton;

	/** 메뉴 초기화 — 아이템 종류에 따라 버튼 생성 */
	void InitMenu(const FLRGridItem& InItem, int32 InItemID, ULRInventoryGridComponent* InGrid);

	/** MenuBox를 뷰포트 좌표(DPI 스케일 적용) 위치로 이동 */
	void PositionNearMouse(FVector2D ViewportPos);

	/** 메뉴 닫힘 알림 (그리드 위젯에서 구독) */
	FSimpleMulticastDelegate OnMenuClosed;

private:
	int32 CachedItemID = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<ULRInventoryGridComponent> CachedGrid;

	// 버튼별 액션 (최대 3개)
	TArray<TFunction<void()>> CachedActions;

	/** 버튼 항목 추가 */
	void AddItem(const FText& Label);

	/** 버튼 인덱스로 액션 실행 후 닫기 */
	void ExecuteAndClose(int32 Index);

	// 각 버튼에 바인딩할 UFUNCTION (최대 3개)
	UFUNCTION() void OnBtn0Clicked()     { ExecuteAndClose(0); }
	UFUNCTION() void OnBtn1Clicked()     { ExecuteAndClose(1); }
	UFUNCTION() void OnBtn2Clicked()     { ExecuteAndClose(2); }
	UFUNCTION() void OnBackdropClicked() { ExecuteAndClose(INDEX_NONE); }
};
