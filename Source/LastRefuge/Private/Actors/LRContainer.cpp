#include "Actors/LRContainer.h"
#include "Components/StaticMeshComponent.h"
#include "Character/LRCharacter.h"
#include "Components/LRInventoryGridComponent.h"
#include "Items/LRItemDataAsset.h"
#include "Items/LRInventoryStructs.h"
#include "Kismet/GameplayStatics.h"

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

		// 같은 종류 아이템이 있고 MaxStackSize에 여유가 있으면 수량 합산
		if (ItemData->MaxStackSize > 1)
		{
			bool bStacked = false;
			for (const auto& [ID, ExistingItem] : ContainerGrid->GetItems())
			{
				if (ExistingItem.ItemData == ItemData &&
					ExistingItem.Quantity < ItemData->MaxStackSize)
				{
					ContainerGrid->AddToStack(ID, 1);
					bStacked = true;
					break;
				}
			}
			if (bStacked) continue;
		}

		// 스태킹 불가 → 빈 슬롯에 새로 배치
		FLRGridItem NewItem;
		NewItem.ItemData = ItemData;
		NewItem.Width    = ItemData->GridWidth;
		NewItem.Height   = ItemData->GridHeight;
		NewItem.Quantity = 1;

		int32 OutX, OutY;
		bool bOutRotated;
		if (ContainerGrid->FindEmptySpace(NewItem, OutX, OutY, bOutRotated))
			ContainerGrid->PlaceItem(OutX, OutY, NewItem, bOutRotated);
		else
			UE_LOG(LogTemp, Error, TEXT("[Container] 공간 없음: %s"), *ItemData->ItemName.ToString());
	}

	UE_LOG(LogTemp, Warning, TEXT("[Container] ContainerGrid 아이템 수: %d"), ContainerGrid->GetItems().Num());
}

void ALRContainer::BeginInteract(ALRCharacter* Player)
{
	if (bSearched) return;

	if (SFX_SearchStart)
		UGameplayStatics::PlaySoundAtLocation(this, SFX_SearchStart, GetActorLocation());

	UE_LOG(LogTemp, Log, TEXT("컨테이너: 수색 시작"));
}

void ALRContainer::EndInteract(ALRCharacter* Player)
{
	if (Player == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("컨테이너: 수색 취소"));
		return;
	}

	if (SFX_SearchComplete)
		UGameplayStatics::PlaySoundAtLocation(this, SFX_SearchComplete, GetActorLocation());

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
