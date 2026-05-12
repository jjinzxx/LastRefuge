#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LRHudWidget.generated.h"

class UProgressBar;
class UTextBlock;
class ULRStatusComponent;
class ALRCharacter;

UCLASS()
class LASTREFUGE_API ULRHudWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// --- 위젯 바인딩 (이름이 Blueprint 위젯 이름과 정확히 일치해야 함) ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_HP;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Stamina;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Noise;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_Prompt;

private:
	UPROPERTY()
	TObjectPtr<ALRCharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<ULRStatusComponent> StatusComponent;

	// --- 점 애니메이션 상태 ---
	FString ProgressBaseText;
	bool bIsAnimating = false;
	float DotTimer = 0.f;
	int32 DotState = 0;  // 0=".', 1="..", 2="..."

	FTimerHandle CompletionHideTimer;

	void HidePrompt();

	// --- 델리게이트 핸들러 ---
	UFUNCTION()
	void OnHealthChanged(float NewHealth, float MaxHealth);

	UFUNCTION()
	void OnStaminaChanged(float NewStamina, float MaxStamina);

	UFUNCTION()
	void OnSearchProgressChanged(float Progress);

	UFUNCTION()
	void OnSearchStarted(FText ProgressBaseText);

	UFUNCTION()
	void OnSearchEnded(bool bCompleted, FText StatusText);

	UFUNCTION()
	void OnInteractionPromptChanged(FText Prompt);
};
