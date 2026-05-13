#include "UI/LRStorageWidget.h"
#include "UI/LRInventoryGridWidget.h"
#include "Components/LRInventoryGridComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

void ULRStorageWidget::InitStorage(
	ULRInventoryGridComponent* InvGrid,
	ULRInventoryGridComponent* InStorageGrid)
{
	if (!GridWidgetClass || !InventoryContainer || !StorageContainer) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 좌측: 플레이어 인벤토리
	PlayerInventoryWidget = CreateWidget<ULRInventoryGridWidget>(GetOwningPlayer(), GridWidgetClass);
	if (PlayerInventoryWidget)
	{
		UCanvasPanelSlot* InvSlot = InventoryContainer->AddChildToCanvas(PlayerInventoryWidget);
		if (InvSlot)
		{
			InvSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			InvSlot->SetOffsets(FMargin(0.f));
		}
		PlayerInventoryWidget->InitGrid(InvGrid, InStorageGrid);
	}

	// 우측: 창고
	StorageGridWidget = CreateWidget<ULRInventoryGridWidget>(GetOwningPlayer(), GridWidgetClass);
	if (StorageGridWidget)
	{
		UCanvasPanelSlot* StgSlot = StorageContainer->AddChildToCanvas(StorageGridWidget);
		if (StgSlot)
		{
			StgSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			StgSlot->SetOffsets(FMargin(0.f));
		}
		StorageGridWidget->InitGrid(InStorageGrid, InvGrid);
	}
}
