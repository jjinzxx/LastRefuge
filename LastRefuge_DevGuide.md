# Last Refuge - 개발 인수인계 문서 (2026-05-21 진행 중)

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
    ├── Input/           # IMC_Default, IA_Move, IA_Look, IA_Jump, IA_Crouch, IA_Sprint, IA_Interact, IA_Inventory, IA_Attack
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
- **[Day 16]** `IA_Attack` (LMB) → `TryAttack()`: ECC_Pawn LineTrace 250 범위, 30 데미지, 0.8초 쿨다운
- **[Day 16]** `AttackDamage / AttackRange / AttackCooldown` EditDefaultsOnly — BP에서 조정 가능
- **[Day 16]** `SFX_Attack` 슬롯 추가
- **[Day 16]** BeginPlay에서 `MaxWalkSpeed = WalkSpeed` 적용 — BP 이동속도 값 실제 반영
- **[Day 16]** `TogglePauseMenu()` 닫기 시 `PauseMenuWidget = nullptr` — 재열기 시 새 인스턴스 생성으로 버튼 중복 바인딩 버그 수정
- **[2026-05-20]** `float MaxCarryWeight = 30.f` 추가, `void UpdateMovementSpeed()` 추가
- **[2026-05-20]** `BeginPlay`: `InventoryGrid->OnGridChanged.AddUObject(this, &ALRCharacter::UpdateMovementSpeed)` — 인벤 변경 시 자동 속도 재계산
- **[2026-05-20]** `UpdateMovementSpeed()`: `GetTotalWeight() / MaxCarryWeight` 비율로 Lerp(1.0 → 0.5) 속도 배수 적용, 현재 MovementState(Walk/Run/Crouch) 기준 BaseSpeed에 곱함
- **[2026-05-20]** `SetMovementState()`: MaxWalkSpeed 직접 설정 제거 → `UpdateMovementSpeed()` 위임
- **[2026-05-20]** `TryInteract()`: `CanInteract()` 차단 시 잠금 프롬프트 브로드캐스트 후 조기 반환

### ULRInventoryGridComponent (Components/LRInventoryGridComponent.h/.cpp) — Day 13 신규
타르코프 스타일 10×5 그리드 인벤토리 컴포넌트.
- 내부: `TArray<int32> Grid` (1D, [y*W+x] = ItemID), `TMap<int32, FLRGridItem> Items`
- `CheckPlacement(X, Y, Item, bRotated)`: 경계 + 셀 겹침 검사
- `PlaceItem(X, Y, Item, bRotated)`: NextItemID++ 방식으로 셀 채움
- `RemoveItem(ItemID)`: ID 기반 제거, 인덱스 재정렬 없음
- `FindEmptySpace(Item, OutX, OutY, bOutRotated)`: First-fit, 원본→회전 순 시도
- `UseItem(ItemID)`: Consumable 타입이면 StatusComponent에 효과 적용
- `FOnGridChanged OnGridChanged` 델리게이트: 데이터 변경 시 UI 자동 리빌드 트리거
- **[2026-05-20]** `float GetTotalWeight() const`: 전체 아이템 `Weight * Quantity` 합산 반환

### 그리드 인벤토리 UI (UI/) — Day 13 신규

| 클래스 | 역할 |
|---|---|
| `ULRInventoryGridWidget` | 그리드 전체. `UCanvasPanel* GridCanvas` (BindWidget). DragOver/Drop/Leave/Cancelled 처리. `RebuildGrid()`로 아이템 위젯 재생성. **[2026-05-20]** NativePaint에서 배경 직접 렌더링(`GridBackgroundColor`), DimOverlay 위젯 불필요 |
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

### ULRSaveGame (LRSaveGame.h/.cpp) — Day 13 신규, Day 15·2026-05-20 수정
- SlotName: `"LRInventorySave"`
- `Save(InvGrid, StorageGrid, ToolbarItems, WorldCtx, FallbackStorageItems)`: 인벤/보관함/툴바 직렬화 → `TArray<FLRSavedItem>`
- `Load(InvGrid, StorageGrid, OutToolbarItems, ItemRegistry, WorldCtx)`: 그리드 초기화 후 복원. 배치 충돌 시 FindEmptySpace 폴백
- `FLRSavedItem`에 `bool bIsToolbar = false`, `int32 ToolbarSlot = -1` 필드 추가 (Day 15)
- **[2026-05-20]** `FallbackStorageItems` 파라미터 추가: `StorageGrid == nullptr`(DangerZone)일 때 `GI->PersistentStorageItems`를 직렬화 → DangerZone 저장 시 창고 데이터 소실 버그 수정

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

### ALRContainer (Actors/LRContainer.h/.cpp) — 2026-05-20 추가
- **[2026-05-20]** `ELRContainerType` enum 추가: `Normal` / `Locked` (키카드 필요) / `Alarmed` (수색 완료 시 AI 경보)
- **[2026-05-20]** `ContainerType` (EditAnywhere), `RequiredKeyCardID` (EditCondition: Locked일 때만 표시)
- **[2026-05-20]** `CanInteract()`: Locked 타입이면 인벤 순회로 키카드 소지 확인. 이미 수색된 컨테이너는 항상 허용
- **[2026-05-20]** `EndInteract()`: Alarmed 타입 완료 시 `UAISense_Hearing::ReportNoiseEvent` 호출 (AI 경보 트리거)
- **[2026-05-20]** `GetInteractionPrompt()`: 등급별 잠금/경보 텍스트 분기

### ALRStorage (Actors/LRStorage.h/.cpp) — Day 13 수정
- `ULRInventoryGridComponent* StorageGrid` 서브컴포넌트 추가
- BeginPlay: `GI->PersistentStorageItems`에서 PlaceItem으로 복원
- EndInteract: 전체 Deposit/Withdraw 방식 → `Player->OpenStorageScreen(StorageGrid)` 호출로 변경
- 프롬프트: `[E] 보관함 열기`

### ALRDoor (Actors/LRDoor.h/.cpp) — Day 13 수정, 2026-05-20 추가
- 레벨 전환 전 인벤 저장: `GetInventoryGrid()->GetItems()` → FLRGridItem 배열
- 보관함 저장: `StorageActor->GetStorageGrid()->GetItems()` → FLRGridItem 배열
- **[2026-05-20]** `bool bRequiresKeyCard`, `FString RequiredKeyCardID` 추가
- **[2026-05-20]** `CanInteract()`: 키카드 소지 확인 (인벤 순회)
- **[2026-05-20]** `GetInteractionPrompt()`: GI `ItemRegistry`로 키카드 이름 조회 → `[E] 레벨명 이동  🔒 키카드명 필요` 표시

