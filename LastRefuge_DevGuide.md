# Last Refuge - 개발 인수인계 문서 (Day 13 완료)

> 이 문서는 어느 채팅에서든 이어서 개발할 수 있도록 현재까지의 진행 상황, 코드 구조, 미완성 작업을 정리한 문서입니다.
> AI에게 이 문서를 첨부하고 "Last Refuge 프로젝트 Day N부터 이어서 진행해줘" 라고 하면 바로 이어받을 수 있습니다.

---

## 프로젝트 기본 정보

| 항목 | 내용 |
|---|---|
| 프로젝트명 | Last Refuge |
| 장르 | 포스트 아포칼립스 하드코어 잠입 생존 |
| 엔진 | Unreal Engine 5.7.4 |
| 언어 | C++ (Rider IDE) |
| 플랫폼 | PC 싱글플레이 |
| 템플릿 | First Person (Advanced, Variant 포함) |
| 모듈명 | `LastRefuge`, 접두사 `LR` |

---

## 핵심 게임 루프 (프로토타입 범위)

1. 안전 기지(Safe Zone)에서 출발
2. 폐허 맵으로 이동, AI 1~2마리 회피
3. 컨테이너에서 수색 게이지(3초) 채워 아이템 획득
4. 기지로 귀환, 보관함에 자원 저장
5. 반복 (긴장감과 성취감 검증)

---

## 폴더 구조

```text
Content/
└── LR/
    ├── Blueprints/      # BP_LRCharacter, BP_LRBot, BP_LRGameMode, BP_LRContainer, BP_LRStorage, BP_LRDoor
    ├── UI/              # WBP_LRHud, WBP_InventoryGrid, WBP_Storage, WBP_LRItem, WBP_DragPreview
    ├── Characters/
    ├── DataAssets/      # DA_Scrap, DA_Medkit, DA_Ration (ItemID, GridWidth, GridHeight 설정 필요)
    ├── Input/           # IMC_Default, IA_Move, IA_Look, IA_Jump, IA_Crouch, IA_Sprint, IA_Interact, IA_Inventory
    ├── Maps/            # L_Base, L_DangerZone
    └── AI/              # BT_LRBot, BB_LRBot

Source/LastRefuge/
├── Public/
│   ├── Character/       # LRCharacter.h
│   ├── AI/              # LRBot.h, LRBotAIController.h, BTTask_LRSelectNextPatrolPoint.h, BTTask_LRSuspiciousScan.h
│   ├── Components/      # LRStatusComponent.h, LRInventoryGridComponent.h (Day 13 신규, 구 LRInventoryComponent 대체)
│   ├── Interfaces/      # LRInteractable.h
│   ├── Items/           # LRItemDataAsset.h, LRInventoryStructs.h
│   ├── Actors/          # LRContainer.h, LRStorage.h, LRDoor.h  ← Day 13: Items/에서 이동
│   └── UI/              # LRHudWidget.h, LRInventoryGridWidget.h, LRItemWidget.h,
│                        #   LRDragPreviewWidget.h, LRItemDragDropOperation.h, LRStorageWidget.h
└── Private/
    └── (동일 구조)
```

---

## 완성된 클래스 및 현재 상태

### ALRCharacter (Character/LRCharacter.h/.cpp)
- 1인칭 카메라 및 Enhanced Input 기반 이동/시점/점프
- 자세 시스템 (Crouch / Walk / Run) 및 스테미나/소음 연동
- **[Day 9]** 전방 LineTrace 기반 상호작용 탐색 + 수색 타이머
- **[Day 12]** 델리게이트 브로드캐스트 (OnSearchStarted/Ended/ProgressChanged/InteractionPromptChanged)
- **[Day 13]** `ULRInventoryGridComponent* InventoryGrid` (구 ULRInventoryComponent 대체)
- **[Day 13]** `IA_Inventory` (Tab) → `ToggleInventory()`: WBP_InventoryGrid 생성/제거
- **[Day 13]** `OpenStorageScreen(ULRInventoryGridComponent*)` / `CloseStorageScreen()`: WBP_Storage 생성/제거
- **[Day 13]** BeginPlay에서 `GI->PersistentInventory` (FLRGridItem 배열)로 인벤 복원

