#include "UI/LRHudWidget.h"
#include "Character/LRCharacter.h"
#include "Components/LRStatusComponent.h"
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

		PB_HP->SetPercent(StatusComponent->GetHealthPercent());
		PB_Stamina->SetPercent(StatusComponent->GetStaminaPercent());
	}

	OwnerCharacter->OnSearchProgressChanged.AddDynamic(this, &ULRHudWidget::OnSearchProgressChanged);
	OwnerCharacter->OnSearchStarted.AddDynamic(this, &ULRHudWidget::OnSearchStarted);
	OwnerCharacter->OnSearchEnded.AddDynamic(this, &ULRHudWidget::OnSearchEnded);
	OwnerCharacter->OnInteractionPromptChanged.AddDynamic(this, &ULRHudWidget::OnInteractionPromptChanged);

	TB_Prompt->SetVisibility(ESlateVisibility::Hidden);
}

void ULRHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// --- 점 애니메이션 (수색중. / 수색중.. / 수색중...) ---
	if (bIsAnimating)
	{
		DotTimer += InDeltaTime;
		if (DotTimer >= 0.4f)
		{
			DotTimer = 0.f;
			DotState = (DotState + 1) % 3;

			FString Dots;
			for (int32 i = 0; i <= DotState; i++) Dots += TEXT(".");

			TB_Prompt->SetText(FText::FromString(ProgressBaseText + Dots));
		}
	}
}

// --- 체력 / 스테미나 ---

void ULRHudWidget::OnHealthChanged(float NewHealth, float MaxHealth)
{
	PB_HP->SetPercent(NewHealth / MaxHealth);
}

void ULRHudWidget::OnStaminaChanged(float NewStamina, float MaxStamina)
{
	PB_Stamina->SetPercent(NewStamina / MaxStamina);
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
	DotTimer = 0.f;
	DotState = 0;

	TB_Prompt->SetText(FText::FromString(ProgressBaseText + TEXT(".")));
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
		CompletionHideTimer, this, &ULRHudWidget::HidePrompt, 2.f, false);
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
