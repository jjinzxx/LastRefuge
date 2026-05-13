#include "LRGameInstance.h"
#include "Items/LRItemDataAsset.h"
#include "Engine/AssetManager.h"

void ULRGameInstance::Init()
{
	Super::Init();
	RegisterItemAssets();
}

void ULRGameInstance::RegisterItemAssets()
{
	// 각 DataAsset을 소프트 경로로 로드 후 등록
	// 경로: 에디터에서 DataAsset 우클릭 → Copy Reference로 확인
	TArray<FSoftObjectPath> Paths = {
		FSoftObjectPath(TEXT("/Game/LR/DataAssets/DA_Scrap.DA_Scrap")),
		FSoftObjectPath(TEXT("/Game/LR/DataAssets/DA_Medkit.DA_Medkit")),
		FSoftObjectPath(TEXT("/Game/LR/DataAssets/DA_Ration.DA_Ration")),
	};

	for (const FSoftObjectPath& Path : Paths)
	{
		if (ULRItemDataAsset* DA = Cast<ULRItemDataAsset>(Path.TryLoad()))
		{
			if (!DA->ItemID.IsEmpty())
				ItemRegistry.Add(DA->ItemID, DA);
		}
	}
}