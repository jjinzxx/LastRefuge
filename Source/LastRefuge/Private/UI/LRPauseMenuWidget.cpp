#include "UI/LRPauseMenuWidget.h"
#include "Character/LRCharacter.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateTypes.h"

void ULRPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyButtonStyle(Btn_Resume);
	ApplyButtonStyle(Btn_MainMenu);
	ApplyButtonStyle(Btn_Quit);

	Btn_Resume->OnClicked.AddDynamic(this, &ULRPauseMenuWidget::OnResumeClicked);
	Btn_MainMenu->OnClicked.AddDynamic(this, &ULRPauseMenuWidget::OnMainMenuClicked);
	Btn_Quit->OnClicked.AddDynamic(this, &ULRPauseMenuWidget::OnQuitClicked);
}

int32 ULRPauseMenuWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D ScreenSz = AllottedGeometry.GetLocalSize();
	const FVector2D PanelSz(PanelWidth, PanelHeight);
	const FVector2D O((ScreenSz.X - PanelWidth) * 0.5f, (ScreenSz.Y - PanelHeight) * 0.5f);
	const float R = O.X + PanelWidth;
	const float B = O.Y + PanelHeight;

	// 반투명 어두운 배경
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(PanelSz, FSlateLayoutTransform(O)),
		FCoreStyle::Get().GetBrush("WhiteBrush"),
		ESlateDrawEffect::None, PanelBgColor);

	// 테두리 4변
	auto Line = [&](FVector2D A, FVector2D B2)
	{
		TArray<FVector2D> Pts = { A, B2 };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(),
			Pts, ESlateDrawEffect::None, BorderColor, true, BorderThickness);
	};
	Line({ O.X, O.Y }, { R,    O.Y });
	Line({ R,    O.Y }, { R,    B   });
	Line({ R,    B   }, { O.X, B   });
	Line({ O.X, B   }, { O.X, O.Y });

	// 네 모서리 L자 강조 (굵게)
	const float T2 = BorderThickness * 2.2f;
	const float C  = CornerSize;
	auto Corner = [&](FVector2D A, FVector2D B2)
	{
		TArray<FVector2D> Pts = { A, B2 };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2,
			AllottedGeometry.ToPaintGeometry(),
			Pts, ESlateDrawEffect::None, BorderColor, true, T2);
	};
	Corner({ O.X, O.Y + C }, { O.X, O.Y }); Corner({ O.X, O.Y }, { O.X + C, O.Y });
	Corner({ R - C, O.Y  }, { R,   O.Y }); Corner({ R,   O.Y }, { R,   O.Y + C });
	Corner({ O.X, B - C  }, { O.X, B   }); Corner({ O.X, B   }, { O.X + C, B   });
	Corner({ R - C, B    }, { R,   B   }); Corner({ R,   B   }, { R,   B - C   });

	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements,
		LayerId + 3, InWidgetStyle, bParentEnabled);
}

void ULRPauseMenuWidget::ApplyButtonStyle(UButton* Btn) const
{
	if (!Btn) return;

	FSlateBrush None;
	None.DrawAs = ESlateBrushDrawType::NoDrawType;

	FSlateBrush Hover;
	Hover.DrawAs = ESlateBrushDrawType::Box;
	Hover.TintColor = BtnHoverColor;

	FSlateBrush Pressed;
	Pressed.DrawAs = ESlateBrushDrawType::Box;
	Pressed.TintColor = FLinearColor(BtnHoverColor.R * 1.0f, BtnHoverColor.G * 1.0f,
		BtnHoverColor.B * 1.0f, 0.5f);

	FButtonStyle Style = Btn->GetStyle();
	Style.SetNormal(None);
	Style.SetHovered(Hover);
	Style.SetPressed(Pressed);
	Style.SetDisabled(None);
	Style.NormalPadding  = FMargin(16.f, 10.f);
	Style.PressedPadding = FMargin(16.f, 11.f, 16.f, 9.f);
	Btn->SetStyle(Style);
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
