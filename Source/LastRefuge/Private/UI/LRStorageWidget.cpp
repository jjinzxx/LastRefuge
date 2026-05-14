#include "UI/LRStorageWidget.h"
#include "UI/LRInventoryGridWidget.h"
#include "Components/LRInventoryGridComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

void ULRStorageWidget::InitStorage(
	ULRInventoryGridComponent* InvGrid,
	ULRInventoryGridComponent* InStorageGrid)
{
	if (!GridWidgetClass)      { UE_LOG(LogTemp, Error, TEXT("[Storage] GridWidgetClass null — WBP_Storage에서 GridWidgetClass 할당 필요")); return; }
	if (!InventoryContainer)   { UE_LOG(LogTemp, Error, TEXT("[Storage] InventoryContainer null — WBP_Storage 디자이너에 'InventoryContainer' Canvas Panel 필요")); return; }
	if (!StorageContainer)     { UE_LOG(LogTemp, Error, TEXT("[Storage] StorageContainer null — WBP_Storage 디자이너에 'StorageContainer' Canvas Panel 필요")); return; }
	if (!InStorageGrid)        { UE_LOG(LogTemp, Error, TEXT("[Storage] InStorageGrid null")); return; }

	UE_LOG(LogTemp, Warning, TEXT("[Storage] InitStorage 시작 — ContainerGrid 아이템 수: %d"), InStorageGrid->GetItems().Num());

	// 그리드 위젯 크기 = GridWidth * SlotSize x GridHeight * SlotSize
	// fill anchor를 쓰면 위젯이 컨테이너 전체를 채워 아이템(50px 단위)이 좌상단에만 몰림
	constexpr float SlotSizePx = 50.f;

	// 좌측: 플레이어 인벤토리
	PlayerInventoryWidget = CreateWidget<ULRInventoryGridWidget>(GetOwningPlayer(), GridWidgetClass);
	if (PlayerInventoryWidget)
	{
		PlayerInventoryWidget->InitGrid(InvGrid, InStorageGrid, SlotSizePx);

		UCanvasPanelSlot* InvSlot = InventoryContainer->AddChildToCanvas(PlayerInventoryWidget);
		if (InvSlot)
		{
			InvSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			InvSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			InvSlot->SetSize(FVector2D(InvGrid->GridWidth * SlotSizePx,
			                           InvGrid->GridHeight * SlotSizePx));
			InvSlot->SetAutoSize(false);
		}
	}

	// 우측: 창고
	StorageGridWidget = CreateWidget<ULRInventoryGridWidget>(GetOwningPlayer(), GridWidgetClass);
	if (StorageGridWidget)
	{
		StorageGridWidget->InitGrid(InStorageGrid, InvGrid, SlotSizePx);

		UCanvasPanelSlot* StgSlot = StorageContainer->AddChildToCanvas(StorageGridWidget);
		if (StgSlot)
		{
			StgSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			StgSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			StgSlot->SetSize(FVector2D(InStorageGrid->GridWidth * SlotSizePx,
			                           InStorageGrid->GridHeight * SlotSizePx));
			StgSlot->SetAutoSize(false);
		}
	}
}
