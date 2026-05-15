#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Items/LRInventoryStructs.h"
#include "LRGameInstance.generated.h"
UCLASS()
class LASTREFUGE_API ULRGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 레벨 이동 시 플레이어 인벤토리 보존 (FLRGridItem — 위치 정보 포함)
	UPROPERTY()
	TArray<FLRGridItem> PersistentInventory;

	// 기지 보관함 내용 보존
	UPROPERTY()
	TArray<FLRGridItem> PersistentStorageItems;

	// 툴바 슬롯 보존 (크기 4, 빈 슬롯은 IsEmpty()==true)
	UPROPERTY()
	TArray<FLRGridItem> PersistentToolbarItems;

	// 레벨 이동이 한 번이라도 일어났는지 (초기 스폰과 구분)
	bool bHasTravelData = false;

	// ItemID → DataAsset 전역 레지스트리 (LRSaveGame::Load에 전달)
	// BeginPlay에서 AssetManager 또는 에디터 DataTable로 채워줄 것
	UPROPERTY()
	TMap<FString, TObjectPtr<class ULRItemDataAsset>> ItemRegistry;
	
	// 플레이어 설정 (메인 메뉴에서 저장)
	UPROPERTY()
	float MouseSensitivity = 1.0f;

	UPROPERTY()
	float MasterVolume = 1.0f;

	virtual void Init() override;
	void RegisterItemAssets();
};