### ALROpenableDoor (Actors/LROpenableDoor.h/.cpp) — 2026-05-20 신규
열고닫을 수 있는 인게임 문. 레벨 전환용 ALRDoor와 별개 클래스.
- `USceneComponent* HingeRoot` (루트): 경첩 피벗. 액터 원점 = 경첩 위치
- `UStaticMeshComponent* MeshComponent`: HingeRoot 자식, Y축 `HingeOffsetY` 오프셋
- `OnConstruction()`: `SetRelativeLocation(0, HingeOffsetY, 0)` — 에디터에서 실시간 확인 가능
- `BeginPlay()`: `ClosedYaw = GetActorRotation().Yaw` 캐싱, `TargetYaw = ClosedYaw`
- `BeginInteract()`: `bIsOpen` 토글, `TargetYaw` 설정, `SetActorTickEnabled(true)`
- `Tick()`: `FInterpTo(Current.Yaw, TargetYaw, DeltaTime, OpenSpeed)`, 0.1도 이내 스냅 후 Tick 비활성화
- `CanInteract()`: `bIsOpen == true`이면 항상 허용(닫기), 닫힌 상태에서만 키카드 체크
- `GetInteractionPrompt()`: GI `ItemRegistry`로 키카드 이름 조회
- 프로퍼티 (EditAnywhere): `HingeOffsetY=50`, `OpenAngle=90`, `OpenSpeed=3`, `bRequiresKeyCard`, `RequiredKeyCardID`
- **에디터 배치**: 액터 오렌지 화살표(원점)를 경첩 위치에 맞춰 배치 → 메시가 Y방향으로 뻗어나감

### ULRHudWidget (UI/LRHudWidget.h/.cpp) — Day 17 수정
- **[Day 12]** 상호작용 프롬프트 텍스트, 수색 진행 텍스트, 완료/취소 텍스트 2초 표시
- **[Day 16]** 수색 시간 표시 `(현재 / 전체 sec)` 텍스트 애니메이션
- **[Day 17]** HP/STA 바: `ULRSegmentBarWidget` (NativePaint) → `UProgressBar` (`PB_HP`, `PB_Sta`) 교체
  - `OnHealthChanged`: `PB_HP->SetPercent(NewHealth / MaxHealth)`
  - `OnStaminaChanged`: `PB_Sta->SetPercent(NewStamina / MaxStamina)`
  - WBP_LRHud: 커스텀 프레임 이미지(`Image_HUDFrame`) 오버레이 구조
- **[Day 17]** `ULRStatusComponent` — `GetMaxHealth()`, `GetMaxStamina()` getter 추가

### ULRSegmentBarWidget (UI/LRSegmentBarWidget.h/.cpp) — Day 17 폐기
- NativePaint 기반 세그먼트 바로 구현했으나 UUserWidget의 Slate 캐싱 문제로 실시간 갱신 불가
- UProgressBar + 프레임 이미지 오버레이 방식으로 대체, 현재 미사용

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
> DimOverlay 위젯은 삭제해도 무방. 배경은 C++ NativePaint에서 GridBackgroundColor(LR|Style)로 직접 렌더링.
> 배경과 그리드 선이 동일한 Origin·SlotSize 변수를 공유하므로 픽셀 단위 일치 보장.

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
| Day 15 | **1인칭 손/다리 + 툴바 + 적 애니메이션 + 메인화면 + ESC 메뉴 + 사운드** | ✅ 완료 (1인칭 손·다리 제외 — 풀바디로 멀티 시 구현 예정) |
| Day 16 | **플레이어 공격 + 봇 체력/피격 시스템 + QA 버그 수정 + 밸런싱** | 완료 |
| Day 17 | **HUD 비주얼 개선 — HP/STA 커스텀 프레임 이미지 + ProgressBar 전환** | 완료 |
| 2026-05-18 | **UI 전체 미니멀 라인아트 스타일 전환** | 완료 |
| 2026-05-19 | **PauseMenu/MainMenu 스타일 + 사운드 시스템 + 메인메뉴 저장 + 패키징** | 완료 |
| 2026-05-20 | **컨테이너 등급/잠금 + 무게 이동속도 + ALROpenableDoor + 그리드 배경 버그 수정** | 완료 |
| 2026-05-21 | **2차 코드 전체 리뷰 수정 + 툴바 드롭 수신 + 아이콘 실루엣 버그** | 완료 |

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

### ✅ 완료 — 툴바 (단축키 슬롯 1~4)

툴바는 인벤토리와 **별도의 물리적 보관 공간**으로 구현. 아이템이 실제로 인벤에서 빠져 나와 툴바 슬롯에 저장됨.

**ALRCharacter 변경:**
- `TArray<FLRGridItem> ToolbarItems` (크기 4, 빈 슬롯 = `IsEmpty() == true`)
- `SetToolbarSlot(SlotIndex, FLRGridItem)`: 슬롯에 아이템 저장 + `OnToolbarSlotChanged` 브로드캐스트
- `ClearToolbarSlot(SlotIndex)`: 아이템을 인벤토리로 반환 후 슬롯 비움 (우클릭)
- `TakeToolbarItem(SlotIndex)`: 드래그 시작 시 슬롯에서 꺼내기 (인벤 반환 없음)
- `UseToolbarSlot(SlotIndex)`: Consumable이면 StatusComponent로 효과 적용, Quantity 감소
- `TogglePauseMenu()`: 인벤/보관함 열려있으면 먼저 닫음
- BeginPlay: `GI->PersistentToolbarItems`로 툴바 복원, `SetInputMode(FInputModeGameOnly())` 리셋
- Look(): `GI->MouseSensitivity` 곱셈 적용
- `IA_Toolbar1~4`, `IA_Menu` 바인딩 추가

**ULRToolbarSlotWidget (신규):**
- `InitSlot(ALRCharacter*, SlotIndex)` / `RefreshSlot(ULRItemDataAsset*, Quantity)`
- BindWidget: `IconImage` / BindWidgetOptional: `QuantityText`, `SlotNumberText`
- 우클릭 → `ClearToolbarSlot` (인벤 반환)
- 좌클릭 드래그 → `TakeToolbarItem` → `ULRItemDragDropOperation` 생성
- Drop: 슬롯 점유 시 기존 아이템을 소스 그리드로 스왑 → `SetToolbarSlot`

