#include "Actors/LRStorage.h"
#include "Character/LRCharacter.h"
#include "Components/LRInventoryGridComponent.h"
#include "Components/StaticMeshComponent.h"
#include "LRGameInstance.h"
#include "Kismet/GameplayStatics.h"

ALRStorage::ALRStorage()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	StorageGrid = CreateDefaultSubobject<ULRInventoryGridComponent>(TEXT("StorageGrid"));
}

void ALRStorage::BeginPlay()
{
	Super::BeginPlay();

	// 레벨 이동 후 보관함 내용 복원
	ULRGameInstance* GI = Cast<ULRGameInstance>(UGameplayStatics::GetGameInstance(this));
	if (GI && GI->bHasTravelData && StorageGrid)
	{
		for (const FLRGridItem& Item : GI->PersistentStorageItems)
		{
			if (!Item.IsEmpty())
				StorageGrid->PlaceItem(Item.GridX, Item.GridY, Item, Item.bIsRotated);
		}
	}
}

void ALRStorage::BeginInteract(ALRCharacter* Player)
{
}

// E 입력 → 그리드 UI 열기 (이미 열려 있으면 닫힘)
void ALRStorage::EndInteract(ALRCharacter* Player)
{
	if (!Player || !StorageGrid) return;
	Player->OpenStorageScreen(StorageGrid);
}

float ALRStorage::GetInteractionDuration() const { return 0.f; }

FText ALRStorage::GetInteractionPrompt() const
{
	return FText::FromString(TEXT("[E] 보관함 열기"));
}

FText ALRStorage::GetProgressText()  const { return FText::GetEmpty(); }
FText ALRStorage::GetStartText()     const { return FText::GetEmpty(); }
FText ALRStorage::GetCancelText()    const { return FText::GetEmpty(); }
FText ALRStorage::GetCompleteText()  const { return FText::GetEmpty(); }
