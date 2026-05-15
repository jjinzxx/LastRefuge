# Last Refuge - 개발 인수인계 문서 (Day 15 진행 중)

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
    ├── Maps/            # L_Base, L_DangerZone, L_MainMenu
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
│                        #   LRDragPreviewWidget.h, LRItemDragDropOperation.h, LRStorageWidget.h,
│                        #   LRToolbarSlotWidget.h, LRPauseMenuWidget.h, LRMainMenuWidget.h
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

### ULRSaveGame (LRSaveGame.h/.cpp) — Day 13 신규, Day 15 수정
- SlotName: `"LRInventorySave"`
- `Save(InvGrid, StorageGrid, ToolbarItems, WorldCtx)`: 인벤/보관함/툴바 직렬화 → `TArray<FLRSavedItem>`
- `Load(InvGrid, StorageGrid, OutToolbarItems, ItemRegistry, WorldCtx)`: 그리드 초기화 후 복원. 배치 충돌 시 FindEmptySpace 폴백
- `FLRSavedItem`에 `bool bIsToolbar = false`, `int32 ToolbarSlot = -1` 필드 추가 (Day 15)

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
| WBP_InventoryGrid | ULRInventoryGridWidget | ItemWidgetClass=WBP_LRItem (**PreviewWidgetClass 제거됨 — Day 14에서 NativePaint 방식으로 변경**) |
| WBP_Storage | ULRStorageWidget | GridWidgetClass=WBP_InventoryGrid |
| WBP_LRItem | ULRItemWidget | - |
| WBP_DragPreview | ULRDragPreviewWidget | ⚠️ 더 이상 ULRInventoryGridWidget에서 사용하지 않음 |

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
| Day 14 | **그리드 UI 버그 수정 + 아이템 스태킹 + 좌/우클릭 분리 + 컨텍스트 메뉴** | ✅ 완료 |
| Day 15 | **1인칭 손/다리 + 툴바 + 적 애니메이션 + 메인화면 + ESC 메뉴 + 사운드** | 🔄 진행 중 (툴바·ESC·메인화면 완료) |

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

## Day 14 — 그리드 UI 버그 수정 + 신규 기능

### Day 14에서 수정한 것

#### 드래그 프리뷰 좌표계 수정
- **원인**: `ULRDragPreviewWidget`을 Canvas Panel 자식으로 추가하는 방식에서 Canvas Panel Slot의 앵커/정렬 기본값 문제로 시각적 위치가 틀렸음
- **해결**: `NativePaint`에서 직접 `FSlateDrawElement::MakeBox`로 프리뷰 사각형 렌더링
  - 그리드 선과 완전히 동일한 좌표계(`AllottedGeometry`) 사용
  - 멤버 변수: `PreviewGridPos`, `PreviewItemW`, `PreviewItemH`, `bPreviewVisible`, `bPreviewCanPlace`
  - `ShowPreview()` → 상태 업데이트 + `Invalidate(Paint)` 호출
- **제거됨**: `NativeConstruct()`, `PreviewWidgetClass` UPROPERTY, `PreviewWidget` UPROPERTY

#### 기타 수정
- 툴팁 위치 고정 (`PC->GetMousePosition` 사용, `GetScreenSpacePosition` 오류 수정)
- 그리드 슬롯 크기 50px → 60px
- 스토리지 화면 중복 열기/닫기 버그 수정 (`bStorageOpen` 가드 추가)

### ~~미해결 버그 — 드래그 프리뷰 위치~~ ✅ 해결됨

**해결**: `EDragPivot::TopLeft` + 커서 기준 배치로 단순화. 프리뷰·배치 모두 커서 셀 기준으로 통일.
- GrabOffset 제거 (`FLRItemDragDropOperation.GrabOffsetSlots`, `LRInventoryGridWidget.GrabOffsetSlots` 삭제)
- `NativePaint` 프리뷰 조건에 `PreviewGridPos.Y >= 0` 추가

