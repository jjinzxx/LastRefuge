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

	// 메인 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Start;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Settings;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Quit;

	// 설정 패널 (숨김/표시 토글)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Panel_Settings;

	// 설정 슬라이더
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SL_Sensitivity;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SL_Volume;

	// 시작할 레벨 이름 (에디터에서 지정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FName StartLevelName = FName("L_Base");

private:
	UFUNCTION() void OnStartClicked();
	UFUNCTION() void OnSettingsClicked();
	UFUNCTION() void OnQuitClicked();
	UFUNCTION() void OnSensitivityChanged(float Value);
	UFUNCTION() void OnVolumeChanged(float Value);
};
