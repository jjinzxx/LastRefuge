#include "UI/LRMainMenuWidget.h"
#include "LRGameInstance.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/Widget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "AudioDevice.h"
#include "Styling/SlateTypes.h"

void ULRMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyButtonStyle(Btn_Start);
	ApplyButtonStyle(Btn_Settings);
	ApplyButtonStyle(Btn_Quit);

	Btn_Start->OnClicked.AddDynamic(this, &ULRMainMenuWidget::OnStartClicked);
	Btn_Settings->OnClicked.AddDynamic(this, &ULRMainMenuWidget::OnSettingsClicked);
	Btn_Quit->OnClicked.AddDynamic(this, &ULRMainMenuWidget::OnQuitClicked);
	SL_Sensitivity->OnValueChanged.AddDynamic(this, &ULRMainMenuWidget::OnSensitivityChanged);
	SL_Volume->OnValueChanged.AddDynamic(this, &ULRMainMenuWidget::OnVolumeChanged);

	Panel_Settings->SetVisibility(ESlateVisibility::Collapsed);

	// GameInstance에서 저장된 설정값 불러오기
	if (ULRGameInstance* GI = Cast<ULRGameInstance>(GetGameInstance()))
	{
		SL_Sensitivity->SetValue(GI->MouseSensitivity);
		SL_Volume->SetValue(GI->MasterVolume);
	}
}

void ULRMainMenuWidget::OnStartClicked()
{
	UGameplayStatics::OpenLevel(this, StartLevelName);
}

void ULRMainMenuWidget::OnSettingsClicked()
{
	const bool bVisible = Panel_Settings->GetVisibility() == ESlateVisibility::Visible;
	Panel_Settings->SetVisibility(bVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void ULRMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void ULRMainMenuWidget::OnSensitivityChanged(float Value)
{
	if (ULRGameInstance* GI = Cast<ULRGameInstance>(GetGameInstance()))
		GI->MouseSensitivity = Value;
}

void ULRMainMenuWidget::OnVolumeChanged(float Value)
{
	if (ULRGameInstance* GI = Cast<ULRGameInstance>(GetGameInstance()))
		GI->MasterVolume = Value;

	if (UWorld* World = GetWorld())
	{
		if (FAudioDevice* AudioDevice = World->GetAudioDeviceRaw())
			AudioDevice->SetTransientPrimaryVolume(Value);
	}
}

void ULRMainMenuWidget::ApplyButtonStyle(UButton* Btn) const
{
	if (!Btn) return;

	FSlateBrush None;
	None.DrawAs = ESlateBrushDrawType::NoDrawType;

	FSlateBrush Hover;
	Hover.DrawAs = ESlateBrushDrawType::Box;
	Hover.TintColor = BtnHoverColor;

	FSlateBrush Pressed;
	Pressed.DrawAs = ESlateBrushDrawType::Box;
	Pressed.TintColor = FLinearColor(BtnHoverColor.R * 0.55f, BtnHoverColor.G * 0.55f,
		BtnHoverColor.B * 0.55f, 0.85f);

	FButtonStyle Style = Btn->GetStyle();
	Style.SetNormal(None);
	Style.SetHovered(Hover);
	Style.SetPressed(Pressed);
	Style.SetDisabled(None);
	Style.NormalPadding  = FMargin(16.f, 10.f);
	Style.PressedPadding = FMargin(16.f, 11.f, 16.f, 9.f);
	Btn->SetStyle(Style);
}
