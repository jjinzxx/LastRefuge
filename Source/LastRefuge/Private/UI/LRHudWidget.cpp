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

	// Noise 바 폴링
	if (StatusComponent)
	{
		const float NoisePercent = FMath::Clamp(StatusComponent->GetNoiseRadius() / 1200.f, 0.f, 1.f);
		PB_Noise->SetPercent(NoisePercent);
	}

	// 점 애니메이션 (수색중. / 수색중.. / 수색중...)
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


void ULRHudWidget::OnSearchProgressChanged(float Progress)
{
	// 진행률 바는 사용하지 않음 — 텍스트 애니메이션으로 대체
}

void ULRHudWidget::OnSearchStarted(FText InProgressBaseText)
{
	ProgressBaseText = InProgressBaseText.ToString();
	bIsAnimating = true;
	DotTimer = 0.f;
	DotState = 0;

	// 애니메이션 첫 프레임 즉시 표시
	TB_Prompt->SetText(FText::FromString(ProgressBaseText + TEXT(".")));
	TB_Prompt->SetVisibility(ESlateVisibility::Visible);

	// 완료 타이머 초기화 (이전 완료 메시지 도중 새 수색 시작 시)
	GetWorld()->GetTimerManager().ClearTimer(CompletionHideTimer);
}

void ULRHudWidget::OnSearchEnded(bool bCompleted, FText StatusText)
{
	bIsAnimating = false;
	GetWorld()->GetTimerManager().ClearTimer(CompletionHideTimer);

	FText DisplayText = StatusText;

	TB_Prompt->SetText(DisplayText);
	TB_Prompt->SetVisibility(ESlateVisibility::Visible);

	// 2초 후 자동으로 숨김
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
	// 수색 중에는 애니메이션 텍스트가 우선
	if (bIsAnimating) return;

	if (Prompt.IsEmpty())
	{
		// 완료 메시지 표시 중이면 건드리지 않음
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