### ULRInventoryGridComponent (Components/LRInventoryGridComponent.h/.cpp) — Day 13 신규
타르코프 스타일 10×5 그리드 인벤토리 컴포넌트.
- 내부: `TArray<int32> Grid` (1D, [y*W+x] = ItemID), `TMap<int32, FLRGridItem> Items`
- `CheckPlacement(X, Y, Item, bRotated)`: 경계 + 셀 겹침 검사
- `PlaceItem(X, Y, Item, bRotated)`: NextItemID++ 방식으로 셀 채움
- `RemoveItem(ItemID)`: ID 기반 제거, 인덱스 재정렬 없음
- `FindEmptySpace(Item, OutX, OutY, bOutRotated)`: First-fit, 원본→회전 순 시도
- `UseItem(ItemID)`: Consumable 타입이면 StatusComponent에 효과 적용
- `FOnGridChanged OnGridChanged` 델리게이트: 데이터 변경 시 UI 자동 리빌드 트리거

### 그리드 인벤토리 UI (UI/) — Day 13 신규

| 클래스 | 역할 |
|---|---|
| `ULRInventoryGridWidget` | 그리드 전체. `UCanvasPanel* GridCanvas` (BindWidget). DragOver/Drop/Leave/Cancelled 처리. `RebuildGrid()`로 아이템 위젯 재생성 |
| `ULRItemWidget` | 개별 아이템 슬롯. 좌클릭=드래그, 우클릭=UseItem, Shift+좌클릭=QuickTransfer |
| `ULRDragPreviewWidget` | 드래그 중 미리보기. 녹색(배치 가능) / 빨간색(불가) 하이라이트 |
| `ULRItemDragDropOperation` | 드래그 데이터: `DraggedItem`, `SourceGrid`, `SourceItemID`, `GrabOffsetSlots` |

**그리드 좌표 변환:**
- `GetGridIndexFromMouse(LocalPx)` = `FloorToInt(LocalPx / SlotSize)`
- `GridToLocal(X, Y)` = `(X * SlotSize, Y * SlotSize)`
- DragOver 스냅: `LocalPx -= GrabOffsetSlots * SlotSize` → 그리드 인덱스 계산

### ULRStorageWidget (UI/LRStorageWidget.h/.cpp) — Day 13 신규
보관함 열기 시 표시되는 분할 화면 위젯 (인벤토리 좌 + 창고 우).
- **BindWidget**: `UCanvasPanel* InventoryContainer`, `UCanvasPanel* StorageContainer`
- `TSubclassOf<ULRInventoryGridWidget> GridWidgetClass` (BP에서 WBP_InventoryGrid 할당)
- `InitStorage(InvGrid, InStorageGrid)`: 런타임에 ULRInventoryGridWidget 두 인스턴스 생성,
  각 Container에 `AddChildToCanvas`, Anchor (0,0→1,1), Offset 0으로 채움
- WBP_Storage Designer: Canvas Panel 2개(`InventoryContainer`, `StorageContainer`)만 배치.
  WBP_InventoryGrid 직접 배치 금지 (C++에서 동적 생성)

### ULRSaveGame (LRSaveGame.h/.cpp) — Day 13 신규
- SlotName: `"LRInventorySave"`
- `Save(InvGrid, StorageGrid, WorldCtx)`: 두 그리드 직렬화 → `TArray<FLRSavedItem>`
- `Load(InvGrid, StorageGrid, ItemRegistry, WorldCtx)`: 그리드 초기화 후 복원. 배치 충돌 시 FindEmptySpace 폴백

### FLRGridItem / FLRSavedItem (Items/LRInventoryStructs.h) — Day 13 수정
```cpp
USTRUCT() FLRGridItem {
    ULRItemDataAsset* ItemData;
    int32 GridX, GridY;
    int32 Width, Height;          // 아이템 고유 크기
    bool bIsRotated;
    int32 GetEffectiveWidth()  const;  // bIsRotated ? Height : Width
    int32 GetEffectiveHeight() const;
    bool IsEmpty() const;
};
USTRUCT() FLRSavedItem {
    FString ItemID;               // ULRItemDataAsset::ItemID
    int32 GridX, GridY;
    bool bIsRotated, bIsStorage;
};
```