### ⚠️ 알려진 이슈 — 드래그 시작 시 아이콘 플리커 (QA 처리)

**증상**: 드래그를 시작하면 아이콘이 잠깐 화면 왼쪽 위에서 커서 위치로 날아오는 것처럼 보임.

**원인**: UMG DragDrop 시스템에서 `NewObject`로 생성한 위젯은 첫 프레임에 레이아웃이 없어
Slate가 임시로 (0,0)에 배치 → 두 번째 프레임에 커서 위치로 이동. Slate 레이어를 직접 구현하지
않으면 UMG API 레벨에서 완전히 해결 불가.

**영향 범위**: 시각적 글리치, 기능(배치·취소·프리뷰)에는 영향 없음. **Day 15 QA "알려진 이슈"로 기록.**

### Day 14에서 추가 구현된 것 (스태킹 · 컨텍스트 메뉴)

#### 아이템 스태킹
- `FLRGridItem.Quantity`, `FLRSavedItem.Quantity` 필드 추가
- `ULRInventoryGridComponent`: `CanStack()`, `AddToStack()`, `ReduceQuantity()` 추가
- `ULRItemWidget`: `QuantityText` (BindWidgetOptional), Quantity > 1일 때만 표시
- `ALRContainer::BeginPlay`: 기존 스택에 합산 우선, 공간 없으면 FindEmptySpace

#### 좌클릭 / 우클릭 분리
- `ULRInventoryGridWidget::NativeOnMouseButtonDown`: 좌클릭=전체 드래그, 우클릭=컨텍스트 메뉴 대기
- `NativeOnDragDetected`: 우클릭 드래그이면 `ReduceQuantity(1)` 후 1개짜리 아이템 드래그
- `NativeOnDrop`: `CanStack` 우선 시도, 실패 시 `CheckPlacement`+`PlaceItem`

#### 컨텍스트 메뉴 (ULRContextMenuWidget)
- 우클릭 단순 클릭 → 커서 위치에 버티컬 메뉴 팝업
- 항목: `[사용하기]` (Consumable 전용) + `[정보]` (모든 아이템)
- 전체화면 투명 BackdropButton → 어디서든 클릭 시 메뉴 닫힘
- 좌클릭/우클릭 모두 `CloseContextMenu()` 호출
- `FSimpleMulticastDelegate OnMenuClosed`로 그리드에 닫힘 통보

#### 인벤토리 이미지 오정렬 버그 수정
- **원인**: `NativeDestruct`가 `OnGridChanged` 핸들 제거 → 탭 재열기 시 `RebuildGrid` 미호출
- **수정**: `ToggleInventory` 열 때마다 `InitGrid` 재호출, `NativeDestruct`에서 `ActiveTooltip = nullptr`

### WBP_ContextMenu Designer 구성 (Day 14 신규)
```
[Root] Canvas Panel
├── Button "BackdropButton"   Anchors (0,0)→(1,1), Offset 0, 배경 Alpha=0, ZOrder=0
└── Vertical Box "MenuBox"    ZOrder=1, 위치는 C++에서 PositionNearMouse()로 설정
```

---

## Day 15 — 비주얼 · UI · 오디오 완성

### 1. 1인칭 손 · 다리 구현
- 캐릭터 1인칭 뷰에서 손과 다리가 화면에 보이도록 메시/애니메이션 추가
- 아이템을 들었을 때 손에 들고 있는 모습 포함 (툴바 아이템 장착 시 연동)
- 관련 클래스: `ALRCharacter` + 1인칭 Arms/Legs SkeletalMesh 추가

### 2. 툴바 (단축키 슬롯 1~4)
- 화면 하단 HUD에 4개 슬롯 표시
- 인벤토리에서 아이템을 슬롯에 드래그하거나 지정 가능
- 슬롯 유형별 동작:
  - **장비류 (무기, 도구)** → 1인칭 손에 장착 (들고 다니기)
  - **소비·회복 아이템** → 키 누르면 즉시 UseItem
  - **키카드류** → 손에 들고, 문 상호작용 시 자동 소모
