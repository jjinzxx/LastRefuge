#include "Components/LRInventoryComponent.h"
#include "Components/LRStatusComponent.h" 
#include "Character/LRCharacter.h"
#include "Items/LRItemDataAsset.h"

ULRInventoryComponent::ULRInventoryComponent()
{
    // Tick이 필요 없으므로 false로 설정하여 성능 최적화
    PrimaryComponentTick.bCanEverTick = false; 
}

bool ULRInventoryComponent::UseItem(int32 SlotIndex)
{
    // IsEmpty()를 활용하여 인덱스와 슬롯 상태를 한 번에 깔끔하게 검사
    if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex].IsEmpty())
    {
        return false;
    }
    
    ULRItemDataAsset* ItemData = InventorySlots[SlotIndex].ItemData;
    if (!ItemData || ItemData->ItemType != ELRItemType::Consumable)
    {
        return false;
    }
    
    ApplyItemEffects(ItemData);
    InventorySlots[SlotIndex].Quantity--;
    
    // 수량 차감 후 슬롯이 비었는지 IsEmpty()로 확인
    if (InventorySlots[SlotIndex].IsEmpty())
    {
        InventorySlots.RemoveAt(SlotIndex);
    }
    
    OnInventoryUpdated.Broadcast();

    return true;
}

void ULRInventoryComponent::ApplyItemEffects(const ULRItemDataAsset* ItemData)
{
    ALRCharacter* OwnerChar = Cast<ALRCharacter>(GetOwner());
    if (!OwnerChar) return;

    ULRStatusComponent* Status = OwnerChar->FindComponentByClass<ULRStatusComponent>();
    if (!Status) return;

    // ItemData에 정의된 회복 수치만큼 StatusComponent 수정
    if (ItemData->HealthRestore > 0.f)
    {
        Status->RestoreHealth(ItemData->HealthRestore);
    }
    
    if (ItemData->StaminaRestore > 0.f)
    {
        Status->RestoreStamina(ItemData->StaminaRestore);
    }
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

            // IsEmpty()를 활용하여 삭제 여부 판단
            if (InventorySlots[i].IsEmpty())
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