### ULRItemDataAsset — Day 13 수정
- `FString ItemID` 추가 (고유 저장 키, DataAsset별로 수동 설정)
- `int32 GridWidth = 1`, `int32 GridHeight = 1` 추가

### ULRGameInstance — Day 13 수정
- `TArray<FLRGridItem> PersistentInventory` / `PersistentStorageItems` (FLRItemSlot → FLRGridItem)
- `TMap<FString, TObjectPtr<ULRItemDataAsset>> ItemRegistry`
- `Init()` → `RegisterItemAssets()`: FSoftObjectPath::TryLoad()로 DataAsset 등록

### ALRStorage (Actors/LRStorage.h/.cpp) — Day 13 수정
- `ULRInventoryGridComponent* StorageGrid` 서브컴포넌트 추가
- BeginPlay: `GI->PersistentStorageItems`에서 PlaceItem으로 복원
- EndInteract: 전체 Deposit/Withdraw 방식 → `Player->OpenStorageScreen(StorageGrid)` 호출로 변경
- 프롬프트: `[E] 보관함 열기`

### ALRDoor (Actors/LRDoor.cpp) — Day 13 수정
- 레벨 전환 전 인벤 저장: `GetInventoryGrid()->GetItems()` → FLRGridItem 배열
- 보관함 저장: `StorageActor->GetStorageGrid()->GetItems()` → FLRGridItem 배열

### ULRHudWidget (UI/LRHudWidget.h/.cpp) — Day 12 완비
- HP/스테미나/소음 Progress Bar, 상호작용 프롬프트 텍스트
- 수색 진행 점 애니메이션 (0.4초 간격), 완료/취소 텍스트 2초 표시

---

## 에디터 설정 체크리스트

### Blueprint 서브클래스
| Blueprint | 부모 클래스 | 추가 설정 |
|---|---|---|
| BP_LRCharacter | ALRCharacter | InventoryWidgetClass=WBP_InventoryGrid, StorageWidgetClass=WBP_Storage, IA_Inventory=IA_Inventory |
| WBP_InventoryGrid | ULRInventoryGridWidget | ItemWidgetClass=WBP_LRItem, PreviewWidgetClass=WBP_DragPreview |
| WBP_Storage | ULRStorageWidget | GridWidgetClass=WBP_InventoryGrid |
| WBP_LRItem | ULRItemWidget | - |
| WBP_DragPreview | ULRDragPreviewWidget | - |

### DataAsset 설정 (DA_Scrap / DA_Medkit / DA_Ration)
- `ItemID`: 고유 문자열 (예: `"Scrap"`, `"Medkit"`, `"Ration"`)
- `GridWidth` / `GridHeight`: 그리드 점유 크기 설정

### WBP_Storage Designer 구성
```
[Root] Canvas Panel
├── Canvas Panel  →  변수명: InventoryContainer  (Anchor 0,0→0.5,1 / 좌측)
└── Canvas Panel  →  변수명: StorageContainer    (Anchor 0.5,0→1,1 / 우측)
```
> WBP_InventoryGrid 인스턴스를 Designer에 직접 올리지 말 것. C++에서 런타임 생성.

### WBP_InventoryGrid Designer 구성
```
[Root] Canvas Panel (루트)
└── Canvas Panel  →  변수명: GridCanvas
```

---

## 2주 개발 일정 및 진행 현황

| Day | 작업 | 상태 |
|---|---|---|
| Day 1~6 | 기초 이동, 자세, 소음, AI 시스템 구축 | ✅ 완료 |
| Day 7 | 1주차 통합 테스트 + 마일스톤 검증 | ✅ 완료 |
| Day 8 | 인벤토리 시스템 + 아이템 데이터 에셋 | ✅ 완료 |
| Day 9 | 컨테이너 + 수색 게이지 + 인터럽트 로직 | ✅ 완료 |
| Day 10 | 아이템 사용(스탯 회복) 구현 + 보관함 + 사망 처리 | ✅ 완료 |
| Day 11 | GetInteractionPrompt + 레벨 전환(ALRDoor) + GameInstance 인벤/보관함 영속 | ✅ 완료 |
| Day 12 | Minimum Viable HUD (ULRHudWidget, 점 애니메이션, 완료/취소 텍스트) | ✅ 완료 |
| Day 13 | **타르코프 스타일 그리드 인벤토리 + 보관함 분할 UI** | ✅ 완료 |
| Day 14 | **밸런싱 (60%) + 사운드 (30%) + 사전 빌드 (10%)** | ⬜ 미완료 |
| Day 15 | **최종 빌드 + QA + 시연 영상** | ⬜ 미완료 |

