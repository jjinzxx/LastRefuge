#include "UI/LRTooltipWidget.h"
#include "Components/TextBlock.h"
#include "Items/LRItemDataAsset.h"

void ULRTooltipWidget::SetItem(const FLRGridItem& Item)
{
	if (!Item.ItemData) return;

	if (NameText)
		NameText->SetText(Item.ItemData->ItemName);

	if (TypeText)
	{
		FText TypeStr;
		switch (Item.ItemData->ItemType)
		{
		case ELRItemType::Resource:   TypeStr = FText::FromString(TEXT("자원"));       break;
		case ELRItemType::Consumable: TypeStr = FText::FromString(TEXT("소비품"));     break;
		case ELRItemType::Equipment:  TypeStr = FText::FromString(TEXT("장비"));       break;
		case ELRItemType::KeyItem:    TypeStr = FText::FromString(TEXT("중요 아이템")); break;
		default:                      TypeStr = FText::GetEmpty();                     break;
		}
		TypeText->SetText(TypeStr);
	}

	if (SizeText)
	{
		SizeText->SetText(FText::FromString(
			FString::Printf(TEXT("%d × %d  |  %.1f kg"),
				Item.ItemData->GridWidth,
				Item.ItemData->GridHeight,
				Item.ItemData->Weight)));
	}

	if (DescriptionText)
		DescriptionText->SetText(Item.ItemData->ItemDescription);
}
