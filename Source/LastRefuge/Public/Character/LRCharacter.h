#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Items/LRInventoryStructs.h"
#include "LRCharacter.generated.h"

class ULRStatusComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
class ULRInventoryGridComponent;
class ULRInventoryGridWidget;
class ULRStorageWidget;
class ULRHudWidget;
class ULRPauseMenuWidget;
class UCanvasPanelSlot;
class ULRItemDataAsset;

UENUM(BlueprintType)
enum class ELRMovementState : uint8
{
    Crouching,
    Walking,
    Running
};

// 수색/이동 게이지 진행률 (0.0 ~ 1.0)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSearchProgressChanged, float, Progress);
// 수색/이동 시작 — 베이스 텍스트 전달 ("수색중", "이동중" 등)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSearchStarted, FText, ProgressBaseText);
// 수색/이동 종료 — bCompleted: true=완료, false=취소 / StatusText: 취소 시 표시할 텍스트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSearchEnded, bool, bCompleted, FText, StatusText);
// 조준 중인 오브젝트의 프롬프트 텍스트 변경 (비어있으면 프롬프트 숨김)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionPromptChanged, FText, Prompt);

// 툴바 슬롯 변경 (SlotIndex 0~3, ItemData=null이면 빈 슬롯, Quantity=총 보유 수량)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnToolbarSlotChanged, int32, SlotIndex, ULRItemDataAsset*, ItemData, int32, Quantity);

UCLASS()
class LASTREFUGE_API ALRCharacter : public ACharacter
{
    GENERATED_BODY()

private:
    float NoiseMakeTimer = 0.f;

    UFUNCTION()
    void OnHealthChanged(float NewHealth, float MaxHealth);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<ULRInventoryGridComponent> InventoryGrid;

    // 인벤토리 UI (탭으로 열고 닫음)
    UPROPERTY()
    TObjectPtr<ULRInventoryGridWidget> InventoryWidget;

    // 보관함 UI
    UPROPERTY()
    TObjectPtr<ULRStorageWidget> StorageWidget;

    bool bInventoryOpen  = false;
    bool bStorageOpen    = false;
    bool bPauseMenuOpen  = false;
    bool bIgnoreLookInput = false;

    UPROPERTY()
    TObjectPtr<ULRPauseMenuWidget> PauseMenuWidget;

    void ToggleInventory();

    // 툴바 데이터 (실제 아이템 보관, 빈 슬롯 = IsEmpty()==true)
    TArray<FLRGridItem> ToolbarItems;

    // EnhancedInput 바인딩용 — 파라미터 포함 시그니처
    void HandleToolbar1(const FInputActionValue&) { UseToolbarSlot(0); }
    void HandleToolbar2(const FInputActionValue&) { UseToolbarSlot(1); }
    void HandleToolbar3(const FInputActionValue&) { UseToolbarSlot(2); }
    void HandleToolbar4(const FInputActionValue&) { UseToolbarSlot(3); }

public:
    ALRCharacter();

    // === 그리드 컴포넌트 접근자 ===
    UFUNCTION(BlueprintPure, Category = "LR|Inventory")
    ULRInventoryGridComponent* GetInventoryGrid() const { return InventoryGrid; }

    // 보관함 UI 열기/닫기 (ALRStorage::EndInteract에서 호출)
    void OpenStorageScreen(class ULRInventoryGridComponent* InStorageGrid);
    void CloseStorageScreen();

    // === 툴바 ===
    static constexpr int32 ToolbarSize = 4;

    UPROPERTY(BlueprintAssignable, Category = "LR|UI")
    FOnToolbarSlotChanged OnToolbarSlotChanged;

    // 슬롯에 아이템 저장 (드래그 드롭 후 LRToolbarSlotWidget에서 호출)
    void SetToolbarSlot(int32 SlotIndex, const FLRGridItem& Item);

    // 슬롯 비우기 — 아이템이 있으면 인벤토리로 반환
    void ClearToolbarSlot(int32 SlotIndex);

    // 슬롯에서 아이템 꺼내기 — 인벤 반환 없이 제거 (드래그 시작 시 사용)
    FLRGridItem TakeToolbarItem(int32 SlotIndex);

    // 1~4 키 → 해당 슬롯 아이템 즉시 사용
    void UseToolbarSlot(int32 SlotIndex);

    const TArray<FLRGridItem>& GetToolbarItems() const { return ToolbarItems; }

    float GetCurrentSearchTime() const { return CurrentSearchTime; }
    float GetSearchDuration()    const { return SearchDuration; }

    // 툴바 단축키 InputAction (에디터에서 할당)
    UPROPERTY(EditAnywhere, Category = "Input|Toolbar")
    TObjectPtr<UInputAction> IA_Toolbar1;

    UPROPERTY(EditAnywhere, Category = "Input|Toolbar")
    TObjectPtr<UInputAction> IA_Toolbar2;