**ULRHudWidget 변경:**
- BindWidgetOptional: `ToolbarSlot1~4`
- `OnToolbarSlotChanged(SlotIndex, ItemData, Quantity)` 핸들러
- `RefreshAllToolbarSlots()`: `GetToolbarItems()` 직접 참조 (그리드 미사용)
- `InitSlot` 4개 호출 후 `RefreshAllToolbarSlots()` (초기 상태 반영)

**ULRGameInstance 변경:**
- `TArray<FLRGridItem> PersistentToolbarItems` 추가
- `float MouseSensitivity = 1.0f`, `float MasterVolume = 1.0f` 추가

**에디터 설정:**
- `IA_Toolbar1~4`, `IA_Menu` InputAction 에셋 생성 후 BP_LRCharacter에 할당
- WBP_Hud에 `ToolbarSlot1~4` 슬롯 위젯 배치 (변수명 정확히 일치)
- WBP_ToolbarSlot: `IconImage`(Image 위젯), `QuantityText`(TextBlock, Optional), `SlotNumberText`(TextBlock, Optional)
- L_MainMenu World Settings → GameMode Override → **Game Mode Base** (Default Pawn = None) 설정 필요

---

### ✅ 완료 — ESC 인게임 일시정지 메뉴

**ULRPauseMenuWidget (신규):**
- BindWidget: `Btn_Resume`, `Btn_MainMenu`, `Btn_Quit`
- Resume → `Char->TogglePauseMenu()`
- MainMenu → `UGameplayStatics::OpenLevel(this, "L_MainMenu")`
- Quit → `UKismetSystemLibrary::QuitGame`

**ALRCharacter::TogglePauseMenu():**
- 인벤/보관함 열려있으면 먼저 닫고 종료
- 메뉴 열기: `SetGamePaused(true)`, `SetShowMouseCursor(true)`, `SetInputMode(GameAndUI)`, 위젯 AddToViewport
- 메뉴 닫기: `SetGamePaused(false)`, `SetShowMouseCursor(false)`, `SetInputMode(GameOnly)`, 위젯 RemoveFromParent

**에디터 설정:**
- `PauseMenuWidgetClass` = WBP_PauseMenu (BP_LRCharacter에서 할당)
- ESC 키 → IA_Menu에 바인딩 (현재 임시로 `5`키 사용 중, 추후 ESC로 변경)

---

### ✅ 완료 — 메인 화면

**ULRMainMenuWidget (신규):**
- BindWidget: `Btn_Start`, `Btn_Settings`, `Btn_Quit`, `Panel_Settings`, `SL_Sensitivity`, `SL_Volume`
- `StartLevelName = "L_Base"` (EditAnywhere)
- Settings 버튼 → `Panel_Settings` 토글 (Collapsed ↔ Visible)
- 슬라이더 변경 → `GI->MouseSensitivity` / `GI->MasterVolume` 저장
- 볼륨 변경 → `FAudioDevice::SetTransientPrimaryVolume(Value)` 즉시 반영
- NativeConstruct에서 GI 저장값으로 슬라이더 초기값 설정

**레벨 설정:**
- `L_MainMenu` 전용 맵. World Settings GameMode Override = **Game Mode Base** (Default Pawn = None)
- Level Blueprint BeginPlay → Get Player Controller → Show Mouse Cursor = true, SetInputMode UIOnly

---

### ⬜ 미완료 — 사운드 연결
- 발자국 3종 (Walk / Run / Crouch), AI 감지 보이스, 피격음/사망음
- 아이템 사용음, 수색 루프음, 앰비언트 BGM
- `UAudioComponent` 또는 `UGameplayStatics::PlaySoundAtLocation`

### ✅ 완료 — 적 애니메이션
- `ULRBotAnimInstance` (Speed, BotState 변수) + `ABP_LRBot` (Blend Space 1D: Idle~Walk)
- `AM_Bot_MeleeAttack` AnimMontage — DefaultSlot 통해 재생
- Mixamo 애니메이션 → IK Retargeter(RTG_Manny_to_Quantum)로 QuantumCharacter 스켈레톤에 리타게팅
- **알려진 문제**: QuantumCharacter 손가락 서브본(`_half`, `_in`, `_dip` 등) 미매핑 → 손가락 모양 부자연스러움

### ✅ 완료 — AI 근접 공격
- 원거리 사격(LineTrace) → 근접 공격(거리 체크 `MeleeRange=150`) 방식으로 교체
- `TryMeleeAttack()`: 범위 내 진입 시 `MeleeDamage` 적용 + `MeleeAttackMontage` 재생
- `MeleeDamage=25`, `MeleeInterval=1.5f`, `MeleeRange=150f` (BP_LRBot에서 조정 가능)

### ✅ 완료 — 제압 시스템
- `IA_Takedown` (F키) → `ALRCharacter::TryTakedown()`
- 조건: 봇 뒤 방향 ±60도, 거리 150 이내
- 성공: `ALRBot::TakedownKill()` → DeathMontage 재생 + AI 비활성화 + 3초 후 제거
- `bIsDead` 플래그로 중복 제압 방지
- Tick에서 조건 충족 시 `[F] 제압하기` 프롬프트 표시

### ⬜ 미완료 — 1인칭 손·다리
- 풀바디 캐릭터 방식으로 멀티플레이 구현 시 함께 처리 예정

---

## Day 16 — 플레이어 공격 + 봇 체력/피격 + QA 버그 수정

시연 영상 컷: 타이틀(0~10s) → 잠입(10~30s) → 수색+인터럽트(30~50s) → 귀환+저장(50~70s) → 보관함 컷(70~90s)

### ✅ 완료 — 플레이어 근접 공격

**ALRCharacter 변경:**
- `IA_Attack` (LMB) → `TryAttack()`: `ECC_Pawn` 채널 LineTrace
  - 범위: 250 (`AttackRange`, EditDefaultsOnly)
  - 데미지: 30 (`AttackDamage`, EditDefaultsOnly)
  - 쿨다운: 0.8초 (`AttackCooldown`, EditDefaultsOnly)
  - 인벤/보관함/일시정지 열려있으면 공격 차단
- `SFX_Attack` 슬롯 추가 (BP에서 할당)
- `BeginPlay`에서 `MaxWalkSpeed = WalkSpeed` 적용 (BP 속도값 실제 반영)

