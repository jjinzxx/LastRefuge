#include "Actors/LRDoor.h"
#include "Character/LRCharacter.h"
#include "Components/LRInventoryGridComponent.h"
#include "Actors/LRStorage.h"
#include "LRGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"

ALRDoor::ALRDoor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	PromptText = FText::FromString(TEXT("[E] 이동하기"));
}

void ALRDoor::BeginInteract(ALRCharacter* Player)
{
}

void ALRDoor::EndInteract(ALRCharacter* Player)
{
	if (!Player || TargetLevel.IsNone())
	{
		UE_LOG(LogTemp, Error, TEXT("[Door] 이동 실패 — Player: %s, TargetLevel: %s"),
			Player ? TEXT("OK") : TEXT("NULL"),
			TargetLevel.IsNone() ? TEXT("미설정") : *TargetLevel.ToString());
		return;
	}

	ULRGameInstance* GI = Cast<ULRGameInstance>(GetGameInstance());
	if (!GI)
	{
		UE_LOG(LogTemp, Error, TEXT("[Door] 이동 실패 — GameInstance가 LRGameInstance가 아님."));
		return;
	}

	// 인벤토리 직렬화
	if (ULRInventoryGridComponent* InvGrid = Player->GetInventoryGrid())
	{
		GI->PersistentInventory.Empty();
		for (const auto& [ID, Item] : InvGrid->GetItems())
		{
			if (!Item.IsEmpty())
				GI->PersistentInventory.Add(Item);
		}
	}

	// 툴바 직렬화
	GI->PersistentToolbarItems = Player->GetToolbarItems();

	// 보관함 직렬화 (기지 출구에만 bSaveStorage = true)
	if (bSaveStorage)
	{
		GI->PersistentStorageItems.Empty();
		for (TActorIterator<ALRStorage> It(GetWorld()); It; ++It)
		{
			if (ULRInventoryGridComponent* SGrid = It->GetStorageGrid())
			{
				for (const auto& [ID, Item] : SGrid->GetItems())
				{
					if (!Item.IsEmpty())
						GI->PersistentStorageItems.Add(Item);
				}
			}
			break; // 맵에 보관함이 1개라고 가정
		}
	}

	GI->bHasTravelData = true;
	UE_LOG(LogTemp, Warning, TEXT("[Door] 레벨 이동 → %s"), *TargetLevel.ToString());
	UGameplayStatics::OpenLevel(this, TargetLevel);
}

float ALRDoor::GetInteractionDuration() const { return InteractionDuration; }
FText ALRDoor::GetInteractionPrompt()   const { return PromptText; }
FText ALRDoor::GetProgressText()        const { return FText::FromString(TEXT("이동중")); }
FText ALRDoor::GetStartText()           const { return FText::FromString(TEXT("이동 시작...")); }
FText ALRDoor::GetCancelText()          const { return FText::FromString(TEXT("이동이 취소되었습니다.")); }
FText ALRDoor::GetCompleteText()        const { return FText::FromString(TEXT("이동완료")); }
