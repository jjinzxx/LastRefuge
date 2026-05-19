#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LRMainMenuWidget.generated.h"

class UButton;
class USlider;
class UWidget;

UCLASS()
class LASTREFUGE_API ULRMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Start;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Settings;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Quit;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Panel_Settings;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SL_Sensitivity;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SL_Volume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FName StartLevelName = FName("L_Base");

	UPROPERTY(EditDefaultsOnly, Category = "LR|Style")
	FLinearColor BtnHoverColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.5f);

private:
	UFUNCTION() void OnStartClicked();
	UFUNCTION() void OnSettingsClicked();
	UFUNCTION() void OnQuitClicked();
	UFUNCTION() void OnSensitivityChanged(float Value);
	UFUNCTION() void OnVolumeChanged(float Value);

	void ApplyButtonStyle(UButton* Btn) const;
};