> **ECC_Visibility 대신 ECC_Pawn 사용 이유**: 캐릭터 캡슐의 기본 콜리전 프로필(`Pawn`)은 Visibility 채널을 Ignore로 설정하기 때문에 ECC_Visibility로 쏘면 봇을 통과함.

**에디터 설정:**
- `Content/LR/Input/` → `IA_Attack` (Digital bool) 생성
- `IMC_Default`에 `IA_Attack` → **Left Mouse Button** 매핑
- BP_LRCharacter: `IA_Attack` 할당, `SFX_Attack` 슬롯에 공격음 할당 (선택)

### ✅ 완료 — 봇 체력 + 피격 시스템

**ALRBot 변경:**
- `MaxHealth = 100`, `CurrentHealth` (EditAnywhere, BP에서 조정 가능)
- `TakeDamage` 오버라이드: HP 감소 → 0 이하면 `TakedownKill()` 호출
- `HitReactMontage`: HP가 남아있을 때 피격 시 재생 (BP에서 할당)
- `BeginPlay`에서 `MaxWalkSpeed = PatrolSpeed` 적용 (BP 속도값 실제 반영)

**ALRBotAIController 변경:**
- `NotifyHitBy(AActor* Attacker)` public 메서드 추가
- 피격 시 `EnterCombat(Attacker)` 호출 → 즉시 전투 상태 전환 + 추적 시작

**에디터 설정:**
- BP_LRBot: `Hit React Montage` → `AM_Bot_HitReact` 할당 (Mixamo 리타게팅 필요)
- BP_LRBot: `Max Health` — 기본 100, 조정 가능

### ✅ 완료 — QA 버그 수정

| 버그 | 원인 | 수정 |
|---|---|---|
| ESC 메뉴 계속하기 2회차 무반응 | `RemoveFromParent` 후 위젯 객체 잔존 → 재열기 시 `NativeConstruct` 재호출로 버튼 중복 바인딩 | 닫기 시 `PauseMenuWidget = nullptr` — 매번 새 인스턴스 생성 |
| BP 이동속도 변경 미반영 (플레이어) | 생성자에서 C++ 기본값 고정 | `BeginPlay`에서 `MaxWalkSpeed = WalkSpeed` 재적용 |
| BP 이동속도 변경 미반영 (봇) | 생성자에서 C++ 기본값 고정 | `BeginPlay`에서 `MaxWalkSpeed = PatrolSpeed` 재적용 |

### 밸런싱 체크

| 변수 | 기본값 | 조정 위치 | 조정 방향 |
|---|---|---|---|
| AI SightRadius | 1200 | BP_LRBot | 너무 자주 들키면 -200 |
| AI HearingRange | 1800 | BP_LRBot | Crouch 이동 시 거의 안 들리는지 확인 |
| Walk 소음 반경 | 600 | BP_LRCharacter | Walk로 안전하게 다닐 수 있는가? |
| 수색 시간 | 3초 | BP_LRContainer | 답답하면 2.5초 |
| AI 근접 데미지 | 25 | BP_LRBot | 플레이어 공격력과 균형 확인 |
| 플레이어 공격 데미지 | 30 | BP_LRCharacter | 봇 HP 100 기준 4타 사망 |
| 봇 체력 | 100 | BP_LRBot | 조정 가능 |

---

## 향후 추가 기능

| 기능 | 상태 | 설명 |
|---|---|---|
| AI 근접 공격 | ✅ 완료 | 거리 기반 데미지 + MeleeAttackMontage |
| 제압 시스템 | ✅ 완료 | F키, 뒤 방향 ±60도, DeathMontage |
| 플레이어 공격 | ✅ 완료 | LMB, ECC_Pawn LineTrace 250 범위, 30 데미지, 0.8초 쿨다운 |
| 사운드 에셋 연결 | ⬜ 예정 | 발자국·피격·AI 보이스 에셋 준비 후 BP에서 할당 |
| 1인칭 손·다리 | ⬜ 예정 | 멀티플레이 구현 시 풀바디 방식으로 함께 처리 |
| 손가락 리타게팅 개선 | ⬜ 알려진 문제 | QuantumCharacter 서브본 미매핑 → 손가락 부자연스러움 |

---

## 일정 리스크 매트릭스

| 리스크 | 대응 시점 | 대응 |
|---|---|---|
| 그리드 UI 드래그 버그 | Day 14 초반 | SlotSize/GrabOffset 수치 디버그 |
| Day 14 밸런싱 실패 | Day 14 저녁 | Day 15 오전을 밸런싱에 더 할애 |
| Day 15 빌드 깨짐 | Day 14 사전 빌드로 헤지 | PIE 녹화로 대체 |

---

## Day 17 — HUD 비주얼 개선 (진행 중)

### 완료 — HP/STA 바 시스템 교체

**배경:**
- NativePaint 기반 `ULRSegmentBarWidget` 구현 → Slate SInvalidationPanel 캐싱으로 실시간 갱신 불가
- `Invalidate(Paint)`, `RegisterActiveTimer`, `SetVolatile` 등 다수 방법 시도 모두 실패
- UProgressBar + 커스텀 프레임 이미지 오버레이 방식으로 전환

**ULRHudWidget 변경:**
- `ULRSegmentBarWidget* PB_HP / PB_Stamina` → `UProgressBar* PB_HP / PB_Sta`
- `#include "Components/ProgressBar.h"` 교체
- `OnHealthChanged`: `PB_HP->SetPercent()`
- `OnStaminaChanged`: `PB_Sta->SetPercent()`

**ULRStatusComponent 변경:**
- `GetMaxHealth()` / `GetMaxStamina()` getter 추가

**WBP_LRHud 구성:**
```
Canvas Panel
├── PB_HP         (UProgressBar, Fill Color 흰색, 배경 투명)
├── PB_Sta        (UProgressBar, Fill Color 흰색, 배경 투명)
└── Image_HUDFrame (커스텀 프레임 이미지, 맨 앞 레이어)
```

**M_StaFill (신규, 현재 미사용):**
- Material Domain: User Interface, Blend Mode: Masked
- UV 기반 평행사변형 마스크: `U - (1-V)*Skew → OpacityMask`
- STA 바를 직사각형으로 단순화하기로 결정, 추후 프레임 이미지 교체 예정

### 미완료 — HUD 정렬 및 마무리
- PB_HP / PB_Sta 크기와 위치를 Image_HUDFrame 각 칸 내부에 정확히 정렬
- STA 바 프레임 이미지를 직사각형 버전으로 교체
- 피격 시 HP 감소 / 달리기 시 STA 감소 인게임 동작 최종 확인

