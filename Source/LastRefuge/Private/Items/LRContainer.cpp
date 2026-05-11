#include "Items/LRContainer.h"
#include "Components/StaticMeshComponent.h"
#include "Character/LRCharacter.h"
#include "Components/LRInventoryComponent.h"
#include "Items/LRItemDataAsset.h"

ALRContainer::ALRContainer()
{
	PrimaryActorTick.bCanEverTick = false;

	// 간단한 메쉬 컴포넌트 추가
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;
}

void ALRContainer::BeginInteract(ALRCharacter* Player)
{
	if (bSearched) return;
    
	// (사운드 재생 등을 여기서 처리할 수 있음)
	UE_LOG(LogTemp, Log, TEXT("컨테이너: 플레이어가 수색을 시작했습니다."));
}

void ALRContainer::EndInteract(ALRCharacter* Player)
{
	// Player가 nullptr로 넘어오면 취소된 것, 유효하면 수색 성공
	if (Player == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("컨테이너: 수색이 중간에 취소되었습니다."));
		return;
	}

	if (!bSearched)
	{
		bSearched = true; // 다시 못 뒤지게 막기
        
		// 플레이어의 인벤토리 컴포넌트 찾기
		ULRInventoryComponent* Inventory = Player->FindComponentByClass<ULRInventoryComponent>();
		if (Inventory)
		{
			// 에디터에서 설정한 LootTable의 아이템들을 인벤토리에 추가
			for (const FLRItemSlot& Loot : LootTable)
			{
				if (Loot.ItemData && Loot.Quantity > 0)
				{
					Inventory->AddItem(Loot.ItemData, Loot.Quantity);
					UE_LOG(LogTemp, Log, TEXT("컨테이너: 아이템 획득 - %s x %d"), *Loot.ItemData->GetName(), Loot.Quantity);
				}
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("컨테이너: 수색 완료 및 아이템 획득 성공!"));
	}
}

float ALRContainer::GetInteractionDuration() const
{
	// 한 번 수색된 상자는 0을 반환해 반응 없게 만듦
	return bSearched ? 0.f : SearchDuration;
}