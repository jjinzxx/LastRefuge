#include "UI/LRHudWidget.h"
#include "Character/LRCharacter.h"
#include "Components/LRStatusComponent.h"
#include "Components/LRInventoryGridComponent.h"
#include "UI/LRToolbarSlotWidget.h"
#include "Items/LRItemDataAsset.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void ULRHudWidget::NativeConstruct()
{
	Super::NativeConstruct();

	OwnerCharacter = Cast<ALRCharacter>(GetOwningPlayerPawn());
	if (!OwnerCharacter) return;

	StatusComponent = OwnerCharacter->FindComponentByClass<ULRStatusComponent>();

	if (StatusComponent)
	{
		StatusComponent->OnHealthChanged.AddDynamic(this, &ULRHudWidget::OnHealthChanged);
		StatusComponent->OnStaminaChanged.AddDynamic(this, &ULRHudWidget::OnStaminaChanged);

		OnHealthChanged(StatusComponent->GetHealth(), StatusComponent->GetMaxHealth());
		OnStaminaChanged(StatusComponent->GetStamina(), StatusComponent->GetMaxStamina());
	}

	OwnerCharacter->OnSearchProgressChanged.AddDynamic(this, &ULRHudWidget::OnSearchProgressChanged);
	OwnerCharacter->OnSearchStarted.AddDynamic(this, &ULRHudWidget::OnSearchStarted);
	OwnerCharacter->OnSearchEnded.AddDynamic(this, &ULRHudWidget::OnSearchEnded);
	OwnerCharacter->OnInteractionPromptChanged.AddDynamic(this, &ULRHudWidget::OnInteractionPromptChanged);
	OwnerCharacter->OnToolbarSlotChanged.AddDynamic(this, &ULRHudWidget::OnToolbarSlotChanged);

	// 툴바 슬롯 초기화
	for (int32 i = 0; i < ALRCharacter::ToolbarSize; ++i)
	{
		if (ULRToolbarSlotWidget* ToolbarSlot = GetToolbarSlotWidget(i))
			ToolbarSlot->InitSlot(OwnerCharacter, i);
	}

	// (툴바는 OnToolbarSlotChanged로만 갱신 — OnGridChanged 구독 불필요)

	// 초기 빈 슬롯 상태 표시
	RefreshAllToolbarSlots();

	TB_Prompt->SetVisibility(ESlateVisibility::Hidden);
}

void ULRHudWidget::NativeDestruct()
{
	// NativeConstruct에서 AddDynamic한 델리게이트 명시적 해제
	// UE가 UObject 소멸 시 자동 정리하지만, RemoveFromParent 후 Pawn 이벤트가
	// 발생해 이미 Viewport에서 제거된 HUD가 반응하는 것을 방지
	if (StatusComponent)
	{
		StatusComponent->OnHealthChanged.RemoveDynamic(this, &ULRHudWidget::OnHealthChanged);
		StatusComponent->OnStaminaChanged.RemoveDynamic(this, &ULRHudWidget::OnStaminaChanged);
	}
	if (OwnerCharacter)
	{
		OwnerCharacter->OnSearchProgressChanged.RemoveDynamic(this, &ULRHudWidget::OnSearchProgressChanged);
		OwnerCharacter->OnSearchStarted.RemoveDynamic(this, &ULRHudWidget::OnSearchStarted);
		OwnerCharacter->OnSearchEnded.RemoveDynamic(this, &ULRHudWidget::OnSearchEnded);
		OwnerCharacter->OnInteractionPromptChanged.RemoveDynamic(this, &ULRHudWidget::OnInteractionPromptChanged);
		OwnerCharacter->OnToolbarSlotChanged.RemoveDynamic(this, &ULRHudWidget::OnToolbarSlotChanged);
	}
	Super::NativeDestruct();
}

void ULRHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// --- 수색 진행 시간 표시 ---
	if (bIsAnimating && OwnerCharacter)
	{
		const float Current = OwnerCharacter->GetCurrentSearchTime();
		const float Total   = OwnerCharacter->GetSearchDuration();

		const FString TimeText = FString::Printf(
			TEXT("%s (%.1f / %.1f sec)"),
			*ProgressBaseText, Current, Total);

		TB_Prompt->SetText(FText::FromString(TimeText));
	}
}

// --- 체력 / 스테미나 ---

void ULRHudWidget::OnHealthChanged(float NewHealth, float MaxHealth)
{
	if (PB_HP) PB_HP->SetPercent(MaxHealth > 0.f ? NewHealth / MaxHealth : 0.f);
}

void ULRHudWidget::OnStaminaChanged(float NewStamina, float MaxStamina)
{
	if (PB_Sta) PB_Sta->SetPercent(MaxStamina > 0.f ? NewStamina / MaxStamina : 0.f);
}

// --- 수색 게이지 ---

void ULRHudWidget::OnSearchProgressChanged(float Progress)
{
	// 텍스트 애니메이션으로 대체 — 사용하지 않음
}

void ULRHudWidget::OnSearchStarted(FText InProgressBaseText)
{
	ProgressBaseText = InProgressBaseText.ToString();
	bIsAnimating = true;

	TB_Prompt->SetText(FText::FromString(ProgressBaseText + TEXT(" (0.0 / 0.0 sec)")));
	TB_Prompt->SetVisibility(ESlateVisibility::Visible);

	GetWorld()->GetTimerManager().ClearTimer(CompletionHideTimer);
}

void ULRHudWidget::OnSearchEnded(bool bCompleted, FText StatusText)
{
	bIsAnimating = false;
	GetWorld()->GetTimerManager().ClearTimer(CompletionHideTimer);

	TB_Prompt->SetText(StatusText);
	TB_Prompt->SetVisibility(ESlateVisibility::Visible);

	GetWorld()->GetTimerManager().SetTimer(
		CompletionHideTimer, this, &ULRHudWidget::HidePrompt, 1.f, false);
}

void ULRHudWidget::HidePrompt()
{
	TB_Prompt->SetVisibility(ESlateVisibility::Hidden);
}

// --- 상호작용 프롬프트 ---

void ULRHudWidget::OnInteractionPromptChanged(FText Prompt)
{
	if (bIsAnimating) return;

	if (Prompt.IsEmpty())
	{
		if (!GetWorld()->GetTimerManager().IsTimerActive(CompletionHideTimer))
		{
			TB_Prompt->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	else
	{
		TB_Prompt->SetText(Prompt);
		TB_Prompt->SetVisibility(ESlateVisibility::Visible);
	}
}

// --- 툴바 ---

ULRToolbarSlotWidget* ULRHudWidget::GetToolbarSlotWidget(int32 Index) const
{
	switch (Index)
	{
	case 0: return ToolbarSlot1;
	case 1: return ToolbarSlot2;
	case 2: return ToolbarSlot3;
	case 3: return ToolbarSlot4;
	default: return nullptr;
	}
}

void ULRHudWidget::OnToolbarSlotChanged(int32 SlotIndex, ULRItemDataAsset* ItemData, int32 Quantity)
{
	if (ULRToolbarSlotWidget* ToolbarSlot = GetToolbarSlotWidget(SlotIndex))
		ToolbarSlot->RefreshSlot(ItemData, Quantity);
}

void ULRHudWidget::RefreshAllToolbarSlots()
{
	if (!OwnerCharacter) return;
	const TArray<FLRGridItem>& Items = OwnerCharacter->GetToolbarItems();

	for (int32 i = 0; i < ALRCharacter::ToolbarSize; ++i)
	{
		ULRToolbarSlotWidget* SlotWidget = GetToolbarSlotWidget(i);
		if (!SlotWidget) continue;

		if (!Items.IsValidIndex(i) || Items[i].IsEmpty())
			SlotWidget->RefreshSlot(nullptr, 0);
		else
			SlotWidget->RefreshSlot(Items[i].ItemData, Items[i].Quantity);
	}
}