---

## 2026-05-18 — UI 전체 미니멀 라인아트 스타일 전환

### 완료 — C++ 그리드 비주얼 라인아트화 (LRInventoryGridWidget)

**NativePaint 변경:**
- 그리드 선: 쿨블루화이트 톤, `GridLineThickness` 프로퍼티로 에디터 조절 가능
- 호버 하이라이트: 단순 사각형 → 외곽선 + 네 모서리 L자 강조
- 드래그 프리뷰: 반투명 채움 박스 → 외곽선 + 내부 대각선 크로스 (라인아트 전용)
- 모든 색상/두께를 `LR|Style` 카테고리 EditDefaultsOnly로 노출 (WBP에서 실시간 조절 가능)

**GridCanvas 런타임 배치:**
- `InitGrid()` 내에서 GridCanvas CanvasPanelSlot을 `GridWidth × SlotSize`, `GridHeight × SlotSize`로 설정
- Anchor (0.5, 0.5), Alignment (0.5, 0.5) → 화면 중앙 자동 배치
- NativePaint Origin = `GridCanvas->GetCachedGeometry()` 실제 위치 기준으로 변경 (아이템 위치와 정합)

**bShowBackground 프로퍼티:**
- `EditAnywhere, BlueprintReadWrite` bool 추가
- WBP_InventoryGrid: DimOverlay(전체화면), 그리드 배경 Border의 Visibility를 bShowBackground에 바인딩
- `LRStorageWidget::InitStorage`: `PlayerInventoryWidget->bShowBackground = false` 설정 (보관함 내부 인벤 배경 미표시)

### 완료 — 아이템 아이콘 크기 수정 (LRItemWidget)

- `SetBrushFromTexture` → 수동 `FSlateBrush` 생성으로 변경
- `Brush.ImageSize = FVector2D(Item.GetEffectiveWidth() * SlotSize, Item.GetEffectiveHeight() * SlotSize)` 명시 설정
- 원본 텍스처 크기가 슬롯 크기를 무시하던 문제 해결

### 완료 — 컨텍스트 메뉴 스타일 + 위치 수정 (LRContextMenuWidget)

- `AddItem()`: 버튼 Normal/Hovered/Pressed 색상 C++에서 설정 (투명→호버 시 블루그레이)
- `PositionNearMouse()`: MenuBox가 Border 안에 중첩될 경우 부모(Border)를 기준으로 위치 지정
- `SetAutoSize(true)` 추가 → 버튼 수에 따라 배경 자동 확장

### 완료 — 인벤토리 상호작용 차단 (LRCharacter)

- `TryInteract()`: `bInventoryOpen` 가드 추가 → Tab 인벤 열린 상태에서 보관함/상호작용 차단

### 진행 중 — Blueprint 스타일 작업

| 위젯 | 상태 |
|---|---|
| WBP_InventoryGrid (DimOverlay, 배경) | 진행 중 |
| WBP_Tooltip (배경, 폰트 색) | 진행 중 |
| WBP_ContextMenu (Border 배경) | 진행 중 |
| WBP_LRHud (HP/STA 색상, 프롬프트) | 미완료 |
| WBP_ToolbarSlot (슬롯 배경, 텍스트) | 미완료 |
| WBP_PauseMenu / WBP_MainMenu | 미완료 |

### 알려진 이슈
- DimOverlay(전체화면 배경) 렌더링 문제 — 조사 중
- WBP_Storage 레이아웃 미니멀 스타일 미적용 (건너뜀)

---

## 2026-05-19 — PauseMenu/MainMenu 스타일 + 사운드 시스템 + 메인메뉴 저장 + 패키징

### 완료 — WBP_PauseMenu / WBP_MainMenu 라인아트 스타일

**ULRPauseMenuWidget 변경:**
- `NativePaint` 추가: 화면 중앙에 어두운 반투명 패널 + 흰 테두리 4변 + L자 모서리 강조 자동 드로우
- 스타일 프로퍼티 (`LR|Style` 카테고리, WBP에서 실시간 조절 가능):
  - `PanelBgColor`, `BorderColor`, `BtnHoverColor`, `PanelWidth/Height`, `BorderThickness`, `CornerSize`
- `ApplyButtonStyle()`: Normal=투명, Hovered=반투명 채움, Pressed=더 어둡게

**ULRMainMenuWidget 변경:**
- `ApplyButtonStyle()`: Normal=투명, Hovered=반투명 흰 채움(Box 방식, 테두리 없음), Pressed=조금 더 밝게
- `BtnHoverColor` EditDefaultsOnly 프로퍼티 추가 (기본 alpha 0.5)

**에디터 설정 (공통):**
- 버튼 내 TextBlock Color를 밝은 흰색 계열로 설정 필요 (어두운 배경과 대비)

---

### 완료 — 발자국 사운드 시스템 교체

**변경 전:** `FootstepTimer` + `UGameplayStatics::PlaySoundAtLocation` 인터벌 방식

**변경 후:** `UAudioComponent FootstepAudioComp` 기반 루프 방식
- 이동 중: `FootstepAudioComp->Play()` — 멈추면 `Stop()` 즉시 정지
- Walk / Run / Crouch 별 사운드 에셋 3개 → **단일 `SFX_Footstep_Walk` 에셋**으로 통합
- 상태별 피치·볼륨 변조 (BP에서 조절 가능):

| 상태 | 프로퍼티 | 기본값 |
|---|---|---|
| Run | `FootstepRunPitch` | 1.3 |
| Run | `FootstepRunVolume` | 1.3 |
| Crouch | `FootstepCrouchPitch` | 0.75 |
| Crouch | `FootstepCrouchVolume` | 0.45 |

**에디터 설정:** BP_LRCharacter → Sound → `SFX_Footstep_Walk` 에 루핑 사운드 할당 (SoundWave 루핑 체크)

---

### 완료 — 수색 루프음 제어 (LRContainer)

- `UAudioComponent* SearchLoopAudioComp` 추가 (bAutoActivate=false)
- `SFX_SearchLoop` 슬롯 추가
- `BeginInteract`: `SFX_SearchStart` 단발 재생 + `SearchLoopAudioComp->Play()` 시작
- `EndInteract`: 취소/완료 모두 `SearchLoopAudioComp->Stop()` 즉시 정지 후 `SFX_SearchComplete` 단발 재생

