#include "UI/LRContextMenuWidget.h"
#include "Components/LRInventoryGridComponent.h"
#include "Items/LRItemDataAsset.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"

void ULRContextMenuWidget::InitMenu(
	const FLRGridItem& InItem, int32 InItemID, ULRInventoryGridComponent* InGrid)
{
	CachedItemID = InItemID;
	CachedGrid   = InGrid;
	CachedActions.Reset();

	if (!MenuBox) return;
	MenuBox->ClearChildren();

	// [사용하기] — Consumable 아이템만
	if (InItem.ItemData && InItem.ItemData->ItemType == ELRItemType::Consumable)
	{
		CachedActions.Add([this]()
		{
			if (CachedGrid) CachedGrid->UseItem(CachedItemID);
		});
		AddItem(FText::FromString(TEXT("사용하기")));
	}

	// 백드롭 버튼 — 메뉴 외부 클릭 시 닫기
	if (BackdropButton)
		BackdropButton->OnClicked.AddDynamic(this, &ULRContextMenuWidget::OnBackdropClicked);

	// [정보] — 모든 아이템
	const FText InfoLabel = InItem.ItemData
		? FText::Format(
			FText::FromString(TEXT("{0}\n{1}")),
			InItem.ItemData->ItemName,
			InItem.ItemData->ItemDescription)
		: FText::GetEmpty();

	CachedActions.Add([InfoLabel]()
	{
		// 툴팁이 호버 시 이미 표시됨 — 추가 동작 없음 (stub)
	});
	AddItem(FText::FromString(TEXT("정보")));
}

void ULRContextMenuWidget::AddItem(const FText& Label)
{
	if (!MenuBox) return;

	const int32 Idx = CachedActions.Num() - 1; // 방금 추가된 액션 인덱스

	UButton* Btn = NewObject<UButton>(this);
	{
		FButtonStyle Style = Btn->GetStyle();
		auto MakeBrush = [](FLinearColor Color) {
			FSlateBrush B; B.TintColor = FSlateColor(Color); B.DrawAs = ESlateBrushDrawType::Box; return B;
		};
		Style.Normal  = MakeBrush(FLinearColor(0.f,  0.f,  0.f,  0.f));
		Style.Hovered = MakeBrush(FLinearColor(0.15f, 0.22f, 0.35f, 0.6f));
		Style.Pressed = MakeBrush(FLinearColor(0.08f, 0.13f, 0.22f, 0.8f));
		Style.SetNormalPadding(FMargin(12.f, 6.f));
		Style.SetPressedPadding(FMargin(12.f, 7.f, 12.f, 5.f));
		Btn->SetStyle(Style);
	}

	UTextBlock* Txt = NewObject<UTextBlock>(this);
	Txt->SetText(Label);
	{
		FSlateFontInfo Font = Txt->GetFont();
		Font.Size = 12;
		Txt->SetFont(Font);
		Txt->SetColorAndOpacity(FLinearColor(0.80f, 0.90f, 1.0f, 1.0f));
	}
	Btn->AddChild(Txt);

	// 인덱스별 UFUNCTION 바인딩
	if      (Idx == 0) Btn->OnClicked.AddDynamic(this, &ULRContextMenuWidget::OnBtn0Clicked);
	else if (Idx == 1) Btn->OnClicked.AddDynamic(this, &ULRContextMenuWidget::OnBtn1Clicked);
	else if (Idx == 2) Btn->OnClicked.AddDynamic(this, &ULRContextMenuWidget::OnBtn2Clicked);

	UVerticalBoxSlot* VBSlot = MenuBox->AddChildToVerticalBox(Btn);
	if (VBSlot) VBSlot->SetPadding(FMargin(2.f, 1.f));
}

void ULRContextMenuWidget::PositionNearMouse(FVector2D ViewportPos)
{
	// MenuBox가 Canvas Panel에 직접 있으면 MenuBox를, 아니면 부모(Border 등)를 위치시킴
	UWidget* Target = MenuBox;
	if (!Cast<UCanvasPanelSlot>(MenuBox->Slot) && MenuBox->GetParent())
		Target = MenuBox->GetParent();

	if (UCanvasPanelSlot* CS = Cast<UCanvasPanelSlot>(Target->Slot))
	{
		CS->SetPosition(ViewportPos);
		CS->SetAutoSize(true);
	}
}

void ULRContextMenuWidget::ExecuteAndClose(int32 Index)
{
	if (CachedActions.IsValidIndex(Index))
		CachedActions[Index]();

	OnMenuClosed.Broadcast();
	RemoveFromParent();
}