    UPROPERTY(EditAnywhere, Category = "Input|Toolbar")
    TObjectPtr<UInputAction> IA_Toolbar3;

    UPROPERTY(EditAnywhere, Category = "Input|Toolbar")
    TObjectPtr<UInputAction> IA_Toolbar4;

    // ESC 일시정지 메뉴 토글 (LRPauseMenuWidget에서도 호출)
    void TogglePauseMenu();

    // === HUD ===
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<ULRHudWidget> HudWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<ULRInventoryGridWidget> InventoryWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<ULRStorageWidget> StorageWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<ULRPauseMenuWidget> PauseMenuWidgetClass;

    // ESC 키 InputAction (에디터에서 할당)
    UPROPERTY(EditAnywhere, Category = "Input")
    TObjectPtr<UInputAction> IA_Menu;

    // 제압 키 InputAction (에디터에서 할당)
    UPROPERTY(EditAnywhere, Category = "Input")
    TObjectPtr<UInputAction> IA_Takedown;

    // 공격 키 InputAction (에디터에서 할당, 기본 LMB)
    UPROPERTY(EditAnywhere, Category = "Input")
    TObjectPtr<UInputAction> IA_Attack;

    // === Combat ===
    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float AttackDamage = 30.f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float AttackRange = 250.f;

    UPROPERTY(EditDefaultsOnly, Category = "Combat")
    float AttackCooldown = 0.8f;

    // === Sound ===
    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    TObjectPtr<USoundBase> SFX_Footstep_Walk;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    TObjectPtr<USoundBase> SFX_Footstep_Run;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    TObjectPtr<USoundBase> SFX_Footstep_Crouch;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    TObjectPtr<USoundBase> SFX_ItemUse;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    TObjectPtr<USoundBase> SFX_Hit;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    TObjectPtr<USoundBase> SFX_Death;

    UPROPERTY(EditDefaultsOnly, Category = "Sound")
    TObjectPtr<USoundBase> SFX_Attack;

    // === UI 델리게이트 ===
    UPROPERTY(BlueprintAssignable, Category = "LR|UI")
    FOnSearchProgressChanged OnSearchProgressChanged;

    UPROPERTY(BlueprintAssignable, Category = "LR|UI")
    FOnSearchStarted OnSearchStarted;

    UPROPERTY(BlueprintAssignable, Category = "LR|UI")
    FOnSearchEnded OnSearchEnded;

    UPROPERTY(BlueprintAssignable, Category = "LR|UI")
    FOnInteractionPromptChanged OnInteractionPromptChanged;

    UFUNCTION(BlueprintPure, Category = "Movement")
    ELRMovementState GetMovementState() const { return MovementState; }

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    TObjectPtr<class UAIPerceptionStimuliSourceComponent> StimuliSource;

    // === Components ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    TObjectPtr<UCameraComponent> FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Status")
    TObjectPtr<ULRStatusComponent> StatusComponent;

    // === Input ===
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> CrouchAction;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* IA_Interact;

    // 인벤토리 열기/닫기 (Tab키)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    class UInputAction* IA_Inventory;

    // === Crouch ===
    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float CrouchCameraHeight = 30.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float StandCameraHeight = 60.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float CrouchCapsuleHalfHeight = 55.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float StandCapsuleHalfHeight = 96.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float CrouchInterpSpeed = 10.f;

    float TargetCameraHeight = 60.f;
    float TargetCapsuleHalfHeight = 96.f;

    float FootstepTimer = 0.f;

    virtual void Tick(float DeltaTime) override;

    // === Movement ===
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    ELRMovementState MovementState = ELRMovementState::Walking;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float CrouchSpeed = 100.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float WalkSpeed = 250.f;

    UPROPERTY(EditDefaultsOnly, Category = "Movement")
    float RunSpeed = 400.f;

    void SetMovementState(ELRMovementState NewState);

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartJump(const FInputActionValue& Value);
    void StopJump(const FInputActionValue& Value);
    void ToggleCrouch(const FInputActionValue& Value);
    void StartSprint(const FInputActionValue& Value);
    void StopSprint(const FInputActionValue& Value);

    void ReportMovementNoise();
    void TryInteract();
    void CancelSearch();
    void TryTakedown();
    void TryAttack(const FInputActionValue& Value);

    bool bCanAttack = true;
    FTimerHandle AttackCooldownHandle;

    // --- 수색 게이지 상태 ---
    bool bIsSearching = false;
    float CurrentSearchTime = 0.f;
    float SearchDuration = 0.f;

    // --- 소음 리플 링 ---
    float NoiseRingPhase[3] = { 0.f, 0.333f, 0.667f };
    static constexpr float NoiseRingCycleSpeed = 1.0f;

    UPROPERTY()
    AActor* CurrentInteractable;

    FText CurrentPromptText;
};