**에디터 설정:**
- BP_LRContainer → `SFX_SearchLoop` = 루핑 사운드 에셋 (SoundWave 루핑 체크)
- `SFX_SearchStart`, `SFX_SearchComplete` = 단발 사운드 에셋

---

### 완료 — 메인메뉴 복귀 시 데이터 저장

**문제:** `ULRPauseMenuWidget::OnMainMenuClicked()`이 저장 없이 바로 `OpenLevel` 호출

**해결:** `ALRCharacter::SaveAndGoToMainMenu()` 추가
- 인벤토리 → `GI->PersistentInventory`
- 툴바 → `GI->PersistentToolbarItems`
- 보관함 → `GI->PersistentStorageItems` + `TActorIterator<ALRStorage>`로 StorageGrid 확보
- `ULRSaveGame::Save(InvGrid, StorageGrid, ToolbarItems, this)` 디스크 저장
- `SetGamePaused(false)` → `OpenLevel("L_MainMenu")`

`OnMainMenuClicked()` → `Char->SaveAndGoToMainMenu()` 호출로 변경

---

### 완료 — 패키징 설정

- `Config/DefaultGame.ini`에 `MapsToCook` 3개 추가:
  - `/Game/LR/Maps/L_MainMenu`
  - `/Game/LR/Maps/L_Base`
  - `/Game/LR/Maps/L_DangerZone`
- 빌드 설정: `Build=Always`, `BuildConfiguration=Shipping`, `ForDistribution=True`

### 신규 에셋 (미커밋)
- `Content/LR/Audios/` — 오디오 에셋 폴더 신규
- `Content/LR/Maps/L_Scavenger.umap` — 새 스캐빈저 맵
- `Content/LR/Models/container/` — 컨테이너 모델 에셋

### 미완료
- 발자국 / 피격 / 아이템 / AI 보이스 실제 에셋 할당 (BP에서 슬롯만 비어있음)
- WBP_LRHud HP/STA 바 프레임 정렬
- HitReactMontage 에셋 준비 및 BP_LRBot 할당
- 시연 영상 촬영

---

## 2026-05-20 — 컨테이너 등급/잠금 + 무게 이동속도 + ALROpenableDoor + 그리드 배경 버그 수정

### 완료 — ILRInteractable::CanInteract() 인터페이스 확장

- 기본 구현 `return true` 추가 → 기존 클래스 무수정 호환
- `ALRCharacter::TryInteract()`: `CanInteract()` 반환 false이면 잠금 프롬프트 브로드캐스트 후 조기 반환

### 완료 — 컨테이너 등급 시스템 (ELRContainerType)

```cpp
UENUM(BlueprintType)
enum class ELRContainerType : uint8 { Normal, Locked, Alarmed };
```

| 등급 | 동작 |
|---|---|
| Normal | 기존과 동일 |
| Locked | RequiredKeyCardID 키카드가 인벤에 있어야 CanInteract() 통과 |
| Alarmed | 수색 완료 시 UAISense_Hearing::ReportNoiseEvent → AI 전투 상태 전환 |

### 완료 — 아이템 무게 이동속도 연동

- `ULRInventoryGridComponent::GetTotalWeight()`: 전체 아이템 `Weight × Quantity` 합산
- `ALRCharacter::MaxCarryWeight = 30.f`, `UpdateMovementSpeed()` 추가
- 속도 배수 = `Lerp(1.0, 0.5, Weight / MaxCarryWeight)` — 만재 시 절반 속도
- `BeginPlay`에서 `OnGridChanged` 바인딩 → 아이템 변경마다 자동 재계산

### 완료 — DangerZone 저장 버그 수정

- **원인**: L_Scavenger(DangerZone)에 ALRStorage 없음 → `StorageGrid = nullptr` → 창고 직렬화 건너뜀 → GI 캐시 덮어씌워 창고 데이터 소실
- **해결**: `ULRSaveGame::Save()` 파라미터에 `FallbackStorageItems` 추가. `StorageGrid == nullptr`이면 GI `PersistentStorageItems`를 폴백으로 직렬화

### 완료 — ALRDoor 키카드 잠금

- `bRequiresKeyCard`, `RequiredKeyCardID (EditConditionHides)` 프로퍼티 추가
- `CanInteract()`: 잠금 상태이면 인벤 순회로 키카드 확인
- `GetInteractionPrompt()`: GI `ItemRegistry`에서 키카드 이름 조회 → `[E] 이동  🔒 키카드명 필요`

### 완료 — ALROpenableDoor 신규 클래스

경첩 기반 여닫이문. `AActor + ILRInteractable` 상속.

**배치 방법:**
1. 레벨에 BP_LROpenableDoor 배치
2. 액터의 오렌지 화살표(원점)를 **경첩이 될 위치**에 정확히 맞춤
3. `HingeOffsetY` = 문 폭의 절반(cm). OnConstruction에서 메시가 Y방향으로 오프셋 → 에디터 실시간 확인 가능
4. `OpenAngle`: 양수=왼쪽, 음수=오른쪽으로 열림

### 완료 — 그리드 배경(DimOverlay) vs 실선 불일치 버그 수정

- **원인**: DimOverlay 위젯과 NativePaint 선을 별도 시스템으로 관리 → BindWidgetOptional 이름 불일치·슬롯 타입 차이 등으로 항상 미세 오차 발생
- **해결**: 배경을 NativePaint에서 `FSlateDrawElement::MakeBox`로 직접 렌더링. `Origin`·`Cols * SlotSize`를 배경과 선이 100% 공유
- `GridBackgroundColor` UPROPERTY (EditDefaultsOnly, LR|Style) 추가 — WBP에서 조절 가능
- WBP_InventoryGrid의 DimOverlay 위젯은 InitGrid에서 Collapsed 처리 (삭제해도 무방)

### 미완료
- 발자국 / 피격 / 아이템 / AI 보이스 실제 에셋 BP 슬롯 할당
- WBP_LRHud HP/STA 바 프레임 정렬
- HitReactMontage 에셋 준비 및 BP_LRBot 할당
- 시연 영상 촬영

---

## 2026-05-21 — 2차 코드 전체 리뷰 수정 + 툴바 드롭 수신 + 아이콘 실루엣 버그

### 완료 — 크래시 위험 null 역참조 수정

| 파일 | 위치 | 수정 내용 |
|---|---|---|
| `LRBot.cpp` | `TakeDamage()` | `GetAnimInstance()` null 체크 없이 `Montage_Play` 호출 → `if (UAnimInstance* Anim = ...)` 가드 추가 |
| `LRBot.cpp` | `TakedownKill()` | 동일 패턴 — 사망 몽타쥬 재생 전 null 체크 |
| `LRBotAIController.cpp` | `TryMeleeAttack()` | 동일 패턴 — 근접 공격 몽타쥬 재생 전 null 체크 |

