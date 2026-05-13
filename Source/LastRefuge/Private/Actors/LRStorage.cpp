#include "Actors/LRStorage.h"
#include "Character/LRCharacter.h"
#include "Components/LRInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "LRGameInstance.h"
#include "Kismet/GameplayStatics.h"

ALRStorage::ALRStorage()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
}

void ALRStorage::BeginPlay()
{
	Super::BeginPlay();

	// 레벨 이동 후 보관함 내용 복원
	ULRGameInstance* GI = Cast<ULRGameInstance>(UGameplayStatics::GetGameInstance(this));
	if (GI && GI->bHasTravelData)
	{
		StoredItems = GI->PersistentStorageItems;
	}
}

void ALRStorage::BeginInteract(ALRCharacter* Player)
{
}

void ALRStorage::EndInteract(ALRCharacter* Player)
{
	if (!Player) return;

	ULRInventoryComponent* Inventory = Player->FindComponentByClass<ULRInventoryComponent>();
	if (!Inventory) return;

	const TArray<FLRItemSlot>& PlayerSlots = Inventory->GetInventorySlots();
	const bool bPlayerHasItems = PlayerSlots.ContainsByPredicate(
		[](const FLRItemSlot& Slot) { return !Slot.IsEmpty(); });

	if (bPlayerHasItems)
	{
		// Deposit: 플레이어 인벤토리 전체를 보관함으로 이동
		for (const FLRItemSlot& Slot : PlayerSlots)
		{
			if (!Slot.IsEmpty())
			{
				StoredItems.Add(Slot);
			}
		}
		Inventory->ClearInventory();
		UE_LOG(LogTemp, Log, TEXT("[Storage] 아이템을 보관함에 저장했습니다. (보관 중: %d 슬롯)"), StoredItems.Num());
	}
	else
	{
		// Withdraw: 보관함 전체를 플레이어 인벤토리로 이동
		for (const FLRItemSlot& Slot : StoredItems)
		{
			if (!Slot.IsEmpty())
			{
				Inventory->AddItem(Slot.ItemData, Slot.Quantity);
			}
		}
		StoredItems.Empty();
		UE_LOG(LogTemp, Log, TEXT("[Storage] 보관함 아이템을 꺼냈습니다."));
	}
}

float ALRStorage::GetInteractionDuration() const
{
	return 0.f;
}

FText ALRStorage::GetInteractionPrompt() const
{
	return StoredItems.IsEmpty()
		? FText::FromString(TEXT("[E] 저장하기"))
		: FText::FromString(TEXT("[E] 꺼내기"));
}

FText ALRStorage::GetProgressText() const
{
	return FText::GetEmpty();
}

FText ALRStorage::GetStartText() const
{
	return FText::GetEmpty();
}

FText ALRStorage::GetCancelText() const
{
	return FText::GetEmpty();
}

FText ALRStorage::GetCompleteText() const
{
	return FText::GetEmpty();
}
