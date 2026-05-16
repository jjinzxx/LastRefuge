#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LRHudWidget.generated.h"

class UTextBlock;
class UProgressBar;
class ULRStatusComponent;
class ALRCharacter;
class ULRToolbarSlotWidget;
class ULRItemDataAsset;

UCLASS()
class LASTREFUGE_API ULRHudWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// --- 상태바 ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_HP;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Sta;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TB_Prompt;

	// --- 툴바 슬롯 (WBP_LRHud에 ToolbarSlot1~4 이름으로 배치) ---
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULRToolbarSlotWidget> ToolbarSlot1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULRToolbarSlotWidget> ToolbarSlot2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULRToolbarSlotWidget> ToolbarSlot3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<ULRToolbarSlotWidget> ToolbarSlot4;

private:
	UPROPERTY()
	TObjectPtr<ALRCharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<ULRStatusComponent> StatusComponent;

	FString ProgressBaseText;
	bool bIsAnimating = false;

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

	UFUNCTION()
	void OnToolbarSlotChanged(int32 SlotIndex, ULRItemDataAsset* ItemData, int32 Quantity);

	// OnGridChanged 구독 — 인벤토리 변경 시 툴바 수량 자동 갱신
	void RefreshAllToolbarSlots();

	ULRToolbarSlotWidget* GetToolbarSlotWidget(int32 Index) const;
};