### 완료 — const_cast 제거 (LRSaveGame.cpp)

- `ItemRegistry.Find()` 반환 타입을 `const ULRItemDataAsset* const*` 대신 `ULRItemDataAsset* const*`로 수정
- 불필요한 `const_cast<ULRItemDataAsset*>(*DataPtr)` 제거 — 인벤/툴바 두 복원 경로 모두 수정

### 완료 — 컨텍스트 메뉴 중복 바인딩 방지 (LRContextMenuWidget.cpp)

- `InitMenu()` 재호출 시 `BackdropButton->OnClicked`에 동일 핸들러가 중복 등록되는 문제
- `!BackdropButton->OnClicked.IsAlreadyBound(this, &ULRContextMenuWidget::OnBackdropClicked)` 조건 추가

### 완료 — 그리드 인벤토리 드래그 드롭 안정화 (LRInventoryGridWidget.cpp)

#### InitGrid 재열기 핸들 누수 수정
- 위젯 재사용 시 기존 `OnGridChanged` 핸들을 `Remove()` 후 `Reset()` — 재열기마다 RebuildGrid 중복 호출 방지

#### 스택 분리 드래그 SourceGrid 판단 로직 수정
- **기존**: `bRightDrag && DraggedItemData.Quantity == 1` 조건으로 판단 → Quantity 덮어쓰기 이후 비교라 항상 nullptr
- **수정**: `OriginalQuantity` 캐시를 덮어쓰기 이전에 보존 → `bRightDrag && OriginalQuantity > 1` 조건으로 정확히 판단
  - 스택 분리(우클릭, 원본 수량 > 1): `SourceGrid = nullptr` (그리드에 잔존, 복원 불필요)
  - 단일 아이템 우클릭 / 좌클릭 전체: `SourceGrid = GridComponent` (RemoveItem됨, 취소 시 복원 필요)

#### 드래그 시작 좌상단 배치 버그 수정
- **기존**: `EDragPivot::MouseDown` → `NewObject<UImage>` 첫 프레임에 AbsolutePosition=0,0 → 화면 좌상단에 아이콘 배치 후 커서 위치로 이동
- **수정**: `EDragPivot::CenterCenter` → Pivot 오프셋 = ImageSize/2. 브러시 ImageSize(ItemPxSize)가 첫 프레임부터 적용되어 정확한 위치에 배치

#### 이중 복원 버그 수정
- `NativeOnDrop`에서 배치 실패 시 선제 복원 후 `return false` → NativeOnDragCancelled 복원과 중복 발생
- 선제 복원 제거 → `NativeOnDragCancelled` 단일 경로에서만 복원 처리

#### 스택 분리 드래그 취소 복원 추가
- `NativeOnDragCancelled`: `SourceGrid == nullptr`이고 `SourceItemID != INDEX_NONE`이면 `GridComponent->AddToStack()` 호출
- 스택 분리 후 드롭 취소 시 원본 스택에 수량 반환

### 완료 — HUD 위젯 수명 관리 개선

**LRHudWidget.h/.cpp — NativeDestruct 추가:**
- `RemoveFromParent()` 후에도 캐릭터 이벤트가 발생하면 이미 제거된 HUD가 반응하는 버그 방지
- `OnHealthChanged`, `OnStaminaChanged`, `OnSearchProgressChanged/Started/Ended`, `OnInteractionPromptChanged`, `OnToolbarSlotChanged` 델리게이트 명시적 해제

**LRCharacter.h — 멤버 추가:**
- `TObjectPtr<ULRHudWidget> HudWidget` — HUD 참조 보관 (Z-order 변경에 필요)
- `IA_Interact`, `IA_Inventory`를 raw pointer → `TObjectPtr<UInputAction>`으로 통일
- `TArray<TWeakObjectPtr<ALRBot>> CachedBots`, `BotCacheTimer`, `BotCacheInterval` — 봇 캐시 (Tick 최적화)

**LRCharacter.cpp — HUD 생성 방식 수정:**
- `CreateWidget(GetWorld(), ...)` → `CreateWidget(PC, ...)` (PlayerController outer)
- 이유: `NativeConstruct`에서 `GetOwningPlayerPawn()`이 올바른 ALRCharacter를 반환하려면 PC가 outer여야 함

**LRCharacter.cpp — Tick 봇 캐시 최적화:**
- `BotCacheInterval(0.5초)`마다만 `GetAllActorsOfClass` 호출, 나머지 프레임은 캐시 순회
- `TWeakObjectPtr` — GC 시 자동 무효화, `IsValid()` 체크로 안전 접근

### 완료 — 툴바 슬롯 드래그 드롭 수신 불가 수정

**원인:**
- Slate 드롭 이벤트는 최상위 히트테스트 위젯에서 처리 후 **부모 체인으로만** 버블링
- HUD(Z=0)와 인벤토리(Z=5)는 `SOverlay` 형제 노드 → 인벤토리 위젯이 드롭 먼저 수신, HUD 툴바 슬롯에는 전달 불가

**해결 — LRCharacter.cpp:**

| 이벤트 | HUD Z-order |
|---|---|
| 인벤토리/보관함 열기 | RemoveFromParent → AddToViewport(6) (인벤 Z=5 위) |
| 인벤토리/보관함 닫기 | RemoveFromParent → AddToViewport(0) (기본 복원) |

- Remove → ReAdd 사이클로 NativeDestruct → NativeConstruct 순서 보장
- NativeConstruct에서 `RefreshAllToolbarSlots()` 재호출 → 현재 툴바 상태 자동 복원
- 적용 범위: `ToggleInventory()`, `OpenStorageScreen()`, `CloseStorageScreen()`

### 완료 — 툴바 아이콘 실루엣 버그 수정 (LRToolbarSlotWidget.cpp)

**원인:**
- `SetBrushFromTexture()`: 내부적으로 기존 `Brush` 멤버의 ResourceObject만 교체 → Blueprint 디자이너에 설정된 TintColor(어두운 색)가 유지 → 아이콘이 실루엣처럼 표시

**수정:**
- `FSlateBrush`를 직접 생성하여 `TintColor = FLinearColor::White`, `DrawAs = Image`로 명시 설정 후 `SetBrush()`로 적용
- 빈 슬롯: `DrawAs = NoDrawType` → 그리기 자체를 비활성화, 슬롯 배경만 표시

