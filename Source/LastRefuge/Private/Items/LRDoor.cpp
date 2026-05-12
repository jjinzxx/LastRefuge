#include "Items/LRDoor.h"
#include "Character/LRCharacter.h"
#include "Components/LRInventoryComponent.h"
#include "Items/LRStorage.h"
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
		UE_LOG(LogTemp, Error, TEXT("[Door] 이동 실패 — GameInstance가 LRGameInstance가 아님. 프로젝트 세팅 확인 필요."));
		return;
	}

	// 인벤토리 저장
	ULRInventoryComponent* Inv = Player->FindComponentByClass<ULRInventoryComponent>();
	if (Inv)
	{
		GI->PersistentInventory.Empty();
		for (const FLRItemSlot& Slot : Inv->GetInventorySlots())
		{
			if (!Slot.IsEmpty())
				GI->PersistentInventory.Add(Slot);
		}
	}

	// 보관함 저장 (기지 출구에만 bSaveStorage = true)
	if (bSaveStorage)
	{
		GI->PersistentStorageItems.Empty();
		for (TActorIterator<ALRStorage> It(GetWorld()); It; ++It)
		{
			for (const FLRItemSlot& Slot : It->GetStoredItems())
			{
				if (!Slot.IsEmpty())
					GI->PersistentStorageItems.Add(Slot);
			}
			break;
		}
	}

	GI->bHasTravelData = true;
	UE_LOG(LogTemp, Warning, TEXT("[Door] 레벨 이동 → %s"), *TargetLevel.ToString());
	UGameplayStatics::OpenLevel(this, TargetLevel);
}

float ALRDoor::GetInteractionDuration() const
{
	return InteractionDuration;
}

FText ALRDoor::GetInteractionPrompt() const
{
	return PromptText;
}

FText ALRDoor::GetProgressText() const
{
	return FText::FromString(TEXT("이동중"));
}

FText ALRDoor::GetStartText() const
{
	return FText::FromString(TEXT("이동 시작..."));
}

FText ALRDoor::GetCancelText() const
{
	return FText::FromString(TEXT("이동이 취소되었습니다."));
}

FText ALRDoor::GetCompleteText() const
{
	return FText::FromString(TEXT("이동완료"));
}
