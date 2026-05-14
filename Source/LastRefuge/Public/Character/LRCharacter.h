#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
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
class UCanvasPanelSlot;

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
    bool bIgnoreLookInput = false;

    void ToggleInventory();

public:
    ALRCharacter();

    // === 그리드 컴포넌트 접근자 ===
    UFUNCTION(BlueprintPure, Category = "LR|Inventory")
    ULRInventoryGridComponent* GetInventoryGrid() const { return InventoryGrid; }

    // 보관함 UI 열기/닫기 (ALRStorage::EndInteract에서 호출)
    void OpenStorageScreen(class ULRInventoryGridComponent* InStorageGrid);
    void CloseStorageScreen();

    // === HUD ===
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<ULRHudWidget> HudWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<ULRInventoryGridWidget> InventoryWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<ULRStorageWidget> StorageWidgetClass;

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
