# Last Refuge - 개발 인수인계 문서 (Day 11 진행 중)

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
    ├── Blueprints/      # BP_LRCharacter, BP_LRBot, BP_LRGameMode, BP_LRContainer (Day 9), BP_LRStorage (Day 10)
    ├── Characters/
    ├── DataAssets/      # 아이템 데이터 에셋
    ├── Input/           # IMC_Default, IA_Move, IA_Look, IA_Jump, IA_Crouch, IA_Sprint, IA_Interact (Day 9)
    ├── Maps/            # L_Day1Test → L_Day11_Blockout (Day 11)
    ├── AI/              # BT_LRBot, BB_LRBot
    └── UI/

Source/LastRefuge/
├── Public/
│   ├── Character/       # LRCharacter.h
│   ├── AI/              # LRBot.h, LRBotAIController.h, BTTask_LRSelectNextPatrolPoint.h, BTTask_LRSuspiciousScan.h
│   ├── Components/      # LRStatusComponent.h, LRInventoryComponent.h
│   ├── Interfaces/      # LRInteractable.h (GetInteractionPrompt 추가 Day 11)
│   └── Items/           # LRItemDataAsset.h, LRInventoryStructs.h, LRContainer.h (Day 9), LRStorage.h (Day 10)
└── Private/
    └── (동일 구조)
