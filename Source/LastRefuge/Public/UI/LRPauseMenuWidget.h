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

	// WBP_PauseMenu 에서 반드시 배치
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Resume;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_MainMenu;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Quit;

private:
	UFUNCTION()
	void OnResumeClicked();

	UFUNCTION()
	void OnMainMenuClicked();

	UFUNCTION()
	void OnQuitClicked();
};
