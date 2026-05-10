#include "Components/LRInventoryComponent.h"
#include "Items/LRItemDataAsset.h"

ULRInventoryComponent::ULRInventoryComponent()
{
    // Tick이 필요 없으므로 false로 설정하여 성능 최적화
    PrimaryComponentTick.bCanEverTick = false; 
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

    // 2. 빈 슬롯에 추가
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
    return Amount <= 0;
}

bool ULRInventoryComponent::RemoveItem(ULRItemDataAsset* Item, int32 Amount)
{
    if (!Item || Amount <= 0) return false;

    // 뒤에서부터 순회하며 아이템 제거 (배열 삭제 시 안전함)
    for (int32 i = InventorySlots.Num() - 1; i >= 0; --i)
    {
        if (InventorySlots[i].ItemData == Item)
        {
            int32 RemoveCount = FMath::Min(Amount, InventorySlots[i].Quantity);
            InventorySlots[i].Quantity -= RemoveCount;
            Amount -= RemoveCount;

            if (InventorySlots[i].Quantity <= 0)
            {
                InventorySlots.RemoveAt(i);
            }

            if (Amount <= 0)
            {
                OnInventoryUpdated.Broadcast();
                return true;
            }
        }
    }

    OnInventoryUpdated.Broadcast();
    return Amount <= 0;
}