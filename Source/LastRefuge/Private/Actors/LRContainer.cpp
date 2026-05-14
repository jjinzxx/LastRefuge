#include "Actors/LRContainer.h"
#include "Components/StaticMeshComponent.h"
#include "Character/LRCharacter.h"
#include "Components/LRInventoryGridComponent.h"
#include "Items/LRItemDataAsset.h"
#include "Items/LRInventoryStructs.h"

ALRContainer::ALRContainer()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	ContainerGrid = CreateDefaultSubobject<ULRInventoryGridComponent>(TEXT("ContainerGrid"));
}

void ALRContainer::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[Container] BeginPlay — LootTable 수: %d"), LootTable.Num());

	for (ULRItemDataAsset* ItemData : LootTable)
	{
		if (!ItemData) continue;

		FLRGridItem NewItem;
		NewItem.ItemData = ItemData;
		NewItem.Width    = ItemData->GridWidth;
		NewItem.Height   = ItemData->GridHeight;

		int32 OutX, OutY;
		bool bOutRotated;
		if (ContainerGrid->FindEmptySpace(NewItem, OutX, OutY, bOutRotated))
		{
			ContainerGrid->PlaceItem(OutX, OutY, NewItem, bOutRotated);
			UE_LOG(LogTemp, Warning, TEXT("[Container] 아이템 배치: %s → (%d,%d)"), *ItemData->ItemName.ToString(), OutX, OutY);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[Container] 공간 없음: %s"), *ItemData->ItemName.ToString());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Container] ContainerGrid 아이템 수: %d"), ContainerGrid->GetItems().Num());
}

void ALRContainer::BeginInteract(ALRCharacter* Player)
{
	if (bSearched) return;

	UE_LOG(LogTemp, Log, TEXT("컨테이너: 수색 시작"));
}

void ALRContainer::EndInteract(ALRCharacter* Player)
{
	if (Player == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("컨테이너: 수색 취소"));
		return;
	}

	bSearched = true;
	Player->OpenStorageScreen(ContainerGrid);
}

float ALRContainer::GetInteractionDuration() const
{
	// 수색 완료 후 재접근 시 즉시 열기
	return bSearched ? 0.f : SearchDuration;
}

FText ALRContainer::GetInteractionPrompt() const
{
	return bSearched
		? FText::FromString(TEXT("[E] 다시 열기"))
		: FText::FromString(TEXT("[E] 열기"));
}

FText ALRContainer::GetProgressText() const
{
	return FText::FromString(TEXT("수색중"));
}

FText ALRContainer::GetStartText() const
{
	return FText::FromString(TEXT("수색 시작..."));
}

FText ALRContainer::GetCancelText() const
{
	return FText::FromString(TEXT("수색이 취소되었습니다."));
}

FText ALRContainer::GetCompleteText() const
{
	return FText::FromString(TEXT("수색완료"));
}