```

---

## 완성된 클래스 및 현재 상태

### ALRCharacter (Character/LRCharacter.h/.cpp)
플레이어 캐릭터 클래스.
- 1인칭 카메라 및 Enhanced Input 기반 이동/시점/점프
- 자세 시스템 (Crouch / Walk / Run) 및 스테미나/소음 연동
- 사망 시 GameMode와 연동 (`OnPlayerDied`)
- **[Day 9]** 전방 LineTrace 기반 상호작용 탐색 (`TryInteract`) 및 수색 타이머 시스템
- **[Day 9]** 이동, 점프 시 수색 인터럽트 처리 (`CancelSearch`)
- **[Day 10]** 피격 시 수색 인터럽트 (`TakeDamage`에서 `CancelSearch` 호출)
- **[Day 11]** Tick에서 조준 중인 `ILRInteractable`의 프롬프트 텍스트를 화면에 표시

### ULRInventoryComponent & 아이템 시스템 (Day 8 완비)
- `ULRItemDataAsset`: 아이템 정보(Resource, Consumable 등), 회복 수치 관리.
- `AddItem` / `RemoveItem`: 수량 기반 중첩 및 배열 관리.
- `UseItem` + `ApplyItemEffects`: 소모품 사용 및 스탯 회복 (구현 완료, UI 연결은 Day 12).
- **[Day 10]** `GetInventorySlots()` / `ClearInventory()` 추가.

### 상호작용 시스템 (Day 9 완비)
- **`ILRInteractable`**: `BeginInteract`, `EndInteract`, `GetInteractionDuration`, **[Day 11] `GetInteractionPrompt()`** 추가.
- **`ALRContainer`**: 인터페이스를 상속받은 컨테이너 액터. 수색 완료 시 `LootTable` 아이템을 인벤에 지급. `bSearched` 플래그로 재수색 방지. 프롬프트: `[E] 수색하기` / `이미 수색함`.

### 보관함 및 사망 처리 (Day 10 완비)
- **`ALRStorage`**: 기지 보관함 액터. `ILRInteractable` 구현, 즉시 상호작용(duration=0). 아이템 있으면 **Deposit**, 없으면 **Withdraw**. 사망 후에도 `StoredItems` 유지. 프롬프트: `[E] 저장하기` / `[E] 꺼내기`.
- **사망 처리**: 새 폰 스폰 시 인벤토리 자동 초기화. 피격 수색 인터럽트 완료.

### AI 시스템 (Day 6 완비)
- 감지(Perception), 순찰(Patrol), 의심(Suspicious), 전투(Combat) 상태 머신 완료.

---

## 2주 개발 일정 및 진행 현황

| Day | 작업 | 상태 |
|---|---|---|
| Day 1~6 | 기초 이동, 자세, 소음, AI 시스템 구축 | ✅ 완료 |
| Day 7 | 1주차 통합 테스트 + 마일스톤 검증 | ✅ 완료 |
| Day 8 | 인벤토리 시스템 + 아이템 데이터 에셋 | ✅ 완료 |
| Day 9 | 컨테이너 + 수색 게이지 + 인터럽트 로직 | ✅ 완료 |
| Day 10 | 아이템 사용(스탯 회복) 구현 + 보관함 + 사망 처리 | ✅ 완료 |
| Day 11 | **Day 10 검증 완료 + GetInteractionPrompt + 맵 블록아웃** | 🔄 진행 중 |
| Day 12 | **UI 시스템 (Minimum Viable HUD)** | ⬜ 미완료 |
| Day 13 | **밸런싱 (60%) + 사운드 (30%) + 사전 빌드 (10%)** | ⬜ 미완료 |
| Day 14 | **최종 빌드 + QA + 시연 영상** | ⬜ 미완료 |

---

## Day 10 검증 결과 (완료)

1. ✅ **Deposit**: 아이템 보유 상태에서 보관함 E키 → 인벤토리 비워지고 로그 출력 확인.
2. ✅ **Withdraw**: 빈 인벤토리 상태에서 보관함 E키 → 이전에 저장한 아이템 돌아옴 확인.
3. ✅ **사망 후 보관함 유지**: 보관함에 저장 → 죽음 → 리스폰 → 보관함 E키 → 아이템 인출 확인.
4. ✅ **피격 수색 인터럽트**: 수색 중 AI 피격 시 수색 즉시 취소 확인.

---

## Day 11 — 맵 레벨 디자인 (Playable Map)

**완료된 C++ 작업:**
- `ILRInteractable::GetInteractionPrompt()` 인터페이스 추가
- `ALRContainer` / `ALRStorage` 각각 프롬프트 구현
- `ALRCharacter` Tick에서 조준 오브젝트 프롬프트 화면 표시 (디버그 채널 20번)

**에디터 작업 체크리스트 (목표: 한 사이클 완주):**
- ⬜ `L_Day11_Blockout` 맵 생성
- ⬜ 3구역 BSP 블록아웃 (기지 SafeZone / 중간 위험지대 / 폐허 고위험지대)
- ⬜ NavMesh 배치 + AI 2마리 + Patrol Waypoint 2~3개 (교차점 1개)
- ⬜ 컨테이너 3~5개 배치 (입구=Scrap, 안쪽=Medkit/Ration)
- ⬜ 보관함 1개 기지에 배치
- ⬜ 통합 플레이 1회 (출발 → AI 회피 → 컨테이너 2개 수색 → 귀환 → 저장)

> ⚠️ **6시간 룰**: 블록아웃이 6시간을 넘으면 분위기·디테일은 포기하고 Day 13으로 미룬다.

---

## Day 12 — UI 시스템 (Minimum Viable HUD)

**목표:** 게임을 멈추지 않고 모든 핵심 상태를 한눈에 본다.

| 우선순위 | 항목 | 비고 |
|---|---|---|
| 🔴 필수 | Health / Stamina ProgressBar (좌측 하단) | `OnHealthChanged` / `OnStaminaChanged` 델리게이트 바인딩 |
| 🔴 필수 | Noise Indicator (중앙 상단, 원형) | Crouch/Walk/Run 차이가 눈에 보여야 함 |
| 🔴 필수 | 수색 게이지 (중앙 하단, 조건부 표시) | `OnSearchProgress(float)` 델리게이트 추가 필요 |
| 🔴 필수 | 상호작용 프롬프트 | `GetInteractionPrompt()` 이미 준비됨 |
| 🟡 권장 | 인벤토리 슬롯 6칸 아이콘 (우측 하단) | 텍스트 폴백 가능 |
| 🟢 옵션 | 게임 오버 / 클리어 화면 | 검은 화면 + 텍스트면 충분 |

**구현 순서:** UMG 위젯 → `BP_LRCharacter` HUD 클래스 지정 → StatusComponent 델리게이트 연결 → 수색 게이지 → 인벤토리는 마지막.

---

## Day 13 — 밸런싱 + 사운드 + 사전 빌드

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

## Day 14 — 최종 빌드 + QA + 시연 영상

시연 영상 컷: 타이틀(0~10s) → 잠입(10~30s) → 수색+인터럽트(30~50s) → 귀환+저장(50~70s) → 보관함 컷(70~90s)

---

## 일정 리스크 매트릭스

| 리스크 | 대응 시점 | 대응 |
|---|---|---|
| Day 11 맵 6시간 초과 | Day 11 오후 | 분위기·디테일 포기 → Day 13으로 |
| Day 12 인벤토리 UI 미완 | Day 12 저녁 | 슬롯 UI 생략, 키 입력만 유지 |
| Day 13 밸런싱 실패 | Day 13 저녁 | Day 14 오전을 밸런싱에 더 할애 |
| Day 14 빌드 깨짐 | Day 13 사전 빌드로 헤지 | PIE 녹화로 대체 |

---

## AI에게 전달할 세션 시작 문구

```text
Last Refuge UE5.7 C++ 프로젝트, Day 11 진행 중.
C++ 작업(GetInteractionPrompt, 프롬프트 표시)은 완료.
현재 에디터에서 L_Day11_Blockout 맵 블록아웃 작업 중.
엔진: UE 5.7.4, IDE: Rider, 접두사: LR
```