---

## Day 13 — 타르코프 스타일 그리드 인벤토리 (완료)

### 구현 내용
- **폴더 이동**: `Items/` → `Actors/` (LRContainer, LRStorage, LRDoor)
- **ULRInventoryGridComponent**: 10×5 그리드, ID 기반 아이템 관리, 회전 지원
- **드래그 앤 드롭**: ULRItemDragDropOperation, GrabOffsetSlots 기반 정밀 스냅
- **배치 미리보기**: 녹색(가능)/빨간색(불가) 하이라이트
- **Shift+클릭 빠른 이동**: FindEmptySpace로 반대편 그리드에 자동 배치
- **보관함 분할 UI**: 인벤토리(좌) + 창고(우) 동시 표시, E키로 열기/닫기
- **저장/불러오기**: ULRSaveGame (SlotName: LRInventorySave), FLRSavedItem 직렬화
- **GameInstance 연동**: PersistentInventory/StorageItems를 FLRGridItem 배열로 교체

### 알려진 미완성 항목
- Tab키 인벤토리 열기: BP_LRCharacter의 `IA Inventory` 슬롯에 IA_Inventory 에셋 할당 필요
- DataAsset ItemID / GridWidth / GridHeight 값 수동 설정 필요
- GameInstance `RegisterItemAssets()` 내 Copy Reference 경로 실제 에셋 경로와 일치 여부 확인 필요

---

## Day 14 — 밸런싱 + 사운드 + 사전 빌드

### 밸런싱 (60%)

| 변수 | 초기값 | 조정 방향 |
|---|---|---|
| AI SightRadius | 1500 | 너무 자주 들키면 -200 |
| AI HearingRange | 2000 | Crouch 200 unit이 거의 안 들리는지 확인 |
| Walk 소음 반경 | 600 | Walk로 안전하게 다닐 수 있는가? |
| 수색 시간 | 3초 | 너무 길어 답답하면 2.5초 |
| AI 사격 데미지 | 25 | 4발 = 즉사 |

### 사운드 (30%)
발자국 3종 / AI 보이스 / 피격음 필수, 수색 루프음·BGM 권장.

### 사전 빌드 (10%)
Shipping 빌드 1회 패키징 → 실행 → 기본 루프 확인.

---

## Day 15 — 최종 빌드 + QA + 시연 영상

시연 영상 컷: 타이틀(0~10s) → 잠입(10~30s) → 수색+인터럽트(30~50s) → 귀환+저장(50~70s) → 보관함 컷(70~90s)

---

## 일정 리스크 매트릭스

| 리스크 | 대응 시점 | 대응 |
|---|---|---|
| 그리드 UI 드래그 버그 | Day 14 초반 | SlotSize/GrabOffset 수치 디버그 |
| Day 14 밸런싱 실패 | Day 14 저녁 | Day 15 오전을 밸런싱에 더 할애 |
| Day 15 빌드 깨짐 | Day 14 사전 빌드로 헤지 | PIE 녹화로 대체 |

---

## AI에게 전달할 세션 시작 문구

```text
Last Refuge UE5.7 C++ 프로젝트, Day 14 시작.
Day 13: 타르코프 스타일 그리드 인벤토리(ULRInventoryGridComponent, 10×5, 드래그앤드롭, 회전, Shift+클릭 이동),
        보관함 분할 UI(WBP_Storage = 인벤 좌 + 창고 우), Actors/ 폴더 분리 완료.
남은 미완성: IA_Inventory BP 할당, DataAsset ItemID/크기 설정, ItemRegistry 경로 확인.
다음 작업: 밸런싱(AI 수치 조정) + 사운드(발자국/피격음) + Shipping 사전 빌드.
엔진: UE 5.7.4, IDE: Rider, 접두사: LR
```
