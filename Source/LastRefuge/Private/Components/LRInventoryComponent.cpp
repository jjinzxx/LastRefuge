// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/LRInventoryComponent.h"

ULRInventoryComponent::ULRInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // 인벤토리는 틱이 필요 없습니다.
}

bool ULRInventoryComponent::AddItem(ULRItemDataAsset* Item, int32 Amount)
{
	if (!Item || Amount <= 0) return false;

	// 1. 기존 슬롯 중 중첩 가능한 슬롯 확인
	for (FLRItemSlot& Slot : InventorySlots)
	{
		if (Slot.ItemData == Item && Slot.Quantity < Item->MaxStackSize)
		{
			int32 AddCount = FMath::Min(Amount, Item->MaxStackSize - Slot.Quantity);
			Slot.Quantity += AddCount;
			Amount -= AddCount;

			if (Amount <= 0)
			{
				OnInventoryUpdated.Broadcast();
				return true;
			}
		}
	}

	// 2. 남은 수량이 있다면 빈 슬롯에 추가
	while (Amount > 0 && InventorySlots.Num() < InventorySlotsMax)
	{
		FLRItemSlot NewSlot;
		NewSlot.ItemData = Item;
		int32 AddCount = FMath::Min(Amount, Item->MaxStackSize);
		NewSlot.Quantity = AddCount;
        
		InventorySlots.Add(NewSlot);
		Amount -= AddCount;
	}

	OnInventoryUpdated.Broadcast();
	return Amount <= 0; // 모두 담았다면 true, 공간 부족으로 남았다면 false
}