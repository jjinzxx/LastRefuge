#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LRStorageWidget.generated.h"

class ULRInventoryGridComponent;
class ULRInventoryGridWidget;
class UCanvasPanel;

/**
 * 보관함 UI — 인벤토리(좌) + 창고(우) 나란히 표시.
 *
 * Blueprint(WBP_Storage) Designer 구성:
 *   Canvas Panel (루트)
 *   ├── Canvas Panel  → 변수명: InventoryContainer  (좌측 영역)
 *   └── Canvas Panel  → 변수명: StorageContainer    (우측 영역)
 *
 * WBP_InventoryGrid 인스턴스는 C++에서 직접 생성하여 각 Container에 추가.
 * BP에서 GridWidgetClass = WBP_InventoryGrid 로 설정 필요.
 */
UCLASS()
class LASTREFUGE_API ULRStorageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitStorage(ULRInventoryGridComponent* InvGrid,
	                 ULRInventoryGridComponent* InStorageGrid);

	// Designer에서 배치하는 빈 컨테이너 패널 (BindWidget)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> InventoryContainer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> StorageContainer;

	// BP에서 WBP_InventoryGrid 할당
	UPROPERTY(EditDefaultsOnly, Category = "LR|Storage")
	TSubclassOf<ULRInventoryGridWidget> GridWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<ULRInventoryGridWidget> PlayerInventoryWidget;

	UPROPERTY()
	TObjectPtr<ULRInventoryGridWidget> StorageGridWidget;
};
