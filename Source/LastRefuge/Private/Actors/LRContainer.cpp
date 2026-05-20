#include "Actors/LRContainer.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Character/LRCharacter.h"
#include "Components/LRInventoryGridComponent.h"
#include "Items/LRItemDataAsset.h"
#include "Items/LRInventoryStructs.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"

ALRContainer::ALRContainer()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	ContainerGrid = CreateDefaultSubobject<ULRInventoryGridComponent>(TEXT("ContainerGrid"));

	SearchLoopAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("SearchLoopAudio"));
	SearchLoopAudioComp->SetupAttachment(RootComponent);
	SearchLoopAudioComp->bAutoActivate = false;
}

bool ALRContainer::CanInteract(ALRCharacter* Player) const
{
	// 이미 수색된 컨테이너나 일반/경보 타입은 항상 접근 가능
	if (ContainerType != ELRContainerType::Locked || bSearched)
		return true;

	if (!Player || RequiredKeyCardID.IsEmpty())
		return false;

	// 플레이어 인벤토리에서 해당 ItemID를 가진 키카드 탐색
	for (const auto& [ID, Item] : Player->GetInventoryGrid()->GetItems())
		if (Item.ItemData && Item.ItemData->ItemID == RequiredKeyCardID)
			return true;

	return false;
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

	if (SFX_SearchLoop && SearchLoopAudioComp)
	{
		SearchLoopAudioComp->SetSound(SFX_SearchLoop);
		SearchLoopAudioComp->Play();
	}

	UE_LOG(LogTemp, Log, TEXT("컨테이너: 수색 시작"));
}

void ALRContainer::EndInteract(ALRCharacter* Player)
{
	// 취소 또는 완료 모두 루프음 즉시 정지
	if (SearchLoopAudioComp && SearchLoopAudioComp->IsPlaying())
		SearchLoopAudioComp->Stop();

	if (Player == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("컨테이너: 수색 취소"));
		return;
	}

	if (SFX_SearchComplete)
		UGameplayStatics::PlaySoundAtLocation(this, SFX_SearchComplete, GetActorLocation());

	// 경보 컨테이너: 수색 완료 시 주변 AI에 청각 자극 송출
	if (ContainerType == ELRContainerType::Alarmed)
	{
		UAISense_Hearing::ReportNoiseEvent(
			GetWorld(),
			GetActorLocation(),
			/*Loudness=*/ 1.0f,
			Player,
			/*MaxRange=*/ 0.f,   // 0 = AISense 기본 범위 사용
			FName("Alarm"));
		UE_LOG(LogTemp, Warning, TEXT("[Container] 경보 발동! AI에 청각 자극 송출"));
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
	if (!bSearched)
	{
		switch (ContainerType)
		{
		case ELRContainerType::Locked:
			return FText::FromString(TEXT("[E] 열기  🔒 보안 키카드 필요"));
		case ELRContainerType::Alarmed:
			return FText::FromString(TEXT("[E] 열기  ⚠ 경보 장치"));
		default:
			return FText::FromString(TEXT("[E] 열기"));
		}
	}
	return FText::FromString(TEXT("[E] 다시 열기"));
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
