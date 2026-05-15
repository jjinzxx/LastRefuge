#include "UI/LRPauseMenuWidget.h"
#include "Character/LRCharacter.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void ULRPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Btn_Resume->OnClicked.AddDynamic(this, &ULRPauseMenuWidget::OnResumeClicked);
	Btn_MainMenu->OnClicked.AddDynamic(this, &ULRPauseMenuWidget::OnMainMenuClicked);
	Btn_Quit->OnClicked.AddDynamic(this, &ULRPauseMenuWidget::OnQuitClicked);
}

void ULRPauseMenuWidget::OnResumeClicked()
{
	if (ALRCharacter* Char = Cast<ALRCharacter>(GetOwningPlayerPawn()))
		Char->TogglePauseMenu();
}

void ULRPauseMenuWidget::OnMainMenuClicked()
{
	UGameplayStatics::OpenLevel(this, FName("L_MainMenu"));
}

void ULRPauseMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
