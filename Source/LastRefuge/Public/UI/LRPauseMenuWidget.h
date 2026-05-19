#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LRPauseMenuWidget.generated.h"

class UButton;

UCLASS()
class LASTREFUGE_API ULRPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Resume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_MainMenu;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Quit;

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	FLinearColor PanelBgColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	FLinearColor BorderColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.80f);

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	FLinearColor BtnHoverColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.65f);

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	float PanelWidth = 320.f;

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	float PanelHeight = 260.f;

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	float BorderThickness = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	float CornerSize = 14.f;

private:
	UFUNCTION() void OnResumeClicked();
	UFUNCTION() void OnMainMenuClicked();
	UFUNCTION() void OnQuitClicked();

	void ApplyButtonStyle(UButton* Btn) const;
};