- 키 바인딩: `1` `2` `3` `4`
- 관련 클래스: `ULRHudWidget` (슬롯 UI), `ALRCharacter` (ActiveSlot 상태 관리)

### 3. 적 모션 · 공격 애니메이션
- `ALRBot` 순찰/의심/추격 상태별 Locomotion 애니메이션 연결
- 근접 공격 또는 사격 공격 모션 추가
- ABP_LRBot (AnimBlueprint) 작성: Idle / Walk / Run / Attack 스테이트

### 4. 메인 화면 (엔트리 UI)
- 게임 시작 시 가장 먼저 보이는 화면
- 버튼 구성: **[게임 시작]** / **[설정]** / **[종료]**
- 설정: 마우스 감도, 볼륨 슬라이더 등 기본 옵션
- 관련 클래스: `ULRMainMenuWidget` (신규), 전용 맵 `L_MainMenu` 또는 레벨 블루프린트

### 5. ESC 인게임 메뉴
- 플레이 중 ESC 누르면 팝업 메뉴
- 항목: **[계속하기]** / **[설정]** / **[메인으로]** / **[종료]**
- `IA_Menu` (ESC 키) → `ALRCharacter::TogglePauseMenu()`
- 관련 클래스: `ULRPauseMenuWidget` (신규)

### 6. 사운드 연결
- 발자국 3종 (Walk / Run / Crouch)
- AI 감지 보이스 (발견 / 의심 / 복귀)
- 피격음 / 사망음
- 아이템 사용음 (회복, 수색 완료)
- 수색 루프음 · 앰비언트 BGM (권장)
- 관련 클래스: `UAudioComponent` 또는 `UGameplayStatics::PlaySoundAtLocation`

---

## Day 16 — 최종 빌드 + QA + 시연 영상

시연 영상 컷: 타이틀(0~10s) → 잠입(10~30s) → 수색+인터럽트(30~50s) → 귀환+저장(50~70s) → 보관함 컷(70~90s)

### 밸런싱 체크

| 변수 | 초기값 | 조정 방향 |
|---|---|---|
| AI SightRadius | 1500 | 너무 자주 들키면 -200 |
| AI HearingRange | 2000 | Crouch 200 unit이 거의 안 들리는지 확인 |
| Walk 소음 반경 | 600 | Walk로 안전하게 다닐 수 있는가? |
| 수색 시간 | 3초 | 너무 길어 답답하면 2.5초 |
| AI 사격 데미지 | 25 | 4발 = 즉사 |

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
Last Refuge UE5.7 C++ 프로젝트, Day 14 이어서.
Day 13: 타르코프 스타일 그리드 인벤토리(ULRInventoryGridComponent, 10×5, 드래그앤드롭, 회전, Shift+클릭 이동),
        보관함 분할 UI(WBP_Storage = 인벤 좌 + 창고 우).
Day 14 진행 중:
  - 드래그 프리뷰를 NativePaint 직접 렌더링 방식으로 교체 (Canvas Panel Slot 좌표계 버그 해결)
  - 미해결: 아이템을 그리드 엣지(예: 9,4)에서 드래그하면 프리뷰 위치가 틀림
    → [Grab] 진단 로그(NativeOnMouseButtonDown) 추가됨, 다음 세션에서 로그 확인 후 수정
  - 다음 작업:
    1. [Grab] 로그 확인 → 프리뷰 버그 수정
    2. 아이템 스태킹 (FLRGridItem에 Quantity 추가, MaxStackSize 제한)
    3. 좌클릭=전체 스택 드래그, 우클릭=1개만 드래그
엔진: UE 5.7.4, IDE: Rider, 접두사: LR
```
