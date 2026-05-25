# Last Refuge — 코드 리뷰 & 구조 설명서

## 목차

1. [전체 구조 한눈에 보기](#1-전체-구조-한눈에-보기)
2. [핵심 개념 — 언리얼의 기본 틀](#2-핵심-개념--언리얼의-기본-틀)
3. [플레이어 캐릭터 시스템](#3-플레이어-캐릭터-시스템)
4. [AI 적 시스템](#4-ai-적-시스템)
5. [인벤토리 시스템](#5-인벤토리-시스템)
6. [상호작용 오브젝트](#6-상호작용-오브젝트)
7. [UI 시스템](#7-ui-시스템)
8. [게임 흐름 & 저장 시스템](#8-게임-흐름--저장-시스템)
9. [코드 패턴 & 잘 만들어진 부분](#9-코드-패턴--잘-만들어진-부분)
10. [개선 가능한 부분](#10-개선-가능한-부분)

<br><br>

## 1. 전체 구조 한눈에 보기

```
LastRefuge/Source/
├── Core (게임의 뼈대)
│   ├── LastRefugeCharacter      ← 모든 캐릭터의 조상
│   ├── LastRefugePlayerController ← 입력 처리 담당
│   ├── LastRefugeCameraManager   ← 카메라 각도 제한
│   └── LastRefugeGameMode        ← 게임 규칙 관리자
│
├── Character/ (플레이어)
│   └── LRCharacter              ← 실제 조작 캐릭터
│
├── AI/ (적 봇)
│   ├── LRBot                   ← 봇 캐릭터 본체
│   ├── LRBotAIController       ← 봇의 두뇌 (행동 결정)
│   └── LRBotAnimInstance       ← 봇 애니메이션
│
├── Items/ (아이템)
│   ├── LRItemDataAsset         ← 아이템 정보 정의
│   └── LRInventoryStructs      ← 인벤토리 자료구조
│
├── Components/ (재사용 가능한 기능 조각)
│   ├── LRInventoryGridComponent ← 격자 인벤토리 로직
│   └── LRStatusComponent       ← 체력/스태미나 관리
│
├── Actors/ (월드에 놓이는 오브젝트)
│   ├── LRDoor                  ← 레벨 이동 문 (키카드 잠금 지원)
│   ├── LROpenableDoor          ← 경첩 기반 여닫이문 (2026-05-20 신규)
│   ├── LRStorage               ← 보관함 (기지)
│   └── LRContainer             ← 등급별 잠금/경보 컨테이너
│
├── Interfaces/ (약속/계약)
│   └── LRInteractable          ← "E키로 상호작용" 공통 규격
│
├── UI/ (화면에 보이는 것들)
│   ├── LRHudWidget             ← 게임 중 HUD (체력바, 도구모음 등)
│   ├── LRInventoryGridWidget   ← 인벤토리 격자 화면
│   ├── LRStorageWidget         ← 보관함 화면 (양쪽 격자)
│   └── LRPauseMenuWidget       ← 일시정지 메뉴
│
└── Systems/ (게임 전반 관리)
    ├── LRGameMode              ← 죽음/리스폰 처리
    ├── LRGameInstance          ← 레벨 간 데이터 유지
    └── LRSaveGame              ← 디스크 저장/불러오기
```

<br><br>

## 2. 핵심 개념 — 언리얼의 기본 틀

코드를 읽기 전에 알아야 할 언리얼 용어들입니다.

### Actor vs Component
- **Actor**: 레벨에 배치되는 모든 물체 (캐릭터, 문, 아이템 등)  
- **Component**: Actor에 붙이는 기능 조각. 예: `LRStatusComponent`는 캐릭터에 붙어서 체력 관리를 담당

> 비유: Actor = 자동차, Component = 엔진/바퀴/에어컨

### Delegate (이벤트 알림 시스템)
```cpp
// 선언 (어떤 정보를 보낼지)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, NewHealth);

// 발사 (무언가 변했을 때)
OnHealthChanged.Broadcast(CurrentHealth);

// 구독 (변화를 받아서 처리)
StatusComp->OnHealthChanged.AddDynamic(this, &ULRHudWidget::UpdateHealthBar);
```
> 비유: 라디오 방송국(Broadcast)과 라디오 수신기(AddDynamic). 방송국은 청취자가 누구인지 몰라도 됨.

### Interface (공통 약속)
```cpp
// 약속 정의
class ILRInteractable {
    virtual bool CanInteract(ALRCharacter* Player) const { return true; }  // 기본값 true
    virtual void BeginInteract(ALRCharacter* Player) = 0;
    virtual FText GetInteractionPrompt() const = 0;
};

// 문도, 보관함도, 컨테이너도, 여닫이문도 이 약속을 지킴
class ALRDoor        : public AActor, public ILRInteractable { ... }
class ALROpenableDoor: public AActor, public ILRInteractable { ... }
class ALRStorage     : public AActor, public ILRInteractable { ... }
class ALRContainer   : public AActor, public ILRInteractable { ... }
```
`CanInteract()`는 기본값 `true` — 기존 클래스를 수정하지 않아도 됨.  
`ALRCharacter::TryInteract()`에서 false 반환 시 잠금 프롬프트를 브로드캐스트하고 조기 반환.

> 비유: 플러그 규격. 220V 규격을 지키면 어떤 제품이든 콘센트에 꽂힘.

<br><br>

## 3. 플레이어 캐릭터 시스템

### 3-1. 상속 구조

```
ACharacter (언리얼 기본)
    └── ALastRefugeCharacter (1인칭 카메라, 기본 입력)
            └── ALRCharacter (실제 플레이어 — 모든 기능 탑재)
```

왜 이렇게 나눴냐면: `ALastRefugeCharacter`는 나중에 다른 캐릭터를 만들 때도 재사용할 수 있는 공통 뼈대입니다.

### 3-2. 이동 상태 시스템

```cpp
enum class ELRMovementState : uint8 {
    Crouch,  // 앉기
    Walk,    // 걷기
    Run      // 달리기
};
```

이 enum 하나로 **이동 속도**, **카메라 높이**, **소음 크기**, **스태미나 소모** 등을 한꺼번에 제어합니다.

```
달리기 → 스태미나 소모 → 스태미나 0 → 강제로 걷기로 변경
앉기   → 카메라가 부드럽게 낮아짐 (Lerp 사용) → 소음 최소화
```

**카메라 높이 보간(Lerp):**
```cpp
// 매 프레임마다 목표 높이로 조금씩 이동 (딱딱하게 점프하지 않고 부드럽게)
CurrentCameraHeight = FMath::FInterpTo(CurrentCameraHeight, TargetHeight, DeltaTime, 10.f);
```

### 3-3. 스태미나 시스템

```
달리는 중 → DrainRate(20/초)로 감소
멈춤 → 1.5초 대기 후 → RegenRate(15/초)로 회복
```

타이머로 구현:
```cpp
// 달리기 멈추면 1.5초 후 회복 시작
GetWorldTimerManager().SetTimer(StaminaRegenTimerHandle, this, 
    &ALRCharacter::StartStaminaRegen, StaminaRegenDelay);
```

### 3-4. 상호작용 시스템 (E키)

```
매 프레임 → 앞으로 200cm 레이 발사
→ ILRInteractable 구현체에 닿으면 → 프롬프트 표시 "[E] 문 열기"
→ E키 누르면 → BeginInteract() 호출
→ 진행 시간(Duration) 동안 유지 → EndInteract() 호출
→ 이동하거나 점프하면 → 취소
```

### 3-5. 소음 시스템 (AI에게 들킴)

```cpp
// 이동 상태에 따라 소음 반경이 달라짐
Crouch: 100cm   ← 거의 들리지 않음
Walk:   400cm   ← 보통
Run:   1200cm   ← 크게 들림

// 언리얼 AI 인식 시스템에 소음 보고
UAISense_Hearing::ReportNoiseEvent(GetWorld(), Location, Loudness, this, Range);
```

플레이어 주변에 **3개의 원**이 그려지는 것도 이 소음 반경을 시각화한 것입니다.

### 3-6. 무게 기반 이동속도 연동

```cpp
// 인벤 변경 시 자동 호출 (OnGridChanged 바인딩)
void ALRCharacter::UpdateMovementSpeed()
{
    const float WeightRatio = FMath::Clamp(
        InventoryGrid->GetTotalWeight() / MaxCarryWeight, 0.f, 1.f);
    const float SpeedMult = FMath::Lerp(1.0f, 0.5f, WeightRatio);
    GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * SpeedMult;
}
```

- 빈 인벤 → 100% 속도 / 만재(30kg) → 50% 속도
- 아이템을 집거나 버릴 때 실시간으로 재계산

### 3-7. 근접 전투 & 제압

- **공격(LMB):** 250cm 앞까지 히트 스캔, 쿨다운 0.8초, 30 데미지
- **제압(F키):** 봇 뒤에서만 가능, 봇 라인트레이스로 위치 검증 후 즉사 처리

### 3-8. Tick 성능 최적화 — 봇 캐시

제압 판정을 위해 Tick마다 `GetAllActorsOfClass`를 호출하면 봇이 많아질수록 부담이 커집니다. 0.5초 간격 캐시로 해결:

```cpp
// LRCharacter.h
TArray<TWeakObjectPtr<ALRBot>> CachedBots;
float BotCacheTimer = 0.f;
static constexpr float BotCacheInterval = 0.5f;

// LRCharacter.cpp — Tick
BotCacheTimer -= DeltaTime;
if (BotCacheTimer <= 0.f)
{
    BotCacheTimer = BotCacheInterval;
    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALRBot::StaticClass(), Found);
    CachedBots.Reset(Found.Num());
    for (AActor* A : Found)
        if (ALRBot* B = Cast<ALRBot>(A))
            CachedBots.Add(B);
}

for (const TWeakObjectPtr<ALRBot>& BotPtr : CachedBots)
{
    ALRBot* Bot = BotPtr.Get();
    if (!IsValid(Bot) || Bot->bIsDead) continue;
    // 제압 거리/각도 판정 ...
}
```

- `TWeakObjectPtr` — UPROPERTY 없이도 GC 시 자동 무효화, dangling 포인터 위험 없음
- `IsValid()` 체크로 GC 직후 프레임에도 안전한 접근 보장

<br><br>

## 4. AI 적 시스템

### 4-1. 봇의 3가지 상태

```
Patrol (순찰) → Suspicious (의심) → Combat (전투)
    ↑_________________________↑___________|
```

```cpp
enum class ELRBotState : uint8 {
    Patrol,      // 웨이포인트 순환 이동
    Suspicious,  // 마지막 위치 조사
    Combat       // 플레이어 추격 & 공격
};
```

### 4-2. AI 인식 시스템 (눈 & 귀)

언리얼의 **AI Perception** 컴포넌트를 사용합니다.

```
시야(Sight):
  - 반경: 1200cm
  - 반각도: 45° (총 90° 시야각)
  - 시야 상실 반경: 1800cm

청각(Hearing):
  - 반경: 1800cm
  - 플레이어 소음이 이 범위 안에 들어오면 반응
```

**감지 후 처리:**
```cpp
void ALRBotAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors) {
    // 시각으로 감지 → 즉시 전투
    // 청각으로 감지 → 의심 상태로, 마지막 위치 기억
    // 감지 사라짐 → 순찰로 복귀
}
```

### 4-3. 블랙보드 — 봇의 기억

```
블랙보드 키        | 타입    | 의미
-----------------|--------|------------------
PatrolTarget     | FVector | 현재 이동할 웨이포인트
PatrolIndex      | int32   | 몇 번째 웨이포인트인지
PlayerActor      | AActor* | 추격할 플레이어 참조
BotState         | int32   | 현재 상태 (0=순찰, 1=의심, 2=전투)
LKL              | FVector | 마지막으로 플레이어를 본 위치
```

> 비유: 블랙보드 = 봇이 메모해두는 포스트잇. 행동 트리는 이 메모를 읽고 행동을 결정.

### 4-4. 근접 공격 루프

```
전투 상태 진입
→ 타이머 시작 (1.5초 간격)
→ 플레이어까지 거리 체크 (150cm 이내?)
→ 이내면 → 애니메이션 몽타주 재생 → 데미지 25 적용
→ 이외면 → 이동 중, 공격 스킵
```

<br><br>

## 5. 인벤토리 시스템

이 프로젝트에서 공을 많이 들인 부분입니다.

### 5-1. 격자 인벤토리의 핵심 아이디어

2D 격자를 **1D 배열**로 표현합니다.

```
격자 (4x3 예시):
[A][A][ ][ ]
[A][A][B][ ]    → 배열: [A,A, , ,A,A,B, , , , , ]
[ ][ ][ ][ ]

인덱스 공식: index = y * GridWidth + x
```

아이템 정보는 별도 해시맵에 저장:
```cpp
TMap<FGuid, FLRGridItem> ItemMap;  // ID → 아이템 데이터
```
덕분에 아이템 삭제가 O(1) (배열 전체를 뒤지지 않아도 됨).

### 5-2. 아이템 데이터 구조

```cpp
struct FLRGridItem {
    ULRItemDataAsset* DataAsset;  // 아이템 정의 (이름, 아이콘, 효과)
    int32 GridX, GridY;           // 격자 위치
    int32 Quantity;               // 수량 (스택 가능)
    bool bRotated;                // 90도 회전 여부
};
```

아이템 정의는 **Data Asset**으로 분리:
```cpp
// LRItemDataAsset.h
FText ItemName;
FText ItemDescription;
UTexture2D* ItemIcon;
ELRItemType ItemType;         // Resource/Consumable/Equipment/KeyItem
int32 GridWidth, GridHeight;  // 격자에서 차지하는 칸 수
float HealthRestore;          // 소모품이면 체력 회복량
float StaminaRestore;         // 소모품이면 스태미나 회복량
FString ItemID;               // 저장/불러오기용 고유 문자열
```

> DataAsset = 아이템의 "설계도". 런타임 아이템 = 설계도를 복사한 "실체".

### 5-3. 아이템 배치 알고리즘

```
PlaceItem(X, Y, Item, bRotated) 호출
→ 해당 위치에 아이템이 차지할 모든 칸 검사
→ 비어있으면 → 배치 성공, ItemID 반환
→ 막혀있으면 → 실패

FindEmptySpace(Item) 호출 (자동 배치)
→ 왼쪽 위부터 오른쪽 아래로 순서대로 시도
→ 원래 방향으로 안 되면 → 실패
```

### 5-4. 스태킹 (겹쳐쌓기)

```cpp
// 동일 아이템이 있고 MaxStackSize 미만이면 스택
if (CanStack(NewItem)) {
    AddToStack(ExistingItemID, Quantity);
} else {
    PlaceItem(X, Y, NewItem);
}
```

<br><br>

## 6. 상호작용 오브젝트

세 종류의 오브젝트가 모두 `ILRInteractable` 인터페이스를 구현합니다.

### 6-1. ALRDoor — 레벨 이동 문

```
E키 누름 (5초 유지)
→ 인벤토리 → GameInstance에 저장
→ 보관함 데이터 → GameInstance에 저장 (bSaveStorage=true인 경우)
→ UGameplayStatics::OpenLevel() 호출
```

### 6-2. ALRStorage — 기지 보관함

```
E키 누름 (즉시)
→ 스토리지 UI 열림 (왼쪽=플레이어 인벤토리, 오른쪽=보관함)
→ Shift+클릭으로 아이템 이동
→ E키 다시 누르면 닫힘
```

### 6-3. ALRContainer — 등급별 잠금/경보 컨테이너

```
BeginPlay에서 LootTable 아이템 랜덤 배치
E키 누름 (3초 유지)
→ [Locked] 키카드 미소지 → CanInteract() false → 잠금 프롬프트 표시
→ [Alarmed] 수색 완료 → UAISense_Hearing::ReportNoiseEvent → AI 경보 전투 전환
→ [Normal/Locked] 완료 → 아이템 플레이어 인벤토리로 이동
→ bSearched = true (다시 뒤질 수 없음)
```

컨테이너 등급 enum:
```cpp
enum class ELRContainerType : uint8 { Normal, Locked, Alarmed };
```

### 6-4. ALROpenableDoor — 경첩 기반 여닫이문

레벨 이동과 무관한 인게임 여닫이문. 경첩 위치에 액터 원점을 맞춰 배치.

```
배치 방법:
  액터 원점(오렌지 화살표) = 경첩 위치
  HingeOffsetY = 문 폭의 절반(cm) → 메시가 Y축으로 오프셋
  OnConstruction에서 오프셋 적용 → 에디터에서 실시간 확인 가능

동작 흐름:
  E키 → bIsOpen 토글 → TargetYaw 설정 → Tick 활성화
  → FInterpTo로 부드러운 회전 → 0.1도 이내 도달 시 스냅 + Tick 비활성화
  → [잠금] CanInteract(): 열린 상태면 항상 허용, 닫힌 상태면 키카드 체크
```

<br><br>

## 7. UI 시스템

### 7-1. HUD 위젯 (게임 중 화면)

```
화면 구성:
┌──────────────────────────────────────────────┐
│                                              │
│                                              │
│                                              │
│                  [크로스헤어]                 │
│               [상호작용 프롬프트]             │
│                                              │
│                                              │
│  ████████ 체력                               │
│  ████     스태미나      [1][2][3][4]         │
└─────────────────────────────────────────────┘
```

HUD는 **Delegate를 구독**하여 값이 변할 때만 업데이트됩니다:
```cpp
// 캐릭터가 방송
StatusComp->OnHealthChanged.Broadcast(NewHealth);

// HUD가 수신
StatusComp->OnHealthChanged.AddDynamic(this, &ULRHudWidget::UpdateHealthBar);
```

**NativeDestruct — 델리게이트 명시적 해제:**
```cpp
// HUD를 RemoveFromParent 후 재추가하는 도중 캐릭터 이벤트가 발생하면
// 이미 Viewport에서 제거된 HUD가 반응할 수 있음 → NativeDestruct에서 해제
void ULRHudWidget::NativeDestruct()
{
    if (StatusComponent)
    {
        StatusComponent->OnHealthChanged.RemoveDynamic(this, &ULRHudWidget::OnHealthChanged);
        StatusComponent->OnStaminaChanged.RemoveDynamic(this, &ULRHudWidget::OnStaminaChanged);
    }
    if (OwnerCharacter) { /* 검색/프롬프트/툴바 델리게이트 해제 */ }
    Super::NativeDestruct();
}
```

**HUD Z-order — 인벤토리/보관함 열기 시 툴바 드롭 수신 보장:**
```
HUD(Z=0) + 인벤토리(Z=5) → HUD 툴바가 인벤 위젯 뒤에 묻혀 드롭 이벤트 수신 불가
해결: 인벤/보관함 열기 → HUD RemoveFromParent → AddToViewport(6)
     인벤/보관함 닫기 → HUD RemoveFromParent → AddToViewport(0) 복원
```
- Slate 드롭 이벤트는 최상위 히트테스트 위젯에서 처리 후 부모 체인으로만 버블링
- HUD와 인벤토리는 `SOverlay` 형제 노드이므로 Z-order로만 우선순위 결정됨
- `ALRCharacter`가 `TObjectPtr<ULRHudWidget> HudWidget` 멤버로 HUD 참조를 보관

### 7-2. 인벤토리 격자 위젯

가장 복잡한 UI 코드입니다.

**커스텀 렌더링 (NativePaint)**:
```
매 프레임 (Paint 단계):
→ 격자 선 그리기
→ 아이템 있는 칸 → 아이콘 그리기
→ 드래그 중인 아이템 → 초록/빨간 미리보기 표시
```

**드래그 앤 드롭 흐름**:
```
마우스 누름 → 아이템 위? → 드래그 시작
이동 중 → 현재 격자 위치 계산 → 배치 가능 여부로 색깔 변경
마우스 놓음 → 배치 시도 → 실패하면 FindEmptySpace() 자동 시도
```

**좌표 변환**:
```cpp
// 픽셀 좌표 → 격자 좌표
int32 GridX = (MouseX - GridOriginX) / SlotSize;
int32 GridY = (MouseY - GridOriginY) / SlotSize;
```

### 7-3. 저장/불러오기 연동

일시정지 메뉴 버튼 → `ALRCharacter::SaveAndGoToMainMenu()`:
```
현재 인벤토리 + 보관함 + 도구모음 → ULRSaveGame::Save()
→ 디스크에 "LRInventorySave" 슬롯으로 저장
→ 메인 메뉴 레벨로 이동
```

<br><br>

## 8. 게임 흐름 & 저장 시스템

### 8-1. 레벨 시작 시 순서

```
BeginPlay
→ 1인칭 카메라 & 입력 설정
→ GameInstance.bHasTravelData == true?
    → Yes: 저장된 인벤토리 복원
    → No: 빈 인벤토리로 시작
→ HUD 생성 & 화면에 붙임
```

### 8-2. 플레이어 사망 처리

```
체력 0
→ 사망 SFX
→ GameMode.OnPlayerDied() 호출
→ GI.PersistentInventory / PersistentToolbarItems 초기화, bHasTravelData = true (창고는 유지)
→ 모든 봇 → 순찰 상태 초기화
→ Pawn 제거 후 "SafeZone" 태그된 PlayerStart에서 리스폰
→ RespawnLevelName 설정 시 해당 레벨로 이동
```

### 8-3. 데이터 지속성 구조

```
GameInstance (메모리, 레벨 간 유지)
├── PersistentInventory     ← 플레이어 인벤토리
├── PersistentStorageItems  ← 기지 보관함
├── PersistentToolbarItems  ← 도구모음
└── ItemRegistry            ← ItemID → DataAsset 매핑

SaveGame (디스크, 게임 종료 후에도 유지)
└── "LRInventorySave" 슬롯
    ├── SavedItems[]        ← 인벤토리 + 보관함
    └── SavedToolbarItems[] ← 도구모음
```

**저장 경로 3가지:**
- 빠른 저장 (레벨 이동 시): GameInstance에만 저장
- 완전 저장 (메뉴 저장 버튼): 디스크까지 기록
- 종료 저장 (게임 종료 버튼): 하드에 저장 기록

<br><br>

## 9. 코드 패턴 & 잘 만들어진 부분

### ✅ 인터페이스로 결합도 낮추기

문, 보관함, 컨테이너가 모두 다르지만 `ILRInteractable`이라는 약속 하나로 동일하게 처리:
```cpp
// 캐릭터 코드에서 문이든 보관함이든 동일하게 처리
if (ILRInteractable* Interactable = Cast<ILRInteractable>(HitActor)) {
    Interactable->BeginInteract(this);
}
```

### ✅ Delegate로 느슨한 연결

UI 코드가 캐릭터 코드를 직접 참조하지 않음:
```cpp
// 캐릭터는 "나 체력 변했어" 만 알림
// HUD는 알아서 반응 (캐릭터는 HUD 존재 몰라도 됨)
```

### ✅ Component로 관심사 분리

`LRStatusComponent`는 체력/스태미나만 담당.  
`LRInventoryGridComponent`는 격자 로직만 담당.  
`ALRCharacter`는 이들을 조합해서 사용.

### ✅ Data Asset으로 데이터와 로직 분리

아이템 수치(체력 회복량, 크기 등)가 코드 안에 하드코딩되지 않고 에디터에서 수정 가능한 DataAsset으로 분리됨.

### ✅ 해시맵으로 O(1) 아이템 조회

```cpp
TMap<FGuid, FLRGridItem> ItemMap;
// ID만 알면 즉시 접근 — 배열 전체를 순환하지 않아도 됨
```

<br><br>

## 10. 개선 가능한 부분

### ⚠️ 하드코딩된 아이템 경로

```cpp
// 현재: 경로를 코드에 직접 씀
ItemRegistry.Add("medkit", LoadObject<ULRItemDataAsset>(nullptr, 
    TEXT("/Game/Items/DA_Medkit")));

// 더 좋은 방법: DataTable이나 폴더 스캔으로 자동 등록
```

### ⚠️ 저장 파일 버전 관리 없음

아이템 구조를 바꾸면 기존 저장 파일이 깨질 수 있음. 버전 번호 필드 추가 권장:
```cpp
UPROPERTY() int32 SaveVersion = 1;
```

### ⚠️ 인벤토리 가득 찼을 때 아이템 분실 가능

`FindEmptySpace()`가 실패하면 아이템이 사라질 수 있음. 바닥에 드롭하는 폴백 필요.

### ⚠️ 근접 공격이 Controller에만 있음

봇의 근접 공격 로직이 `LRBotAIController`에 직접 구현됨. 행동 트리 태스크로 분리하면 재사용성이 높아짐.

<br><br>

## 부록 — 파일 빠른 참조표

| 파일 | 역할 한 줄 요약 |
|------|---------------|
| `LRCharacter.h/cpp` | 플레이어 캐릭터 전체 (이동/전투/상호작용/인벤토리 UI) |
| `LRBot.h/cpp` | 적 봇 캐릭터 (인식 컴포넌트, 체력, 애니메이션 상태) |
| `LRBotAIController.h/cpp` | 봇 두뇌 (상태 전환, 공격 타이머, 블랙보드 업데이트) |
| `LRInventoryGridComponent.h/cpp` | 격자 인벤토리 전체 로직 (배치/제거/스택/저장) |
| `LRStatusComponent.h/cpp` | 체력 & 스태미나 (소모, 회복, 델리게이트 방송) |
| `LRItemDataAsset.h/cpp` | 아이템 설계도 (이름, 아이콘, 효과, 크기) |
| `LRDoor.h/cpp` | 레벨 이동 + 인벤토리 직렬화 + 키카드 잠금 |
| `LROpenableDoor.h/cpp` | 경첩 기반 여닫이문 (HingeRoot, FInterpTo 회전, 키카드 잠금) |
| `LRStorage.h/cpp` | 기지 보관함 UI 트리거 |
| `LRContainer.h/cpp` | 등급별(Normal/Locked/Alarmed) 수색 컨테이너 |
| `LRInteractable.h` | 상호작용 인터페이스 (E키 약속) |
| `LRHudWidget.h/cpp` | 게임 중 HUD (체력/스태미나/도구모음/프롬프트) |
| `LRInventoryGridWidget.h/cpp` | 격자 인벤토리 UI (커스텀 렌더링 + 드래그앤드롭) |
| `LRStorageWidget.h/cpp` | 보관함 양쪽 격자 UI |
| `LRGameInstance.h/cpp` | 레벨 간 데이터 캐시 + 아이템 레지스트리 |
| `LRSaveGame.h/cpp` | 디스크 저장/불러오기 직렬화 |
| `LRGameMode.h/cpp` | 사망/리스폰/봇 리셋 처리 |

<br><br>

*생성일: 2026-05-20 | 프로젝트: Last Refuge | 엔진: Unreal Engine 5.7.4*