### 미완료
- 발자국 / 피격 / 아이템 / AI 보이스 실제 에셋 BP 슬롯 할당
- WBP_LRHud HP/STA 바 프레임 정렬
- HitReactMontage 에셋 준비 및 BP_LRBot 할당
- 시연 영상 촬영

---

## AI에게 전달할 세션 시작 문구

```text
Last Refuge UE5.7 C++ 프로젝트, 2026-05-21 작업 완료 상태에서 이어서.
엔진: UE 5.7.4, IDE: Rider, 접두사: LR

Day 13: 타르코프 스타일 그리드 인벤토리(ULRInventoryGridComponent, 10×5, 드래그앤드롭, 회전, Shift+클릭 이동),
        보관함 분할 UI(WBP_Storage = 인벤 좌 + 창고 우).
Day 14: 드래그 프리뷰 NativePaint 방식 교체, 아이템 스태킹(Quantity), 우클릭 1개 드래그,
        컨텍스트 메뉴(ULRContextMenuWidget, 사용하기/정보), 인벤 이미지 오정렬 버그 수정.
Day 15: 툴바 별도 물리 공간(TArray<FLRGridItem> ToolbarItems, ULRToolbarSlotWidget, 1~4키),
        ESC 일시정지 메뉴(ULRPauseMenuWidget), 메인화면(ULRMainMenuWidget, 감도·볼륨 슬라이더),
        적 애니메이션(ABP_LRBot), AI 근접공격(MeleeDamage=25, MeleeRange=150),
        제압 시스템(F키, 뒤 ±60도, DeathMontage).
Day 16: 플레이어 근접 공격(IA_Attack LMB, ECC_Pawn LineTrace 250범위 30데미지 0.8초쿨다운),
        봇 체력(MaxHealth=100, TakeDamage 오버라이드, HitReactMontage),
        피격 시 전투 전환(NotifyHitBy → EnterCombat),
        QA 버그 수정(PauseMenu nullptr 처리, BeginPlay 이동속도 재적용).
Day 17: HP/STA 바 UProgressBar로 교체(PB_HP, PB_Sta), ULRStatusComponent GetMaxHealth/Stamina getter,
        WBP_LRHud 커스텀 프레임 이미지(Image_HUDFrame) 오버레이 구조.
2026-05-18: UI 전체 미니멀 라인아트 스타일 전환 (LRInventoryGridWidget NativePaint, LRContextMenuWidget 스타일).
2026-05-19:
  완료:
    - WBP_PauseMenu: NativePaint 패널+테두리+L자 모서리, 버튼 라인아트 스타일
    - WBP_MainMenu: 버튼 호버 반투명 배경 채움 스타일 (BtnHoverColor alpha 0.5)
    - 발자국: UAudioComponent 기반 루프 방식, 단일 SFX_Footstep_Walk + 피치/볼륨 변조
    - LRContainer: SFX_SearchLoop + UAudioComponent (취소 시 즉시 정지)
    - ALRCharacter::SaveAndGoToMainMenu() — 메인메뉴 복귀 시 인벤/보관함/툴바 저장
    - DefaultGame.ini MapsToCook 3개 맵 등록 (L_MainMenu, L_Base, L_DangerZone)
2026-05-20:
  완료:
    - ELRContainerType(Normal/Locked/Alarmed): Locked=키카드 잠금, Alarmed=수색완료 시 AI 경보
    - ILRInteractable::CanInteract() 기본값 true, TryInteract에서 차단 처리
    - ALRDoor 키카드 잠금(bRequiresKeyCard, RequiredKeyCardID, GI ItemRegistry 이름 조회)
    - ALROpenableDoor 신규: HingeRoot+MeshComponent, OnConstruction Y오프셋, Tick FInterpTo 회전
    - ULRInventoryGridComponent::GetTotalWeight(), ALRCharacter::UpdateMovementSpeed()
      (OnGridChanged 바인딩, Weight/MaxCarryWeight Lerp 1.0→0.5)
    - DangerZone 저장 버그: ULRSaveGame::Save() FallbackStorageItems 파라미터로 GI 캐시 폴백
    - 그리드 배경 NativePaint 직접 렌더링(GridBackgroundColor UPROPERTY): 선과 픽셀 단위 일치
2026-05-21:
  완료:
    - [크래시 수정] LRBot::TakeDamage, TakedownKill / LRBotAIController::TryMeleeAttack
      GetAnimInstance() null 체크 추가 (몽타쥬 재생 전 null 역참조 방지)
    - [코드 정리] LRSaveGame: const_cast 제거, ItemRegistry.Find() 반환 타입 수정
    - [버그 수정] LRContextMenuWidget: IsAlreadyBound로 OnClicked 중복 바인딩 방지
    - [안정화] LRInventoryGridWidget: InitGrid 재열기 핸들 누수 수정, OriginalQuantity 캐시로
      SourceGrid 판단 로직 수정, CenterCenter 피벗으로 드래그 좌상단 배치 버그 수정,
      NativeOnDrop 이중 복원 제거, 스택 분리 취소 시 AddToStack 복원
    - [수명 관리] LRHudWidget NativeDestruct 추가 — RemoveFromParent 후 델리게이트 명시적 해제
    - [최적화] LRCharacter Tick: 봇 TWeakObjectPtr 캐시(0.5초 갱신), GetAllActorsOfClass 빈도 감소
    - [수정] LRCharacter HUD 생성: CreateWidget outer = PlayerController (NativeConstruct에서
      GetOwningPlayerPawn() 올바른 값 반환 보장)
    - [버그 수정] 툴바 슬롯 드롭 수신 불가: ToggleInventory/OpenStorageScreen/CloseStorageScreen에서
      HUD Z=6(열기)/Z=0(닫기) 재배치 — SOverlay 형제 위계 문제로 Z-order로만 드롭 우선순위 결정
    - [버그 수정] 툴바 아이콘 실루엣: RefreshSlot에서 FSlateBrush 직접 생성(TintColor=White,
      DrawAs=Image) → SetBrushFromTexture의 기존 TintColor 잔존 문제 해소
  미완료:
    - 발자국/피격/아이템/AI 보이스 실제 에셋 BP 슬롯 할당
    - WBP_LRHud HP/STA 바 프레임 정렬
    - HitReactMontage 에셋 준비 및 BP_LRBot 할당
    - 시연 영상 촬영
```
