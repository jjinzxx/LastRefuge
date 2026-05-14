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

	constexpr float SlotSizePx = 60.f;

	if (!InvGrid)
	{
		UE_LOG(LogTemp, Error, TEXT("[Storage] InvGrid(플레이어 인벤토리) null — BP_LRCharacter에서 InventoryGrid 컴포넌트 확인 필요"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Storage] InvGrid OK — GridWidth=%d GridHeight=%d Items=%d"),
			InvGrid->GridWidth, InvGrid->GridHeight, InvGrid->GetItems().Num());
	}
	UE_LOG(LogTemp, Warning, TEXT("[Storage] StorageGrid — GridWidth=%d GridHeight=%d"),
		InStorageGrid->GridWidth, InStorageGrid->GridHeight);

	// 좌측: 플레이어 인벤토리
	PlayerInventoryWidget = CreateWidget<ULRInventoryGridWidget>(GetOwningPlayer(), GridWidgetClass);
	if (PlayerInventoryWidget && InvGrid)
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
