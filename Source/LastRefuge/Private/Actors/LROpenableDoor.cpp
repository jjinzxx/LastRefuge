#include "Actors/LROpenableDoor.h"
#include "Character/LRCharacter.h"
#include "Components/LRInventoryGridComponent.h"
#include "Items/LRItemDataAsset.h"
#include "Components/StaticMeshComponent.h"
#include "LRGameInstance.h"

ALROpenableDoor::ALROpenableDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false; // 움직일 때만 Tick 활성화

	// 경첩 피벗을 루트로 설정 — 액터 회전 = 문 회전축
	HingeRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HingeRoot"));
	RootComponent = HingeRoot;

	// 문짝 메시를 HingeRoot에 Y축 오프셋으로 부착
	// HingeOffsetY는 BeginPlay에서 적용 (EditAnywhere 값 반영 위해)
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(HingeRoot);
}

void ALROpenableDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// 에디터에서도 실시간으로 확인 가능하도록 메시 오프셋 적용
	// → 오렌지 화살표(액터 원점)가 경첩 위치. 메시가 Y축 방향으로 뻗어나감
	if (MeshComponent)
		MeshComponent->SetRelativeLocation(FVector(0.f, HingeOffsetY, 0.f));
}

void ALROpenableDoor::BeginPlay()
{
	Super::BeginPlay();

	// 에디터에서 배치된 초기 회전을 "닫힌 상태" 기준으로 캐싱
	ClosedYaw = GetActorRotation().Yaw;
	TargetYaw = ClosedYaw;
}

// ──────────────────────────────────────────────────────────────
// CanInteract — 키카드 체크
// ──────────────────────────────────────────────────────────────
bool ALROpenableDoor::CanInteract(ALRCharacter* Player) const
{
	// 열려 있으면 키카드 없이도 닫을 수 있음
	if (bIsOpen) return true;
	if (!bRequiresKeyCard || RequiredKeyCardID.IsEmpty()) return true;
	if (!Player) return false;

	for (const auto& [ID, Item] : Player->GetInventoryGrid()->GetItems())
		if (Item.ItemData && Item.ItemData->ItemID == RequiredKeyCardID)
			return true;

	return false;
}

// ──────────────────────────────────────────────────────────────
// BeginInteract — 즉시 토글 (EndInteract도 바로 호출됨)
// ──────────────────────────────────────────────────────────────
void ALROpenableDoor::BeginInteract(ALRCharacter* Player)
{
	bIsOpen = !bIsOpen;
	TargetYaw = bIsOpen ? ClosedYaw + OpenAngle : ClosedYaw;

	// Tick 활성화 → 보간 완료 후 비활성화
	SetActorTickEnabled(true);
}

void ALROpenableDoor::EndInteract(ALRCharacter* Player)
{
	// 즉시 완료 (Duration = 0) — 별도 처리 없음
}

// ──────────────────────────────────────────────────────────────
// Tick — 목표 Yaw까지 부드럽게 회전
// ──────────────────────────────────────────────────────────────
void ALROpenableDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FRotator Current = GetActorRotation();
	const float NewYaw = FMath::FInterpTo(Current.Yaw, TargetYaw, DeltaTime, OpenSpeed);
	SetActorRotation(FRotator(Current.Pitch, NewYaw, Current.Roll));

	// 목표에 충분히 가까워지면 Tick 비활성화
	if (FMath::IsNearlyEqual(NewYaw, TargetYaw, 0.1f))
	{
		SetActorRotation(FRotator(Current.Pitch, TargetYaw, Current.Roll));
		SetActorTickEnabled(false);
	}
}

// ──────────────────────────────────────────────────────────────
// GetInteractionPrompt
// ──────────────────────────────────────────────────────────────
FText ALROpenableDoor::GetInteractionPrompt() const
{
	if (!bIsOpen && bRequiresKeyCard && !RequiredKeyCardID.IsEmpty())
	{
		// GI ItemRegistry에서 키카드 이름 조회
		FString KeyName = RequiredKeyCardID;
		if (const ULRGameInstance* GI = Cast<ULRGameInstance>(GetGameInstance()))
		{
			if (const TObjectPtr<ULRItemDataAsset>* Found = GI->ItemRegistry.Find(RequiredKeyCardID))
				if (*Found)
					KeyName = (*Found)->ItemName.ToString();
		}
		return FText::FromString(FString::Printf(TEXT("[E] 열기  🔒 %s 필요"), *KeyName));
	}

	return bIsOpen
		? FText::FromString(TEXT("[E] 닫기"))
		: FText::FromString(TEXT("[E] 열기"));
}
